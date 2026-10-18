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

CPU measurements refer to the isolated engine on the named benchmark runner,
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

### Measured Windows CPU reduction

GitHub Actions Windows 2022 trial build, commit `de1a7b2` versus `8875771`,
48 kHz stereo, 256-sample blocks, 10 seconds per scenario, median of three runs.
Normal quality reduced engine CPU by **17.0–31.3%** in these eight scenarios.
This includes the stereo-read and Reslice seed fixes from `723a5a1`.

| Preset scenario | Before (% of one core) | Normal (% of one core) | Reduction |
| --- | ---: | ---: | ---: |
| Default (-1) | 3.594 | 2.812 | 21.8% |
| 0 | 3.906 | 2.969 | 24.0% |
| 1 | 4.531 | 3.125 | 31.0% |
| 8 | 5.000 | 3.594 | 28.1% |
| 24 | 5.938 | 4.531 | 23.7% |
| 25 | 7.344 | 6.094 | 17.0% |
| 29 | 7.500 | 5.156 | 31.3% |
| 100 (extended Reslice route) | 6.875 | 5.625 | 18.2% |

All runs were deterministic. Five scenarios remained bit-identical to the
baseline; three changed with the intentional quality/grain-continuity updates.
Windows passed all 32 regression suites, all 47 VST3 validator checks, and the
native editor smoke tests. Results and packages:
[commercial build run](https://github.com/danielmelegari/GrainsDosageGTRv2/actions/runs/38904427052).
These figures do not establish the savings or dropout behavior in Cubase on Mojave.

The macOS trial and licensed builds also passed their 31 regression suites
and 47 VST3 validator checks. Unlocked macOS and Windows builds passed as well.
Testing in the user's Cubase session on macOS 10.14 remains a separate check.

### Quality-tier stress measurements

Local Linux Release build, 48 kHz, 180 BPM, 10 seconds per case, median of
three runs without competing builds/tests. `grains_benchmark_quality` uses two
distinct synthetic dense-grain patches; the second enables a modulated phaser.
Values are engine process CPU as a percentage of one core.

| Patch | Draft | Normal | High |
| --- | ---: | ---: | ---: |
| Dense grains | 6.499% | 26.690% | 29.279% |
| Dense grains with modulated phaser | 7.260% | 12.612% | 29.021% |

Draft used 75–78% less CPU than High and 42–76% less than Normal in these
stress patches. These deliberately heavy cases are not typical-preset forecasts;
sparse patches may show little difference between tiers. The final local build
passed all 31 regression suites and 47 VST3 validator checks.
