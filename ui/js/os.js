/* ── Canvas / WebSocket layer ───────────────────────────────────────────────
   Manages the agent WebSocket connection and renders canvas elements.
   This layer is pure vanilla JS — Alpine handles auth + cursor input.
   ─────────────────────────────────────────────────────────────────────────── */

let ws         = null
let canvasDiv  = null
const elMap    = {}   // id → DOM element

function connectWS() {
  const proto = location.protocol === 'https:' ? 'wss' : 'ws'
  ws = new WebSocket(`${proto}://${location.host}/ws`)

  ws.onopen = () => console.log('agent connected')

  ws.onmessage = (e) => {
    const msg = JSON.parse(e.data)
    switch (msg.op) {
      case 'render':          renderEl(msg.element);              break
      case 'remove':          removeEl(msg.id);                   break
      case 'canvas':          msg.elements.forEach(renderEl);     break
      case 'cursor_response': showCursorResponse(msg);            break
    }
  }

  ws.onclose = () => {
    console.log('agent disconnected — reconnecting in 2s')
    setTimeout(connectWS, 2000)
  }
}

function sendEvent(event) {
  if (ws && ws.readyState === WebSocket.OPEN)
    ws.send(JSON.stringify(event))
}

/* ── Element lifecycle ────────────────────────────────────────────────────── */

function removeEl(id) {
  const div = elMap[id]
  if (div) { div.remove(); delete elMap[id] }
}

function renderEl(el) {
  removeEl(el.id)

  const div = document.createElement('div')
  div.dataset.elId = el.id
  div.classList.add('canvas-element', el.type.replace(/_/g, '-'))
  div.style.left = el.x + 'px'
  div.style.top  = el.y + 'px'

  if (el.type === 'app_icon') {
    buildAppIcon(div, el)
  } else if (el.type === 'window') {
    buildWindow(div, el)
  }

  canvasDiv.appendChild(div)
  elMap[el.id] = div
  makeDraggable(div, el)
}

/* ── App icon ─────────────────────────────────────────────────────────────── */

function buildAppIcon(div, el) {
  const icon  = el.props?.icon  || '📦'
  const label = el.props?.label || el.props?.name || el.component
  const dot   = el.running ? '<div class="icon-dot"></div>' : ''

  div.innerHTML = `
    <div class="icon-glyph">${icon}</div>
    <div class="icon-label">${label}</div>
    ${dot}
  `

  div.addEventListener('click', () => {
    sendEvent({
      type:     'app_icon.clicked',
      data:     { id: el.id, component: el.component },
      position: { x: el.x, y: el.y },
    })
  })
}

/* ── Window (content card) ────────────────────────────────────────────────── */

function buildWindow(div, el) {
  const content = document.createElement('div')
  content.className = 'window-content'
  content.innerHTML = '<div class="loading">Loading…</div>'
  div.appendChild(content)

  // Apply explicit size if provided
  if (el.props?.width)  div.style.width  = el.props.width  + 'px'
  if (el.props?.height) div.style.height = el.props.height + 'px'

  const parts   = el.component.split('.')
  const appId   = parts[0]
  const compName = parts[1] || parts[0]
  const url     = `/components/${appId}/${compName}.html`

  fetch(url)
    .then(r => r.ok ? r.text() : Promise.reject(r.status))
    .then(html => {
      content.innerHTML = html
      // Make props available to component scripts via window scope
      window.__componentProps = el.props || {}
      // Re-execute inline scripts (innerHTML doesn't run scripts)
      content.querySelectorAll('script').forEach(old => {
        const s = document.createElement('script')
        s.textContent = old.textContent
        old.replaceWith(s)
      })
    })
    .catch(() => {
      content.innerHTML = `<div class="loading">${el.component}</div>`
    })

  // Forward data-event button clicks to agent
  div.addEventListener('click', (e) => {
    const btn = e.target.closest('button[data-event]')
    if (btn) {
      sendEvent({
        type: 'button.clicked',
        data: { window: el.id, event: btn.dataset.event, value: btn.dataset.value },
      })
    }
  })
}

/* ── Cursor response ──────────────────────────────────────────────────────── */

