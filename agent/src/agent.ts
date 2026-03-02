import Anthropic from '@anthropic-ai/sdk'
import WebSocket from 'ws'
import { Canvas } from './canvas'
import { toolDefinitions } from './tools'
import { BusEvent, CanvasElement, InstalledApp, WSMessage } from './types'

const SERVICE_PORTS: Record<string, number> = {
  auth:  7700,
  dock:  7701,
  agent: 7702,
}

async function fetchApps(): Promise<InstalledApp[]> {
  try {
    const res = await fetch('http://localhost:7701/apps')
    return await res.json() as InstalledApp[]
  } catch {
    return []
  }
}

function appPort(appId: string, apps: InstalledApp[]): number {
  if (SERVICE_PORTS[appId]) return SERVICE_PORTS[appId]
  return apps.find(a => a.id === appId)?.port ?? 3000
}

function buildSystemPrompt(apps: InstalledApp[]): string {
  const appList = apps.map(a => {
    const components = a.components?.length ? a.components.join(', ') : 'none'
    const apiList    = a.api?.map(e => `${e.method} ${e.endpoint} — ${e.description}`).join('; ') ?? ''
    return `  - ${a.id} (${a.name}, port ${a.port}): components=[${components}] api=[${apiList}]`
  }).join('\n')

  return `You are the Crimata desktop agent — an agentic window manager for a personal Linux instance.
You control a spatial canvas UI by calling tools. Think of yourself as both the OS shell and a helpful assistant.

Installed apps:
${appList || '  (none)'}

Component naming: "app.component", e.g. "contacts.list", "contacts.card"
App icons use type "app_icon", windows use type "window".

Layout rules:
- Place app icons spread naturally across the canvas (not all in one corner)
- Open windows adjacent to their app icon
- Keep the layout clean and uncluttered

Behaviour:
- On session.start: render all installed app icons on the canvas, then open each app's defaultComponent
- On app_icon.clicked: render the app's default window next to the icon
- On cursor.input: interpret naturally — open apps, answer questions, rearrange canvas, call APIs
- On button.clicked or api events: decide whether to respond or stay silent
- Only respond when you have something useful to do — silence is fine`
}

export async function handleEvent(
  event: BusEvent,
  canvas: Canvas,
  ws: WebSocket | null,
  apiKey: string,
): Promise<void> {
  const apps  = await fetchApps()
  const model = process.env.CLAUDE_MODEL || 'claude-haiku-4-5-20251001'

  const client = new Anthropic({ apiKey })

  const canvasJson = JSON.stringify(canvas.getState(), null, 2)
  const userMsg    = `Canvas:\n${canvasJson}\n\nEvent: ${JSON.stringify(event)}`

  const messages: Anthropic.MessageParam[] = [
    { role: 'user', content: userMsg },
  ]

  // Agentic loop — keep going until Claude stops calling tools
  while (true) {
    const response = await client.messages.create({
      model,
      max_tokens: 2048,
      system:     buildSystemPrompt(apps),
      tools:      toolDefinitions,
      messages,
    })

    const toolUses = response.content.filter(b => b.type === 'tool_use')
    if (toolUses.length === 0) break

    const toolResults: Anthropic.ToolResultBlockParam[] = []

    for (const block of response.content) {
      if (block.type !== 'tool_use') continue

      const input = block.input as Record<string, unknown>
      let   result: unknown = { ok: true }

      switch (block.name) {
        case 'render': {
          const el = input as unknown as CanvasElement
          canvas.render(el)
          send(ws, { op: 'render', element: el })
          break
        }

        case 'remove': {
          const id = input.id as string
          canvas.remove(id)
          send(ws, { op: 'remove', id })
          break
        }

        case 'call_api': {
          try {
            const port = appPort(input.app as string, apps)
            const url  = `http://localhost:${port}${input.endpoint as string}`
            const res  = await fetch(url, {
              method:  input.method as string,
              headers: { 'Content-Type': 'application/json' },
              body:    input.body ? JSON.stringify(input.body) : undefined,
            })
            result = await res.json()
          } catch (e: unknown) {
            result = { error: String(e) }
          }
          break
        }

        case 'show_cursor_response': {
          send(ws, {
            op:       'cursor_response',
            text:     input.text as string | undefined,
            pills:    input.pills as string[] | undefined,
            position: input.position as { x: number; y: number },
          })
          break
        }
      }

      toolResults.push({
        type:        'tool_result',
        tool_use_id: block.id,
        content:     JSON.stringify(result),
      })
    }

    messages.push({ role: 'assistant', content: response.content })
    messages.push({ role: 'user',      content: toolResults })

    if (response.stop_reason !== 'tool_use') break
  }
}

function send(ws: WebSocket | null, msg: WSMessage): void {
  if (ws && ws.readyState === WebSocket.OPEN) {
    ws.send(JSON.stringify(msg))
  }
}
