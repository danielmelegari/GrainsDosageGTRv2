# DSGMachines certificate licensing — integration branch

This service supersedes the proposed `../license-worker` service for the new
six-month certificate protocol. Do not deploy the older service against this
D1 database: its schema and perpetual RSA activation policy are different.

## Deployment status — 18 October 2026

- Full Worker deployed as `dsgmachines-license-api`.
- D1 `dsgmachines-licenses`, ID `c1ba368a-eab8-444d-b86e-599e5811303a`.
- Endpoint: https://dsgmachines-license-api.danielmelegarii.workers.dev
- Deployment ID: `8fc0fda5c81f461484f7c9807df29ac7`.
- `SIGNING_PRIVATE_KEY` and `MACHINE_PEPPER` provisioned directly as Worker secrets;
  private values were not returned to chat or written to this repository.
- Public key: `public-key.pem`, key ID `dsg-es256-9786202924645f18`.
  The handoff public key was replaced because its private counterpart was not
  provided, the Worker had no secrets, and D1 had no issued licenses/activations.
  The plugin's existing RSA key and legacy activations are untouched.
- `schema-hardening.sql` applied to the inspected production schema. Apply ONCE,
  not on each deploy. `schema.sql` is only for a clean staging database.
- Rate limits bound: 20 requests/minute/source IP and 10/minute/purchase-key hash.
- 14 local tests pass, including real workerd execution, real local D1 bindings,
  ECDSA signing, replacement transactions and rate limiting with mocked Gumroad.
- Wrangler deployment dry run passed. Production settings/secrets verified via API.

**Not release-ready yet:** this execution session blocks outbound requests to the
workers.dev hostname and api.gumroad.com. A live successful Gumroad purchase has
not been tested. Add a valid test purchase key as the Worker secret
`GUMROAD_TEST_LICENSE_KEY` using Cloudflare's dashboard, and enable HTTPS access
to the Worker hostname for the test environment. The current production handler
does not read this test secret or have a test-key bypass. Test tooling must not
expose it or replace the real Gumroad verifier with a production mock.

No VST3/source GUI/audio changes have been made. Per the requested sequencing,
backend live testing must finish before plugin integration and release changes.

## Entitlement and storage

- Permanent ownership of GrainsDosage **major version 1 only**.
- No subscription and no payment renewal at six months.
- Offline certificate expires after exactly six UTC calendar months; dates at
  month end clamp to the last valid day (Aug 31 -> Feb 28/29).
- Two simultaneously active machines and one lifetime replacement, as in handoff.
- Every activate/verify/replace verifies the purchase with Gumroad without
  incrementing Gumroad's usage counter. Refunds, disputes, chargebacks, wrong
  products and subscriptions are refused. Provider outages issue no certificate.
- At expiry, an offline client must reconnect; an unexpired cached certificate
  can continue working during a transient verification outage.
- Replacement denies renewal/reactivation on the old device. Its previously
  issued certificate remains usable offline until its expiration. Six-month
  offline certificates cannot provide immediate offline revocation.
- Store only public verification keys in the plugin. Keep the purchase key and
  device private key in Keychain/Credential Manager. D1 stores HMAC identifiers
  and public device keys; it never stores the raw Gumroad key or raw machine ID.
- The pepper and signing private key must survive redeploys. Do not rotate either
  without a planned migration/key-ring rollout. Restrict deployment privileges.
  Cloudflare secret values cannot be read back through the API; no external
  private-key backup was created by this deployment. Preserve this Worker.

## Protocol 2 — authenticated device requests

`POST /v1/activate`, `/v1/verify`, `/v1/replace`, application/json, <=8192 bytes.
All requests include:

```
{
  "license_key": "GUMROAD-KEY",
  "machine_id": "64 lowercase hex (or GDDEV2- plus those 64 hex)",
  "plugin_version": "1.0.0",
  "device_public_key": "base64url DER SPKI P-256",
  "timestamp": 1792281600,
  "nonce": "32 lowercase random hex",
  "signature": "base64url 64-byte P1363 ECDSA-SHA256 signature"
}
```

