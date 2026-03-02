import Anthropic from '@anthropic-ai/sdk'

export const toolDefinitions: Anthropic.Tool[] = [
  {
    name: 'render',
    description: 'Place or update a component on the canvas. Use stable IDs so re-renders update in place.',
    input_schema: {
      type: 'object' as const,
      properties: {
        id:        { type: 'string', description: 'Unique stable ID, e.g. "contacts-list", "mail-inbox"' },
        type:      { type: 'string', enum: ['app_icon', 'window'] },
        component: { type: 'string', description: 'App component, e.g. "contacts.list", "contacts.card"' },
        props:     { type: 'object', description: 'Data to pass to the component' },
        x:         { type: 'number', description: 'Canvas X position in pixels' },
        y:         { type: 'number', description: 'Canvas Y position in pixels' },
        running:   { type: 'boolean', description: 'Whether the app is running (for app_icon type)' },
      },
      required: ['id', 'type', 'component', 'x', 'y'],
    },
  },
  {
    name: 'remove',
    description: 'Remove a component from the canvas.',
    input_schema: {
      type: 'object' as const,
      properties: {
        id: { type: 'string' },
      },
      required: ['id'],
    },
  },
  {
    name: 'call_api',
    description: 'Call a local app or service API on this instance. Returns the JSON response.',
    input_schema: {
      type: 'object' as const,
      properties: {
        app:      { type: 'string', description: 'App ID or service: "contacts", "auth", "dock"' },
        endpoint: { type: 'string', description: 'Path, e.g. "/contacts" or "/contacts/123"' },
        method:   { type: 'string', enum: ['GET', 'POST', 'PUT', 'PATCH', 'DELETE'] },
        body:     { type: 'object', description: 'Request body for POST/PUT/PATCH' },
      },
      required: ['app', 'endpoint', 'method'],
    },
  },
  {
    name: 'show_cursor_response',
    description: 'Show a text response or action pills near the cursor. Use for replying to user input.',
    input_schema: {
      type: 'object' as const,
      properties: {
        text:     { type: 'string', description: 'Text to display' },
        pills:    { type: 'array', items: { type: 'string' }, description: 'Action buttons to show' },
        position: {
          type: 'object',
          properties: {
            x: { type: 'number' },
            y: { type: 'number' },
          },
          required: ['x', 'y'],
        },
      },
      required: ['position'],
    },
  },
]
