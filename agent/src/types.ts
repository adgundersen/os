export type CanvasElement = {
  id:        string
  type:      'app_icon' | 'window' | 'cursor_response'
  component: string           // e.g. "contacts", "contacts.list", "generic.form"
  props?:    Record<string, unknown>
  x:         number
  y:         number
  running?:  boolean
}

export type BusEvent = {
  type:      string
  data?:     Record<string, unknown>
  position?: { x: number; y: number }
  text?:     string
}

export type InstalledApp = {
  id:               string
  name:             string
  icon:             string
  port:             number
  running:          boolean
  defaultComponent: string
  components:       string[]
  api:              ApiEndpoint[]
}

export type ApiEndpoint = {
  name:        string
  description: string
  method:      string
  endpoint:    string
  params?:     Record<string, string>
  body?:       Record<string, string>
}

export type WSMessage =
  | { op: 'render';          element: CanvasElement }
  | { op: 'remove';          id: string }
  | { op: 'canvas';          elements: CanvasElement[] }
  | { op: 'cursor_response'; text?: string; pills?: string[]; position: { x: number; y: number } }