The canonical signed bytes are UTF-8 JSON.stringify of this ARRAY, with no spaces:

```
["DSGMachines-request-v1",path,license_key,machine_id,plugin_version,
 device_public_key,timestamp,nonce,old_machine_id_or_empty_string]
```

Keys and IDs must already be trimmed and canonical. The timestamp must be within
five minutes of server UTC. Retries are idempotent by license/device identity;
short-window replay cannot consume another slot or replacement. The nonce binds
request signatures but is not a persistent replay blacklist.

`/verify` only renews an existing active device. `/activate` pins the supplied
public key on first registration. A different public key cannot take over an
existing machine ID. First registration still relies on possession of the
Gumroad key: stolen purchase keys remain a risk before device slots are filled.

Replacement also includes `old_machine_id` and `old_signature`, signed over the
same canonical bytes by the OLD device key. The new signature proves possession
of the new key. D1 atomically revokes the old activation, creates the new one,
consumes the replacement and writes the event. Duplicate retries do not consume
another replacement. If the old device/key is lost, this endpoint deliberately
refuses replacement: a separate publisher-verified recovery process is required.
Do not turn knowledge of an old machine ID into replacement authorization.

Certificates are compact JWS with ES256/P1363 signatures and a pinned `kid`.
Required claims: `iss=DSGMachines`, `aud=GrainsDosage`, `product=GrainsDosage`,
`major=1`, `perpetual=true`, license/activation IDs, `machine_hash` (SHA-256 of
canonical machine ID), `device_key_hash` (SHA-256 of base64url SPKI), `iat`, `exp`,
`issued_at`, `expires_at`. Validate these AFTER signature verification and reject
unknown algorithms/keys, malformed/duplicate claims, future-issued/expired tokens,
wrong machines, wrong device keys and major version 2. Never infer entitlement
from `perpetual` without the other checks.

## Plugin integration gate and design

After the backend passes live tests:

1. Add a commercial-build-only client behind an OFF-by-default CMake option on
   this isolated branch. Preserve existing offline licenses and Trial behavior.
2. Keep audio-thread checks atomic and cheap. No HTTP, certificate parsing,
   cryptography, disk I/O or mutex acquisition in process().
3. Use bounded, cancellable networking on a background worker; automatic renewal
   near expiry and bounded retry/backoff after network failures. Never start one
   permanent poller per plugin instance. Shutdown must join/cancel deterministically.
4. Persist a validated certificate atomically and keys using OS secure storage.
   Store monotonic/wall-clock observations to detect obvious clock rollback;
   do not promise offline protection against complete machine snapshot restoration.
5. Reuse the existing activation dialog; keep the audio engine and GUI design.
   Client supports the signed-device protocol above and only embeds public keys.
6. Test expiry, tampering, product/version/device mismatch, renewal failures,
   offline restarts, multiple instances, unloading, suspend/resume, reinstall and
   replacement. Run native Mojave-compatible Mac and Windows tests before enabling
   the option for any released VST3. Linux-only crypto tests are not native proof.

## Development

```
npm ci
npm test
npm run check
```

The runtime test uses Miniflare's installed V4 compatibility conversion with
Miniflare 5. Tests generate temporary keys in memory and mock only the external
Gumroad response. They do not contact Gumroad or mint real customer activations.

Production main module is worker.js; implementation exports stay in api.js to
avoid exposing constants as Worker entrypoints. Requests use redirect:'manual'
and explicitly refuse redirects: Workers does not support redirect:'error'.

The deployment was performed through the authorized Cloudflare connector, not
Wrangler login. Future uploads must preserve existing secret_text bindings.
Do not rerun the one-time SQL migration blindly; inspect schema first.
