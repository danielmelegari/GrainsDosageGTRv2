# GrainsDosage — Trial and Licensed editions (macOS)

Both editions include AU and VST3, target Mojave 10.14 on Intel and macOS 11 on
Apple Silicon, and share the same plugin identity and saved sessions. Install
one edition at a time. Trial and Licensed replace each other; do not keep renamed
duplicates installed. The existing development builds remain unrestricted for
internal testing and must not be distributed as commercial builds.

Trial requires a publisher-signed, machine-bound activation. Send the complete
GDDEV2 code to the publisher and paste the GDS2 code from GrainsDosage-Trial-Key.txt.
It lasts 14 calendar days from its signed UTC start (first issuance in the native
generator), regardless of DAW use. The last seen time is protected in Keychain;
the signed interval cannot be extended by reinstalling or deleting local state.
An unsigned first-open record from older versions no longer authorizes a trial.
Significant clock rollback disables the trial until the clock is corrected.
After expiry the header says TRIAL EXPIRED and processing fades to dry over
20 milliseconds. The interface and saved settings remain available. A background
check enforces expiration even with the editor closed (within one second).
No Keychain, filesystem or signature operations occur on the audio thread. The
verified activation is cached; protected trial storage is refreshed every 30 seconds
while a lightweight background clock check runs each second. Transient storage
errors are retried without extending the signed expiry.

Licensed requires a valid device activation before enabling processing. A valid
activation also permanently unlocks Trial. Activation is offline and shared by
AU/VST3 in the same macOS account. Reinstalling on the same Mac or activating its
other user accounts uses the same Device Code and does not consume another slot.
The plugin contains only the RSA public key, verifies SHA-256 signatures, and
matches the signed device hash to this Mac's IOPlatformUUID. The displayed hash
is product-specific; the raw hardware UUID is not included in activation codes.

## Three-device offline licenses (GDS2)

Each customer gets a `GDL2-…` license ID. In ACTIVATE, the customer clicks **Copy
Device Code** and sends the `GDDEV2-…` code plus their license ID to the publisher.
The publisher issues a `GDS2.…` activation for that specific Mac. The customer
pastes it into ACTIVATE. A license ID alone cannot activate a plugin.

The publisher's SQLite ledger allows **at most three distinct devices per
license**, with a transaction protecting concurrent requests. Issuing another
copy for the same device is allowed and consumes no additional slot. A signed
activation for a different Mac is rejected by the plugin. Activation never uses
a server, network call or audio-thread disk/cryptography operation.

These are three lifetime registered devices, not three floating seats: permanent
offline activations cannot be remotely revoked. There is intentionally no
"delete device" command that could free a fourth slot while an old activation
still works. A motherboard replacement can change the code. Hardware spoofing,
cloned virtual hardware or modified binaries are outside the protection this
offline scheme can provide. One authoritative publisher ledger must be used;
restoring an old copy or keeping independent copies can lose the registration
history. Keep current backups of both the key and ledger.

### Previous portable serials

New commercial builds reject GDS1 portable serials because accepting them would
bypass the three-device policy. Exchange any previously delivered GDS1 serial for
one new GDL2 license and device activations. Keep the original customer record to
avoid creating multiple replacement licenses. Previously shipped binaries cannot
be remotely upgraded or revoked and will still accept the old serials.
The existing signing key is retained; projects and presets are unchanged.

## Publisher: issuing activations

Keep `publisher-private.pem` and `licenses.sqlite` private, backed up and outside
Git/customer downloads. Only the publisher should receive the PRIVATE kit. The
key alone is not enough to reconstruct which computers were already activated.
Do not run `init` again with the existing key; it is only for a new product key.

Install Python 3 and `python3 -m pip install cryptography`. From the kit folder:

```sh
# Once per purchased license; save the printed GDL2 ID with the customer record.
python3 license-issuer.py create --private-key publisher-private.pem --customer "Customer 001"

# Replace both placeholders with the complete license ID and customer's Device Code.
python3 license-issuer.py activate --private-key publisher-private.pem --license GDL2_LICENSE_ID --device GDDEV2_DEVICE_CODE --output customer-001-mac1.txt

# Inspect registered devices and the used slots (0/3 to 3/3).
python3 license-issuer.py list --private-key publisher-private.pem --license GDL2_LICENSE_ID
```

The ledger defaults to `licenses.sqlite` beside the private key. Send only the
activation text file to the customer. For their second and third computers, reuse
the SAME license ID with the new Device Code. A fourth device is refused.
If exporting the activation file fails, retry that same device with a valid new
output filename; the reserved slot is retained safely. No payment/email service is included. The native generator also issues machine-bound
Windows permanent licenses for the updated Windows trial build.

Use [GrainsDosage License Generator.app](license-generator.md) on Mojave Intel for
normal trial/permanent issuance with one-time Keychain setup and the same SQLite
ledger. Offline protection cannot prevent binary modification, cloned hardware
identity or restoration of a complete system backup.

## Editable module graphics

The bundled module backgrounds are PNG files: turquoise Granulizer, red Beat
Repeater, purple Reslice and amber Gater. Reslice uses an extended taller texture
matching its 974:770 panel; the other modules use the supplied wide artwork.
Windows embeds all four images as resources. macOS packages them in Resources.
Images are decoded once when opening the editor, not reloaded every frame.

On macOS, optional custom PNGs can be placed in:

`~/Library/Application Support/GrainsDosage/Skins/Modules/`

Use `granulizer.png`, `beatrepeater.png`, `reslice.png` and `gater.png`.
Existing legacy JPG copies are left untouched and no longer override the new
bundled PNGs. Missing or invalid custom PNGs fall back to the bundled artwork.
Close and reopen the editor after changing a custom PNG.

Backgrounds cover the full module face beneath the rack screws. Knobs, text,
meters, buttons, layout and rack screws are separate live UI layers and do not
move or change shape when you replace a PNG. Do not bake text or controls into
these backgrounds. Edits outside the signed plugin bundle preserve its signature.
