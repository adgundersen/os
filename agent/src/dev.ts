/**
 * dev.ts — local development server
 *
 * Replaces the full Linux stack with mocks so you can run on macOS:
 *   - Serves UI static files from ../ui/
 *   - Mocks /auth/me and /auth/auth (skips PAM login)
 *   - Mocks dock at :7701 by reading ../apps/*\/crimata.json directly
 *   - Runs the real agent (WebSocket + /events)
 *
 * Usage:
 *   ANTHROPIC_API_KEY=sk-... npm run dev
 */

import express from 'express'
import { WebSocketServer, WebSocket } from 'ws'
import { createServer } from 'http'
import { createServer as createHttpServer } from 'http'
import { readFileSync, readdirSync } from 'fs'
import { join, resolve } from 'path'
import { Canvas }      from './canvas'
import { handleEvent } from './agent'
import { BusEvent }    from './types'

const PORT     = 7702
const API_KEY  = process.env.ANTHROPIC_API_KEY ?? ''
const UI_DIR   = resolve(__dirname, '../../ui')
const APPS_DIR = resolve(__dirname, '../../apps')

/* ── Read installed apps from local crimata.json files ───────────────────── */

function localApps() {
  const apps: unknown[] = []
  try {
    for (const entry of readdirSync(APPS_DIR, { withFileTypes: true })) {
      if (!entry.isDirectory()) continue
      try {
        const raw = readFileSync(join(APPS_DIR, entry.name, 'crimata.json'), 'utf8')
        const m   = JSON.parse(raw)
        apps.push({ running: true, ...m })
      } catch { /* skip malformed manifests */ }
    }
  } catch { /* apps dir missing */ }
  return apps
}

/* ── Mock dock on :7701 so agent's fetchApps() works ─────────────────────── */

const dockApp = express()
dockApp.get('/apps', (_req, res) => res.json(localApps()))
createHttpServer(dockApp).listen(7701, () =>
  console.log('  mock dock      → :7701'))

/* ── Main dev server on :7702 ────────────────────────────────────────────── */

const app    = express()
const server = createServer(app)
const wss    = new WebSocketServer({ server, path: '/ws' })
const canvas = new Canvas()

let activeWs:      WebSocket | null = null
let sessionStarted = false

app.use(express.json())
app.use(express.static(UI_DIR))     // serve ../ui/ at root

// Mock auth — skip PAM, always succeed
app.get('/auth/me',   (_req, res) => res.json({ ok: true, username: 'dev' }))
app.post('/auth/auth', (_req, res) =>
  res.json({ success: true, token: 'dev-token', username: 'dev' }))

// Mock dock for browser requests (UI calls /dock/apps)
app.get('/dock/apps', (_req, res) => res.json(localApps()))

// Mock app API for contacts component (proxies would handle this in production)
// For now the component's .catch() handles the failure gracefully.

app.get('/health', (_req, res) => res.json({ status: 'ok' }))

app.post('/events', (req, res) => {
  const event: BusEvent = req.body
  res.json({ ok: true })
  handleEvent(event, canvas, activeWs, API_KEY).catch(console.error)
})

/* ── WebSocket ────────────────────────────────────────────────────────────── */

wss.on('connection', (ws) => {
  activeWs = ws
  console.log('browser connected')

  ws.send(JSON.stringify({ op: 'canvas', elements: canvas.getState() }))

  if (!sessionStarted) {
    sessionStarted = true
    console.log('firing session.start → Claude')
    handleEvent({ type: 'session.start' }, canvas, ws, API_KEY)
      .catch(console.error)
  }

  ws.on('message', async (raw) => {
    try {
      const event: BusEvent = JSON.parse(raw.toString())
      console.log('event ←', event.type)
      await handleEvent(event, canvas, ws, API_KEY)
    } catch (e) {
      console.error('ws error:', e)
    }
  })

  ws.on('close', () => { if (activeWs === ws) activeWs = null })
})

server.listen(PORT, () => {
  console.log('\n  crimata dev server → http://localhost:' + PORT)
  if (!API_KEY)
    console.warn('  ⚠  ANTHROPIC_API_KEY not set — Claude calls will fail\n')
  else
    console.log('  ANTHROPIC_API_KEY  → set ✓\n')
})
