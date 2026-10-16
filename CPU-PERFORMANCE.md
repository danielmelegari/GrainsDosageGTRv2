# Audio quality and CPU

Version 0.13.15 adds **Quality** in the Modulation page's Output panel:

| Mode | Maximum simultaneous grains | Grain envelope | Filter coefficient refresh at 48 kHz |
| --- | ---: | --- | --- |
| Draft | 16 | 256-point interpolated Hann | Every 32 samples (1.5 kHz) |
| Normal (default) | 64 | 2048-point interpolated Hann | Every 8 samples (6 kHz) |
| High | 128 | Analytic Hann | Every sample (48 kHz) |

Draft trades some dense granular texture and fast filter movement detail for
lower cost. Normal is the everyday setting; High is useful for demanding
modulation or rendering. The coefficient cadence applies to the newer filter
models; the Classic filter keeps its original rate. Every tier retains host-rate
stereo audio, sample-accurate LFO timing, all enabled effects, the limiter, and
unchanged latency. Quiet/simple patches may cost almost the same in all tiers.
The selector is automated and saved with projects and rack presets; older files
load Normal. Existing parameter and routing IDs are unchanged.

Quality changes do not clear buffers or cut off sounding grains. Excess new
grains are skipped when the selected budget is occupied. A 3 ms gain smoother
prevents retiring grains from abruptly changing the level of the remaining
voices in non-flow mode. This addresses a reproducible source of bass clicks;
it cannot guarantee removal of host buffer underruns or intentionally chopped,
bitcrushed, gated or clipped sounds.

All tiers also benefit from sparse LFO route processing, cached routing order,
a neutral-transpose fast path, cached formant coefficients, and lower-rate UI
meter publishing. The master filter stops doing inaudible work once bypass has
faded out; the limiter continues normally. No audio-thread allocation is added.

CPU measurements below refer to the isolated engine on the local Linux runner,
not Cubase on an Intel Mojave Mac. Host, UI, input de-clicker and output limiter
costs are outside that engine benchmark. Use the same preset, sample rate,
buffer size and active instance count when comparing on the target Mac.

# Previous CPU work

The purple three-page branch caches filter and reverb coefficients when their
inputs are unchanged. Modulated parameters still update at audio rate. Phaser
coefficients are shared between stereo channels. That earlier optimization did not reduce voices, effects or sample rates. Extended modulation updates its routed
fields directly instead of copying the complete settings and LFO matrices for
every audio sample. Scenario 100 exercises an extended Reslice Mix route.

On x86 audio threads, the processor temporarily enables flush-to-zero and
denormals-are-zero so extremely small decaying values do not cause CPU spikes
during silence. The host's previous floating-point mode is restored at the end
of each processing call, including early returns.

The Windows trial workflow compares the engine against commit `8875771` on the
same runner, using eight preset scenarios at 48 kHz stereo and 256-sample blocks.
It reports median process CPU time over three runs, verifies deterministic
output within each build, and reports changed hashes for intentional quality
and grain-continuity changes. Download `Windows-audio-CPU-comparison` from the workflow for results.
This benchmark excludes the editor, host, licensing and input/output wrapper;
it is not a prediction of a particular DAW's CPU meter.

To reproduce locally, build `tools/benchmark_engine.cpp` with C++17 and release
optimization, once with `-DGRAINS_BENCHMARK_OLD_ENGINE=1 -I/path/to/baseline/src`
and once with `-Isrc`. Then run:

```
python tools/compare_engine_cpu.py ./engine-before ./engine-after cpu-report.txt --allow-audio-change
```

Run without other builds or heavy applications for more consistent timings.
The full regression suite also checks DSP, automation/state behavior, routing,
modulation, reverb tails, limiter behavior and restoration of the host's floating
point mode. Native Windows smoke tests cover all three interface pages.

The current benchmark optionally accepts `0`, `1`, or `2` for Draft, Normal,
or High. Comparisons across tiers intentionally do not require exact hashes.

### Bass continuity regression

A 3-second 30/55/90 Hz sine test at 48 kHz uses overlapping non-flow grains with
variation, in serial and legacy parallel routing. Before the fix, maximum adjacent
sample jumps were 0.111–0.158, with 10–14 jumps above 0.025 per measured interval.
After gain smoothing they were 0.0023–0.0106, with none above 0.025. This is a
specific DSP regression, not a diagnosis of every reported Cubase dropout.
