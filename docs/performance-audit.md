# Repeated-work audit and Output bar removal

Changes relative to commit 67c089e:

- Remove the unused Engine settings mirror and its repeated copies.
- Read the saved parameter array directly when XY is disabled; retain the
  private modified copy when XY is enabled, including combined axis offsets.
- Cache fitted Mac label text before font measurement. Cached paints retain
  the fitted font size and include available width in the cache key.
- Remove the Output left/right level bars, their smoothing, redraw triggers,
  per-sample RMS accumulation, and RMS output notifications. Keep their old
  parameter IDs registered to preserve existing parameter/state layouts.

The DSP benchmark ran three times before and three times after the copy changes
at 48 kHz, updating settings every 256 samples. All eight audio hashes matched
exactly. Median CPU time, expressed as percent of one core on this Linux runner:

| Preset | Before | After |
| --- | ---: | ---: |
| Default | 1.394 | 1.306 |
| 0 | 1.472 | 1.463 |
| 1 | 1.717 | 1.598 |
| 8 | 2.014 | 2.003 |
| 24 | 2.810 | 2.750 |
| 25 | 3.823 | 3.657 |
| 29 | 3.445 | 3.239 |
| Extended Reslice route | 3.847 | 3.618 |

These small measurements include timing variability. The benchmark exercises
DSP and parameter conversion, not native drawing or Ableton. It does not
measure the additional savings from removing the wrapper's RMS calculation.
Run `grains_benchmark_engine` from a Release build to reproduce the workload.

Regression coverage includes XY destination conversion, Freeze, routing and
buffer allocation checks, 32/64-bit host buffers, sample-offset automation,
state compatibility, and display transport. The host-buffer test now asserts
that removed RMS monitors produce no output parameter queues. Display transport
uses the retained peak monitor to verify delivery.
