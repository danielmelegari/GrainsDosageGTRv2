# GrainsDosage activation on Cloudflare Workers + D1

A separate activation website for the **Licensed edition**, using automatic
Gumroad verification and the existing offline Mac/Windows license formats.
No plugin rebuild, background polling or audio-thread networking is introduced.
The automatic 14-day Trial remains independent.

## Customer flow

1. Purchase the configured permanent product through Gumroad.
2. Open the Licensed plugin and copy its Device Code / Machine ID.
3. Open your Worker URL and enter the Gumroad license key and Device Code.
4. Download the returned TXT or paste the complete code into the plugin.
5. Subsequent audio use is offline. Up to three distinct computers per purchase,
   counting Windows and Mac together; retries do not consume another slot.

This first version supports one product and one-time purchases, not subscriptions.
No seller access token is needed for Gumroad's license-verification endpoint.

## Deployment (publisher's computer)

Requirements: Node 22+ (Node 24 for tests), a Cloudflare account with Workers/D1,
your exact Gumroad product ID, and the EXISTING publisher RSA signing key.

```
cd services/license-worker
npm ci
npm test
npx wrangler login
npx wrangler d1 create dsgmachines-licenses
```
Your configured database is `dsgmachines-licenses` with ID
`c1ba368a-eab8-444d-b86e-599e5811303a`; skip creation for this existing database. Set `database_id` and
`database_name` in `wrangler.jsonc` to that database's exact values. Set
`GUMROAD_PRODUCT_ID` to the exact ID copied from Gumroad (including trailing `=`).
Do not use the product URL slug, sample license key or Cloudflare account ID.

```
npx wrangler d1 migrations apply dsgmachines-licenses --remote
```
Before enabling customers, complete the existing-customer migration below.
The migration creates new tables; do not apply it to an unrelated database.

Configure `SIGNING_PRIVATE_KEY` as a **Worker secret**, not a D1 row, GitHub
variable, source file or public `vars` entry. It must be the existing RSA-2048
private key in unencrypted PKCS#8 PEM form. The Worker verifies every new
signature against the public key pinned in `src/public-key.js`; a mismatched
key fails closed. Existing plugin public keys are unchanged.

On your Mac, use the publisher generator's encrypted private-key backup if
needed. Decrypt/convert locally and pipe directly to Wrangler without saving an
unencrypted file (OpenSSL prompts locally for the encrypted backup password):

```
openssl pkcs8 -topk8 -nocrypt -in /secure/path/publisher-backup.pem | npx wrangler secret put SIGNING_PRIVATE_KEY
npm run check
npm run deploy
```
Never send the backup, password, signing key or Cloudflare API token in chat.
A Worker secret is encrypted storage but is accessible to Worker code at runtime:
protect deployment/account access with MFA and restricted roles. D1 is only the
ledger; it cannot sign licenses by itself. Cloudflare deployment credentials must
permit Worker deployment, D1 operations and secret configuration on your account.

Use the HTTPS workers.dev URL printed by Wrangler, or attach a custom domain.
The activation page and script are served by the Worker; no Pages project needed.
The rate-limit binding allows 10 requests per minute per source IP (Cloudflare's
rate limiter is approximate, not the device-limit authority). Missing binding
fails closed. Apply account WAF/rate limits as appropriate for public traffic.

## Existing customers — required before go-live

Do not create a fresh independent allowance for a purchase already activated
with the offline generator. That would permit three additional devices.
Before opening the site, map each existing Gumroad purchase to its original
GDL2 license ID and import all its previously issued device records/codes into
D1. `licenses` requires: original `id`, Gumroad `sale_id`, exact `product_id`,
SHA-256 hex of the trimmed Gumroad `license_key`, and `status='active'`.
`activations` requires: original `license_id`, exact `device` and the complete
existing signed `code` (Mac GDS2 or Windows five-line document).
The old generator's ledger has IDs/devices but does NOT store the signed codes;
use previously saved customer TXT files or re-export for the same devices from
the original generator before migration. Keep its original ledger and key intact.

D1 becomes authoritative for migrated purchases. Do not keep issuing additional
devices for those purchases through an independent offline ledger. Missing keys,
purchase mappings or TXT files require reconciliation before that customer uses
the website. This package does not automatically import customer records.
For a launch with no previous paid activations, no customer import is necessary.

## Operations and limits

- Each activation, including repeat downloads, re-verifies the purchase with
  Gumroad (`increment_uses_count=false`, eight-second timeout).
- Rejects wrong product, refunded/disputed/charged-back purchases, subscriptions
  and malformed verification replies. No Gumroad webhook alone grants access.
- D1 triggers enforce three devices atomically, even across concurrent requests.
  Codes are returned only after a slot is committed. Inserts are idempotent.
- Raw purchase keys and customer emails are not stored in D1. Device identifiers,
  purchase references and signed codes are stored; restrict database access and
  maintain your privacy policy accordingly. No request-body logging is enabled.
- To block future activations, set `licenses.status='blocked'` for the purchase
  through the Cloudflare D1 console. To restore it, use `'active'`.
- Blocking/refunding does NOT revoke an already issued permanent offline code.
  There is deliberately no delete-device endpoint. SQL triggers prevent deletion
  or modification of device rows, avoiding accidental release of used slots.
- Back up D1 and use its recovery facilities. Never recreate an empty ledger or
  restore an old snapshot and continue issuing without reconciling later grants.
- Do not change the signing key without a planned public-key rollout to plugins.
- Existing build-time Windows public-key overrides must match the Mac public key;
  test the actual shipped binaries, not just the source defaults.

## Validation before sharing publicly

Local tests use real RSA signatures and SQLite with the shipped SQL triggers,
including concurrent calls, duplicate retries, four-device attempts, purchase
rejection, blocked licenses, signing-key mismatch and database-write failures.
They simulate the D1 interface and Gumroad, not a deployed service.

After deploying to staging, test a real eligible purchase and refund handling,
repeat activation, three mixed Mac/Windows devices, refusal of a fourth, and
paste/import on the actual Licensed AU/VST3 builds. Validate the activation site
on Safari on Mojave as well as current browsers. Do not call the service ready
for customers until these live checks and any ledger migration pass.

No GitHub workflow is added or triggered by this service package.
