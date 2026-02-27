# Crimata OS

## Install Paths

| Component         | Path                                        |
|-------------------|---------------------------------------------|
| Web desktop UI    | `/opt/crimata/ui/`                          |
| Auth daemon       | `/opt/crimata/bin/crimata-auth`             |
| Dock daemon       | `/opt/crimata/bin/crimata-dock`             |
| Contacts app      | `/usr/lib/crimata-contacts/`               |
| Nginx config      | `/etc/nginx/sites-available/crimata`        |
| systemd units     | `/etc/systemd/system/crimata-*.service`     |
| Postgres DBs      | `crimata_contacts`, `crimata_blog`, …       |

## Ports

| Service            | Port |
|--------------------|------|
| nginx              | 80   |
| crimata-auth       | 7700 |
| crimata-dock       | 7701 |
| crimata-contacts   | 3001 |