function showCursorResponse(msg) {
  removeEl('cursor_response')

  const div = document.createElement('div')
  div.dataset.elId = 'cursor_response'
  div.classList.add('canvas-element', 'cursor-response')
  div.style.left = msg.position.x + 'px'
  div.style.top  = msg.position.y + 'px'

  let html = ''
  if (msg.text) html += `<p>${msg.text}</p>`
  if (msg.pills?.length) {
    html += '<div class="response-pills">'
    msg.pills.forEach(p => { html += `<button data-pill="${p}">${p}</button>` })
    html += '</div>'
  }
  div.innerHTML = html

  div.querySelectorAll('[data-pill]').forEach(btn => {
    btn.addEventListener('click', () => {
      sendEvent({ type: 'pill.clicked', data: { text: btn.dataset.pill } })
      removeEl('cursor_response')
    })
  })

  // Dismiss on outside click
  const dismiss = (e) => {
    if (!div.contains(e.target)) {
      removeEl('cursor_response')
      document.removeEventListener('click', dismiss)
    }
  }
  setTimeout(() => document.addEventListener('click', dismiss), 50)

  canvasDiv.appendChild(div)
  elMap['cursor_response'] = div
}

/* ── Drag ─────────────────────────────────────────────────────────────────── */

function makeDraggable(div, el) {
  let startX, startY, startL, startT

  const handle = div  // drag from anywhere on element

  handle.addEventListener('mousedown', (e) => {
    // Don't initiate drag from interactive elements inside windows
    if (el.type === 'window' && (
      e.target.tagName === 'BUTTON' ||
      e.target.tagName === 'INPUT'  ||
      e.target.tagName === 'A'      ||
      e.target.tagName === 'SELECT'
    )) return

    e.preventDefault()
    startX = e.clientX
    startY = e.clientY
    startL = parseInt(div.style.left) || 0
    startT = parseInt(div.style.top)  || 0

    const onMove = (e) => {
      div.style.left = (startL + e.clientX - startX) + 'px'
      div.style.top  = (startT + e.clientY - startY) + 'px'
    }
    const onUp = () => {
      window.removeEventListener('mousemove', onMove)
      window.removeEventListener('mouseup',   onUp)
    }
    window.addEventListener('mousemove', onMove)
    window.addEventListener('mouseup',   onUp)
  })
}

/* ── Alpine component (auth + cursor input) ─────────────────────────────────
   Keeps Alpine scope minimal — just auth state and cursor input.
   ─────────────────────────────────────────────────────────────────────────── */

const cursorDot = document.getElementById('cursor-dot')

document.addEventListener('alpine:init', () => {
  Alpine.data('os', () => ({

    // Auth
    authed:        false,
    loginUsername: '',
    loginPassword: '',
    loginError:    '',

    // Cursor input
    inputMode:   false,
    cursorQuery: '',
    cursorX:     0,
    cursorY:     0,

    trackCursor(e) {
      this.cursorX = e.clientX
      this.cursorY = e.clientY
      if (cursorDot) {
        cursorDot.style.left = e.clientX + 'px'
        cursorDot.style.top  = e.clientY + 'px'
      }
    },

    async init() {
      canvasDiv = document.getElementById('canvas')
      await this.checkAuth()
      if (this.authed) connectWS()
    },

    async checkAuth() {
      const token = sessionStorage.getItem('auth_token')
      if (!token) return
      try {
        const res = await fetch('/auth/me', {
          headers: { Authorization: `Bearer ${token}` },
        })
        if (res.ok) {
          this.authed = true
        } else {
          sessionStorage.removeItem('auth_token')
        }
      } catch (_) { /* auth service down */ }
    },

    async submitLogin() {
      this.loginError = ''
      try {
        const res  = await fetch('/auth/auth', {
          method:  'POST',
          headers: { 'Content-Type': 'application/json' },
          body:    JSON.stringify({
            username: this.loginUsername,
            password: this.loginPassword,
          }),
        })
        const data = await res.json()
        if (!res.ok || !data.success) {
          this.loginError = data.error || 'Login failed'
          return
        }
        sessionStorage.setItem('auth_token', data.token)
        this.loginPassword = ''
        this.authed = true
        connectWS()
      } catch (_) {
        this.loginError = 'Auth service unreachable'
      }
    },

    handleKeydown(e) {
      if ((e.metaKey || e.ctrlKey) && e.code === 'Space') {
        e.preventDefault()
        this.inputMode ? this.exitInputMode() : this.enterInputMode()
      }
    },

    enterInputMode() {
      this.inputMode   = true
      this.cursorQuery = ''
      this.$nextTick(() => this.$refs.cursorInput?.focus())
    },

    exitInputMode() {
      this.inputMode   = false
      this.cursorQuery = ''
    },

    submitQuery() {
      const text = this.cursorQuery.trim()
      if (!text) { this.exitInputMode(); return }
      sendEvent({
        type:     'cursor.input',
        text,
        position: { x: this.cursorX, y: this.cursorY },
      })
      this.exitInputMode()
    },

  }))
})
