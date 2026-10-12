# Audio CPU checks

The purple three-page branch caches filter and reverb coefficients when their
inputs are unchanged. Modulated parameters still update at audio rate. Phaser
coefficients are shared between stereo channels. No voices, effects, quality
settings or sample rates are reduced.

On x86 audio threads, the processor temporarily enables flush-to-zero and
denormals-are-zero so extremely small decaying values do not cause CPU spikes
during silence. The host's previous floating-point mode is restored at the end
of each processing call, including early returns.

The Windows trial workflow compares the engine against commit `b88e029` on the
same runner, using seven preset scenarios at 48 kHz stereo and 256-sample blocks.
It reports median process CPU time over three runs and checks exact output
hashes. Download `Windows-audio-CPU-comparison` from the workflow for results.
This benchmark excludes the editor, host, licensing and input/output wrapper;
it is not a prediction of a particular DAW's CPU meter.

To reproduce locally, build `tools/benchmark_engine.cpp` with C++17 and release
optimization, once with `-I/path/to/baseline/src` and once with `-Isrc`. Then run:

```
python tools/compare_engine_cpu.py ./engine-before ./engine-after cpu-report.txt
```

Run without other builds or heavy applications for more consistent timings.
The full regression suite also checks DSP, automation/state behavior, routing,
modulation, reverb tails, limiter behavior and restoration of the host's floating
point mode. Native Windows smoke tests cover all three interface pages.
