# DSGMachines GrainsDosage Licensing API

Production target: Cloudflare Worker `dsgmachines-license-api` with D1 binding `DB` to `dsgmachines-licenses`.

## State of delivery

Backend implementation is included in `worker.js`. Initial worker health endpoint was deployed previously. **This source is not yet a tested production deployment; do not ship the VST3 relying on it until Worker secrets are installed, the full Worker source is deployed and tested.**

Configure two Worker secrets (never commit or copy into plugin):
- `SIGNING_PRIVATE_KEY`: PEM PKCS#8 P-256 ECDSA key (generate separately, store safely)
- `MACHINE_PEPPER`: strong random unique value used to hash license keys and machine identifiers

Generate keys locally (Python cryptography):
```python
from cryptography.hazmat.primitives.asymmetric import ec
from cryptography.hazmat.primitives import serialization
import secrets
key=ec.generate_private_key(ec.SECP256R1())
print(key.private_bytes(serialization.Encoding.PEM,serialization.PrivateFormat.PKCS8,serialization.NoEncryption()).decode())
print(key.public_key().public_bytes(serialization.Encoding.PEM,serialization.PublicFormat.SubjectPublicKeyInfo).decode())
print(secrets.token_hex(32))
```
Store private key in Cloudflare Worker secret; embed **only public key** in VST3. The included public-key.pem will only be useful if you provision the matching private key, which is not distributed with this package. Never rotate the signing key without supporting previously issued certificates and never change the pepper without a migration.

Deploy after secrets configured, from directory containing worker.js and wrangler.toml:
`npx wrangler deploy`

Apply `schema-additions.sql` to production D1 using Wrangler CLI or SQL console before enabling device replacement.

## API

`POST /v1/activate` and `POST /v1/verify` with JSON:
```json
{"license_key":"CUSTOMER_GUMROAD_LICENSE","machine_id":"STABLE_MACHINE_IDENTIFIER","plugin_version":"1.0.0"}
```
`POST /v1/replace` with additional `old_machine_id` and new `machine_id`.

Return JSON with `certificate` (compact ES256 signed token) and `expires_at` (exactly six calendar months from the server verification). These requests always verify purchase validity with Gumroad and set `increment_uses_count=false`.

Codex/VST3 must persist the user's key securely (OS keychain/credential manager), sign-cert public key, certificate, and stable machine ID; validate signature, issuer, expiration, allowed major version and machine hash. The certificate `machine_hash` contains SHA-256 of the raw canonical machine ID. Verify it matches the locally computed SHA-256 of the same canonical ID. D1 stores a separate peppered hash. Never embed `MACHINE_PEPPER` in the plugin.

Six-month checks are automatic; customer enters the key only once. A failed Gumroad API check must not mint a certificate. At certificate expiry and offline, require internet to resume. License remains perpetual for major version 1.x, no payment renewal. New major version 2.0 is denied.

IMPORTANT: Add abuse rate limiting (Turnstile not suitable inside a VST plugin), verify `/v1/replace` authorization, concurrency tests, and refund/chargeback test vectors before public release. An attacker with a stolen license key can impersonate a customer's device; prefer signing each machine request with a device-local asymmetric keypair.
