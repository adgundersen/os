export interface Contact {
  id: number
  name: string
  domain: string
  avatar_url: string | null
  notes: string | null
  created_at: string
}

export interface CreateContactBody {
  name: string
  domain: string
  avatar_url?: string
  notes?: string
}

export interface UpdateContactBody {
  name?: string
  domain?: string
  avatar_url?: string
  notes?: string
}
