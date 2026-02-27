document.addEventListener('alpine:init', () => {
  Alpine.data('os', () => ({

    // ── Canvas pan state ──────────────────────────────────────────────────
    offset:   { x: 0, y: 0 },
    isPanning: false,
    panStart: { x: 0, y: 0 },

    startPan(e) {
      if (e.target !== document.getElementById('canvas') &&
          !e.target.closest('#canvas') ||
          e.target.closest('.window')) return
      this.isPanning = true
      this.panStart = { x: e.clientX - this.offset.x, y: e.clientY - this.offset.y }
      document.getElementById('canvas').classList.add('panning')
    },

    pan(e) {
      if (!this.isPanning) return
      this.offset = { x: e.clientX - this.panStart.x, y: e.clientY - this.panStart.y }
    },

    endPan() {
      this.isPanning = false
      document.getElementById('canvas').classList.remove('panning')
    },

    // ── Windows ───────────────────────────────────────────────────────────
    windows:   [],
    zCounter:  100,

    openApp(app) {
      // Bring to front if already open
      const existing = this.windows.find(w => w.appId === app.id)
      if (existing) { this.focusWindow(existing); return }

      this.windows.push({
        id:    Date.now(),
        appId: app.id,
        title: app.name,
        url:   app.url,
        x:     120 + Math.random() * 200,
        y:     80  + Math.random() * 100,
        w:     900,
        h:     600,
        z:     ++this.zCounter,
      })
    },

    focusWindow(win) {
      win.z = ++this.zCounter
    },

    closeWindow(win) {
      this.windows = this.windows.filter(w => w.id !== win.id)
    },

    // ── Window drag ───────────────────────────────────────────────────────
    dragging:  null,
    dragStart: { x: 0, y: 0 },

    startDrag(e, win) {
      this.focusWindow(win)
      this.dragging  = win
      this.dragStart = { x: e.clientX - win.x, y: e.clientY - win.y }

      const onMove = (e) => {
        if (!this.dragging) return
        this.dragging.x = e.clientX - this.dragStart.x
        this.dragging.y = e.clientY - this.dragStart.y
      }
      const onUp = () => {
        this.dragging = null
        window.removeEventListener('mousemove', onMove)
        window.removeEventListener('mouseup', onUp)
      }
      window.addEventListener('mousemove', onMove)
      window.addEventListener('mouseup', onUp)
    },

    // ── Cursor input mode ─────────────────────────────────────────────────
    inputMode:   false,
    cursorQuery: '',

    handleKeydown(e) {
      // ⌘+Space or Ctrl+Space toggles input mode
      if ((e.metaKey || e.ctrlKey) && e.code === 'Space') {
        e.preventDefault()
        this.inputMode ? this.exitInputMode() : this.enterInputMode()
        return
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
      const query = this.cursorQuery.trim().toLowerCase()

      // Built-in commands
      if (query === 'login') {
        this.openApp({ id: 'login', name: 'Login', url: '/login', icon: '🔐' })
        this.exitInputMode()
        return
      }

      // Check if it matches an installed app name
      const app = this.installedApps.find(a => a.name.toLowerCase() === query)
      if (app) { this.openApp(app); this.exitInputMode(); return }

      // Otherwise — send to LLM (placeholder)
      console.log('LLM query:', query)
      this.exitInputMode()
    },

    // ── Installed apps ────────────────────────────────────────────────────
    installedApps: [],

    async loadApps() {
      try {
        const res  = await fetch('/dock/apps')
        const data = await res.json()
        this.installedApps = data.map(app => ({
          ...app,
          url: `/apps/${app.id}`,
        }))
      } catch (e) {
        console.error('dock unreachable:', e)
      }
    },

    // ── Init ──────────────────────────────────────────────────────────────
    async init() {
      await this.loadApps()

      // Refresh running status every 10s
      setInterval(() => this.loadApps(), 10_000)

      // Open bio full-screen on load
      const bio = this.installedApps.find(a => a.id === 'bio')
      if (bio) this.openApp(bio)
    }

  }))
})
