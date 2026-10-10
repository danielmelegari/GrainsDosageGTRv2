# Keyboard values, MOD W rates and preset banks

Click a numeric readout to type its value. Right-click any control to enter its
value or exact option label, including menu controls and switches. Apply commits
one host edit gesture; Cancel leaves the value unchanged. Numeric entry accepts
the displayed units and decimal commas. Invalid or out-of-range input leaves the
dialog open. Double-clicking a knob still restores its default.

MOD W now includes 2/1 and 4/1: 8 and 16 host quarter-note beats, respectively.
The appended rate selectors default to “Saved rate”, which follows the previous
MOD W parameter. This preserves old presets, sessions and automation IDs. Choose
an explicit rate (or Off) to override it. State format 0x51473149 appends the four
new selectors; format 0x51473148 and earlier remain readable.

The preset menu installs three banks and upgrades untouched factory files from the previous release. User-edited files are preserved:

- **Legacy:** the current 128 routing presets, with the Mix revision below.
- **Circuit Fractures:** 128 original glitch presets inspired by Richard Devine's
  intricate experimental sound design. Eight families explore micro-slices,
  irregular repeats, reverse fragments, gates, resonances and spatial effects.
- **Forest Escape:** 128 original slower, more harmonic and organic presets
  inspired by the atmosphere of Parvati Records. Uses longer grains, musically
  related pitch intervals, restrained chaos, slow LFOs and spacious reverbs.

Each bank contains eight categories of sixteen presets and covers all 24
four-module routings. Presets process incoming audio and follow the host tempo.
Downloads are separate ZIP packages with a category/routing catalogue.

The approved skin, audio buffer optimizations and GUI refresh rate are retained.
Knob label/face/readout spacing has increased; each module number occupies its
own badge and the drag handle sits separately on the right of the title.

All 384 presets set Granulizer, Beat Repeater, Reslice and Reverb Mix strictly
to 100% when enabled and 0% when disabled. Gater has no separate Mix control.
Preslicer is retired; its parameter IDs stay reserved so old sessions remain
readable. Its factory patches are revoiced for the remaining four modules.

Reslice now has only CutDSG, WarpDSG and PushDSG. Every bank includes all three
algorithms, with slower and gentler settings in Forest Escape and denser cutting
in Circuit Fractures. Previous untouched factory files upgrade automatically;
user-edited presets are preserved. Older Reslice sessions load with CutDSG and
sensible new-control defaults; their sound changes because the manual sequencer
has been replaced at the user's request.

Beat Repeater Random changes all sixteen step divisions. Granulizer Random
centres Pan, selects Manual pan mode and randomizes its own controls, including
buffer, Density Flow and Freeze, while respecting Mix locks. Master Options are
not changed by Granulizer Random.
