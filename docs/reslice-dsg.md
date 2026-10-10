# Procedural Reslice

These are original algorithms inspired by the generative breakbeat-cutting
concept described for BBCut/LiveCut. They do not reproduce or incorporate those
projects' source code and are not claimed to be bit-identical implementations.

- CutDSG: straight-grid cuts, bar anchors, repeated fragments and phrase-end rolls.
- WarpDSG: straight/triplet microdurations, reordered history, playback-rate shifts
  and probabilistic reverse.
- PushDSG: a syncopated push/pull skeleton with interruptions and phrase-end fills.

PHRASE selects 1, 2, 4 or 8 host bars. BUFFER selects the amount of recent audio
available to the cutter. REPEAT controls reuse of the last fragment; VARIATION
increases rearrangement and rhythmic changes; FILL controls phrase-end rolls;
REVERSE controls backwards fragments. MIX retains its lock. NEW PHRASE changes
the random seed without changing these controls. The bars below the controls
show recent source positions and the current play position; they are not steps
the user must enable. There is no Manual mode.

Cut decisions follow host quarter-note position, tempo and time signature, with
bar-boundary alignment. A prepared stereo ring is reused; processing, reseeding,
mode changes and tempo changes allocate no memory. Reads are bounded to recorded
history, and short crossfades soften cut and loop boundaries. Bypass settles to
the original signal. Brief startup history is required before cutting begins.

The new parameter IDs are appended. State version 0x5147314A reads 0x51473149 and
older saves without moving existing IDs; missing cutter controls receive defaults.
Retired manual step/Source Slice parameters remain reserved and hidden.
