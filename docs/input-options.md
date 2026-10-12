# Input Options

Transpose and Free Stretch now process incoming stereo audio before any of the
four routing modules. The controls, parameter IDs, saved values and modulation
destinations remain the same. Granulizer, Beat Repeater and Reslice capture this
processed input, and Gater receives it in the selected chain order.

The filter, filter sequencer, reverb, output Mix and limiter retain their output
positions. Output Mix at 0% still delivers the original input. Existing presets
with non-neutral Input Options intentionally sound different because processing
now happens before captures, rather than after the module chain.

The two existing processors and prepared buffers are reused, with no added audio
thread allocations. Neutral Transpose and disabled Stretch remain transparent.
