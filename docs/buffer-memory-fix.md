# Buffer allocation and Freeze fix

The Granulizer's 1/2/4/8/16-second selector previously rebuilt two vectors in
`Engine::set`, which runs on the audio thread. It copied the recording history,
freed the old storage and rebased voice positions during playback.

The engine now allocates a fixed 16-second grain ring in `prepare`. The selector
changes its logical read window without allocating, copying or rebasing audio.
Growing the window reveals retained history. Freeze stops the recording counter
as well as recording; changing the window while frozen stays anchored to the
same captured audio, even after waiting longer than the physical ring length.

Removed the unused stereo glitch bank. Legacy parallel Beat Repeater reuses the
serial Beat Repeater's history bank; these paths cannot run simultaneously.
Switching between them resets capture validity so stale shared storage is not
played. The 96-second Reslice history remains necessary for a previous 48-second
window at 20 BPM while the next window records.

Standalone Linux allocator measurements (engine only, no editor or Cubase),
with a 4-second grain selection:

| Sample rate | Before, MiB | After, MiB |
| --- | ---: | ---: |
| 48 kHz | 79.925 | 72.591 |
| 96 kHz | 159.240 | 144.580 |
| 192 kHz | 317.866 | 288.557 |

The new regression checks no heap allocations or releases during buffer/routing
changes, all 120 routings plus legacy paths, wrapped history, long Freeze holds,
frozen window expansion, shared-bank validity and prepared memory budgets at
44.1/48/96/192 kHz. Actual Cubase memory and GUI performance require host testing.
The current skin and GUI refresh rate are unchanged by this fix.
