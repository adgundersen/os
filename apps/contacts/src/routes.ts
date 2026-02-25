import { Router } from "express"
import { pool } from "./db"
import type { CreateContactBody, UpdateContactBody } from "./types"

export const router = Router()

// List all contacts
router.get("/", async (req, res) => {
  const result = await pool.query(
    "SELECT * FROM contacts ORDER BY name ASC"
  )
  res.json(result.rows)
})

// Get a single contact
router.get("/:id", async (req, res) => {
  const result = await pool.query(
    "SELECT * FROM contacts WHERE id = $1",
    [req.params.id]
  )
  if (result.rowCount === 0) return res.status(404).json({ error: "Not found" })
  res.json(result.rows[0])
})

// Create a contact
router.post("/", async (req, res) => {
  const { name, domain, avatar_url, notes } = req.body as CreateContactBody
  if (!name || !domain) {
    return res.status(400).json({ error: "name and domain are required" })
  }
  const result = await pool.query(
    `INSERT INTO contacts (name, domain, avatar_url, notes)
     VALUES ($1, $2, $3, $4)
     RETURNING *`,
    [name, domain, avatar_url ?? null, notes ?? null]
  )
  res.status(201).json(result.rows[0])
})

// Update a contact
router.patch("/:id", async (req, res) => {
  const { name, domain, avatar_url, notes } = req.body as UpdateContactBody
  const result = await pool.query(
    `UPDATE contacts
     SET name       = COALESCE($1, name),
         domain     = COALESCE($2, domain),
         avatar_url = COALESCE($3, avatar_url),
         notes      = COALESCE($4, notes)
     WHERE id = $5
     RETURNING *`,
    [name ?? null, domain ?? null, avatar_url ?? null, notes ?? null, req.params.id]
  )
  if (result.rowCount === 0) return res.status(404).json({ error: "Not found" })
  res.json(result.rows[0])
})

// Delete a contact
router.delete("/:id", async (req, res) => {
  await pool.query("DELETE FROM contacts WHERE id = $1", [req.params.id])
  res.status(204).send()
})
