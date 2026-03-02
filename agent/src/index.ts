import express from 'express'
import { WebSocketServer, WebSocket } from 'ws'
import { createServer } from 'http'
import { Canvas } from './canvas'
import { handleEvent } from './agent'
import { BusEvent } from './types'

const PORT    = 7702
const API_KEY = process.env.ANTHROPIC_API_KEY ?? ''

const app    = express()
const server = createServer(app)
const wss    = new WebSocketServer({ server, path: '/ws' })
const canvas = new Canvas()

let activeWs:      WebSocket | null = null
let sessionStarted = false

app.use(express.json())

app.get('/health', (_req, res) => res.json({ status: 'ok' }))

// POST /events — server-side services emit events to the agent
app.post('/events', async (req, res) => {
  const event: BusEvent = req.body
  res.json({ ok: true })
  handleEvent(event, canvas, activeWs, API_KEY).catch(console.error)
})

wss.on('connection', (ws) => {
  activeWs = ws
  console.log('browser connected')

  // Send current canvas state immediately
  ws.send(JSON.stringify({ op: 'canvas', elements: canvas.getState() }))

  // Fire session.start on first browser connection to initialise canvas
  if (!sessionStarted) {
    sessionStarted = true
    handleEvent({ type: 'session.start' }, canvas, ws, API_KEY).catch(console.error)
  }

  ws.on('message', async (raw) => {
    try {
      const event: BusEvent = JSON.parse(raw.toString())
      await handleEvent(event, canvas, ws, API_KEY)
    } catch (e) {
      console.error('ws message error:', e)
    }
  })

  ws.on('close', () => {
    if (activeWs === ws) activeWs = null
    console.log('browser disconnected')
  })
})

server.listen(PORT, () => {
  console.log(`crimata-agent listening on :${PORT}`)
})
