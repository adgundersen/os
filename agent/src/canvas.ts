import { CanvasElement } from './types'

export class Canvas {
  private elements = new Map<string, CanvasElement>()

  render(element: CanvasElement): void {
    this.elements.set(element.id, element)
  }

  remove(id: string): void {
    this.elements.delete(id)
  }

  update(id: string, patch: Partial<CanvasElement>): void {
    const el = this.elements.get(id)
    if (el) this.elements.set(id, { ...el, ...patch })
  }

  getState(): CanvasElement[] {
    return Array.from(this.elements.values())
  }

  clear(): void {
    this.elements.clear()
  }
}
