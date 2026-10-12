# Registered-computer offline trials

The Windows trial requires a publisher-signed license for that computer. No server,
account, or internet connection is needed by the plugin or registration tool.
Without a valid license the trial plugin silences output. The full edition remains
unrestricted; distribute only the trial installer to evaluation users.

## Publisher setup (once, on your own computer)

Install Python 3 and the signing dependency:

```text
python -m pip install cryptography
```

From a checkout of this repository, generate a private key outside the repository:

```text
python tools/issue_trial_license.py keygen --key ../grains-trial-private.pem --public-header ../grains-trial-public.h
```

Back up `grains-trial-private.pem` securely. Keep it on your issuing computer;
never commit it, send it to customers, or put it in GitHub Actions. Losing it means
you cannot issue licenses for builds signed by its public key. Replacing the key
requires rebuilding the trial and reissuing licenses.

The command also creates `grains-trial-public.b64`. In the GitHub repository,
Settings > Secrets and variables > Actions > Variables, create the repository
variable `GRAINS_TRIAL_PUBLIC_KEY` containing that file's text. It is a PUBLIC key,
not a secret. Run **Windows 14-day trial** manually after setting the variable.
Only the public key is embedded in the plugin and registration tool.

For a local build, pass `-DGRAINS_TRIAL=ON` and
`-DGRAINS_TRIAL_PUBLIC_KEY_HEADER=/absolute/path/to/grains-trial-public.h` to CMake.
There is deliberately no default production signing key. CI can run licensing tests
with a separate test verification key, but never publishes those test builds.

## Register a computer and issue its trial

1. The customer installs the trial, opens **GrainsDosage Trial Registration** from
   the Start menu, and chooses **Yes**. This saves
   `GrainsDosage-machine-request.json` on their Desktop.
2. They send that request to you. Decide whether this computer should receive a
   trial. Keep your own registration records so you do not issue repeated trials.
3. Issue the license on your private issuing computer:

   ```text
   python tools/issue_trial_license.py issue --key ../grains-trial-private.pem --request GrainsDosage-machine-request.json --out trial.license
   ```

   This creates a fixed 14-day period starting when the license is issued.
   To choose a start date, add `--start 2026-11-01T12:00:00Z` (UTC), or an ISO date
   with an explicit timezone offset. The expiry is always exactly 14 days later.
4. Send only `trial.license` to that customer. They open the registration tool,
   choose **No**, select the license, and reload the plugin. The tool displays
   the start and expiry dates in UTC. A future-dated license can be installed
   early but does not enable audio before its start date.

The machine ID hashes the Windows MachineGuid and system volume serial. The
request contains only that hash. Reinstalling Windows or changing the system
volume can change the ID and require a new license. The license is installed in
`%LOCALAPPDATA%\GrainsDosage-Trial\trial.license`; other Windows accounts on the same
computer can install that same license and retain the same signed expiry date.
Installing the trial elsewhere is possible, but it will not process audio with a
license whose machine ID does not match.

## Protection and limits

RSA-2048/SHA-256 signatures protect the machine ID and start/expiry dates. Protected
state uses Windows DPAPI with copies in the current user's registry and local
application data. Missing/corrupt single copies, inconsistent starts, and clock
rollback beyond two minutes block the trial. Small clock corrections do not move
the recorded time backwards. A monotonic deadline also limits each open session.
Storage, signature checks, and DPAPI run off the audio thread. Every 30 seconds the
plugin rechecks the license, identity, clock, and state; expiry is checked on each
audio block. A detected failure stays blocked for that plugin instance.

Deleting both state copies does not change the signed expiry. Reinstalling the
plugin does not reset it. Existing unsigned trials must register for a signed
license after upgrading. Uninstalling does not remove registration state.

Offline protection is tamper-resistant, not tamper-proof. A determined administrator
can patch the binary, spoof the identifiers, restore a whole machine snapshot, or
restore all state and manipulate its clock. No purely offline mechanism can prove
real elapsed calendar time against those attacks. DPAPI is protection against
casual file/registry edits, not against code running as the same Windows user.
