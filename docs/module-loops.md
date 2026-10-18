# Module capture loops

Granulizer, Beat Repeater, Reslice and Gater each have a LOOP toggle and a WINDOW selector in their header. The module name and dotted grip still drag to change the routing.

| Window | Quarter-note beats | Duration at 120 BPM |
| --- | ---: | ---: |
| 2/1 | 8 | 4 seconds |
| 1/1 | 4 | 2 seconds |
| 1/2 | 2 | 1 second |
| 1/4 | 1 | 0.5 seconds |

LOOP captures the most recent complete window of that module’s output (after its Mix), then repeats that exact capture until LOOP is switched off. Downstream modules continue processing it. Each module has its own capture; multiple loops can run together. Input Options remain before the chain; Filter, Reverb and Output remain after it.

Feed audio with the transport playing to build history. If there is not yet a full window, LOOP arms and continues passing the module output until it can capture. The lit button indicates the requested ON state, including this arming period. OFF returns to current module output with a 3 ms fade; capture and wrap boundaries are also smoothed.

The captured audio survives transport seeks, host cycle boundaries and stop/start while the plugin remains processing. Disabling its module releases the capture. Changing WINDOW releases the old capture and arms a fresh window. After releasing a frozen capture, a full fresh window is required before another capture. A tempo change adjusts playback rate to keep the chosen musical duration; this changes pitch, rather than applying time stretching.

Toggle and window settings are saved in projects and presets, but captured audio is not. Older projects and all factory presets load with LOOP off and WINDOW 2/1. Existing parameter IDs are preserved; the eight new parameters are appended. Random does not change LOOP or WINDOW.

## Realtime cost

Four stereo rings are allocated once during engine preparation, each sized for 24 seconds (2/1 at the minimum supported 20 BPM). Capture freezes its ring in place: no full-buffer copy, allocation or deallocation occurs on the audio thread. The retired Preslicer capture allocation has been removed. Net additional buffer storage is approximately 23.44 MiB at 48 kHz, scaling with sample rate. The GUI uses existing parameter-driven redraws and does not add a refresh timer.

Regression coverage includes all window lengths, capture order, tempo changes, OFF/recapture, transport continuity, per-module insertion, allocation guards, old-state migration and 5,760 page/routing layout variants. Native GUI and state tests run in the unified macOS/Windows workflow.

A local whole-engine benchmark at 48 kHz (four modules enabled, five passes of 30 simulated seconds) measured median one-core CPU of 1.78% before this change, 2.03% with capture history recording and LOOP off, and 2.09% with all four loops on after eight seconds. The LOOP-off output checksum matches the baseline exactly in this scenario. These cloud-worker measurements exclude the GUI and do not predict Ableton/Cubase load on a specific Mac.
