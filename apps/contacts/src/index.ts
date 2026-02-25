import express from "express"
import { router } from "./routes"

const app = express()
app.use(express.json())
app.use("/contacts", router)

const port = Number(process.env.PORT ?? 3001)
app.listen(port, () => console.log(`contacts app listening on :${port}`))
