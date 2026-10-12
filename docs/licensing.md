# GrainsDosage — Trial and Licensed editions (macOS)

Both editions include AU and VST3, target Mojave 10.14 on Intel and macOS 11 on
Apple Silicon, and share the same plugin identity and saved sessions. Install
one edition at a time. Trial and Licensed replace each other; do not keep renamed
duplicates installed. The existing development builds remain unrestricted for
internal testing and must not be distributed as commercial builds.

Trial starts when the plugin window is first opened, not when the DAW scans it.
It lasts 14 calendar days on the current macOS user account. The start and last
seen time are stored in Keychain, outside presets, plugin bundles and projects.
Reopening, updating or reinstalling the plugin does not reset that record.
Significant clock rollback disables the trial until the clock is corrected.
After expiry the header says TRIAL EXPIRED and processing fades to dry over
20 milliseconds. The interface and saved settings remain available. A background
check enforces expiration even with the editor closed (within 15 seconds).
No Keychain, filesystem or signature operations occur on the audio thread.

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
The existing signing key is retained; no trial record, project or preset changes.

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
output filename; the reserved slot is retained safely. No payment/email service
or Windows commercial licensing is included.

Trial remains 14 calendar days from the first editor opening, stored in Keychain
outside the plugin and presets. This offline trial is not tamper-proof against
Keychain deletion, binary modification or restoration of a system backup.

## Editable module graphics

On first opening, four independent JPG backgrounds are copied into:

`~/Library/Application Support/GrainsDosage/Skins/Modules/`

- granulizer.jpg — petrol teal
- beatrepeater.jpg — bronze
- reslice.jpg — violet
- gater.jpg — navy blue

Edit those files in any image editor, retain their exact filenames, save and
close/reopen the plugin window. Existing user files are never overwritten by
updates. The supplied images are 2172 × 724 pixels; keep a similar wide aspect
ratio. Backgrounds are cached when opening the editor, not reloaded each frame.
Missing or invalid images fall back to bundled artwork; deleting a custom file
restores the default on the next opening.

JPGs cover the complete module face from its left edge to its right edge, including
the area beneath the rack screws. There is no separate purple frame. JPGs change
each panel's colors, texture and decoration. Knobs, readable text,
meters, buttons, layout and rack screws are separate live UI layers and do not
move or change shape when you replace a JPG. Do not bake text or controls into
these backgrounds. Edits outside the signed plugin bundle preserve its signature.
