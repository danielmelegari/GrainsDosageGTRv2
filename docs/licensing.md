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

Licensed requires a valid serial before enabling processing. Click ACTIVATE in
the header and paste the whole GDS1 serial. A valid serial also permanently
unlocks Trial. Activation is offline, portable, and shared by AU/VST3 in the
same account. There is no device limit, online revocation or remote reset.
The plugin contains only an RSA public key and verifies SHA-256 signatures.
As with any fully offline trial, this is not tamper-proof against a user who
changes the binary, deletes Keychain records or restores an entire system backup.

## Publisher: issuing serials

Keep the publisher-private.pem key private and backed up. Never put it in Git,
the plugin, a customer download or a preset. Losing it prevents issuing new
serials for this public key. The supplied publisher kit includes your key;
only you should receive that kit. The public key alone is compiled into the app.

Install Python 3 and `python3 -m pip install cryptography`, then:

```sh
python3 license-issuer.py issue --private-key publisher-private.pem --output customer-001.txt
```

Send only customer-001.txt to that customer. Every run creates a unique serial.
Do not run `init` again for routine issuing: replacing the compiled public key
would invalidate existing customers' licenses. No payment or email service is
connected automatically.

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

JPGs change each panel's colors, texture and decoration. Knobs, readable text,
meters, buttons, layout and rack screws are separate live UI layers and do not
move or change shape when you replace a JPG. Do not bake text or controls into
these backgrounds. Edits outside the signed plugin bundle preserve its signature.
