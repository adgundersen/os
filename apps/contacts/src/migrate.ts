import { pool } from "./db"

async function migrate() {
  await pool.query(`
    CREATE TABLE IF NOT EXISTS contacts (
      id         SERIAL PRIMARY KEY,
      name       TEXT NOT NULL,
      domain     TEXT NOT NULL UNIQUE,
      avatar_url TEXT,
      notes      TEXT,
      created_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
    )
  `)
  console.log("contacts: migration complete")
  await pool.end()
}

migrate().catch((err) => {
  console.error("contacts: migration failed", err)
  process.exit(1)
})
