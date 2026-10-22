# GrainsDosage — complete user manual

**Windows VST3 · charcoal three-page edition · 0.13.18**\
Applies to the updated `work/mockup-purple-skin` branch. The pages are **Modules**, **Modulation**, and **Morph**. This manual describes the current generative Reslice, not the retired Preslicer or the old Reslice step editor. Shared sound controls also apply to the macOS build; installation and registration below describe Windows.

## Contents

1. [Install and register](#1-install-and-register)
2. [First session in Cubase](#2-first-session-in-cubase)
3. [Navigation and editing](#3-navigation-and-editing)
4. [Signal path and module loops](#4-signal-path-and-module-loops)
5. [Input controls](#5-input-controls)
6. [Granulizer](#6-granulizer)
7. [Beat Repeater](#7-beat-repeater)
8. [Reslice: every control](#8-reslice-every-control)
9. [Gater](#9-gater)
10. [Modulation](#10-modulation)
11. [Filter and filter sequencer](#11-filter-and-filter-sequencer)
12. [Reverb and master output](#12-reverb-and-master-output)
13. [Morph XY](#13-morph-xy)
14. [Presets, randomization and automation](#14-presets-randomization-and-automation)
15. [Practical starting patches](#15-practical-starting-patches)
16. [Troubleshooting and CPU](#16-troubleshooting-and-cpu)
17. [Publisher: issuing offline trials](#17-publisher-issuing-offline-trials)

## 1. Install and register

### Installation

Close Cubase, run the Windows trial Setup executable, and accept the administrator prompt. The installer places the complete VST3 bundle at:

```
C:\Program Files\Common Files\VST3\GrainsDosage.vst3
```

The bundle is a folder; do not move just its internal binary. Use Windows 10 or 11 x64 and a compatible 64-bit VST3 host. Trial and full editions occupy the same plugin location; installing one replaces the other. Your DAW identifies them as the same plugin.

Start Cubase after installation. Open **Studio → VST Plug-in Manager** and rescan if GrainsDosage is not listed. Insert it as an **audio effect**, not as an instrument. It needs incoming audio.

### Automatic Trial

Install the Trial edition and open it in your DAW. No license code, registration or account is required. The header displays **TRIAL - N DAYS LEFT**, updating as time passes.

### Exactly what “14 days” means

The trial starts on first editor opening or audio activation and lasts **14 consecutive days**, including time while the DAW is closed. Restarting the DAW or reinstalling the plugin preserves an intact trial record. There is no one-hour session limit.

At expiry the header displays **TRIAL EXPIRED**. Output fades to silence over 20 milliseconds and wet processing stops, including with the editor closed. Your projects and presets remain available. Keep the system clock correct; invalid protected storage or clock rollback blocks processing.

The record is stored per OS user in macOS Keychain or protected Windows storage. Fully offline software cannot prevent every deliberate reset, system restoration or binary modification.

### Purchased licenses

Install the separate Licensed edition and use **ACTIVATE** to submit its device code and paste the publisher's activation. The automatic Trial has no activation dialog. Trial and Licensed share the plugin identity and saved settings.

## 2. First session in Cubase

1. Load an audio loop on a stereo audio track and enable a repeating region in Cubase.
2. Insert GrainsDosage and start playback. For live input, enable the track's input monitoring.
3. Start with a moderate track level and keep the plugin's limiter enabled.
4. On **Modules**, enable one processor at a time. Use its Mix to hear the change.
5. On **Modulation**, scroll to the master section and use the master Mix to blend the complete effect with the input.
6. Save the Cubase project and a plugin preset when you have a useful sound.

Reslice and the module loops need time to collect incoming audio. Beat-based processors use Cubase's tempo and transport. Stopping the host can stop new rhythmic captures; reverb tails can continue.

## 3. Navigation and editing

The header and the three page tabs stay in place. Each page has its own vertical scroll position. Use the mouse wheel or the scrollbar to reach lower sections.

| Action | Windows operation |
|---|---|
| Adjust a knob | Drag vertically |
| Adjust a horizontal slider | Drag horizontally |
| Adjust a vertical slider | Drag vertically |
| Fine adjustment | Begin dragging, then hold **Shift** |
| Enter an exact value | Double-click the numeric readout (right-click also works) |
| Restore a knob/slider default | Double-click the knob/fader body, or Shift-click the control |
| Choose a discrete option | Click anywhere in the selector to open its menu; right-click to type a value |
| Change module order | Drag a module's header/handle up or down |
| Change page | Click Modules, Modulation or Morph |

Click the name of Granulizer, Beat Repeater, Reslice or Gater (▾) to open its **Save module preset… / Load module preset…** menu. These `.gdmodule` files include that module’s controls, loop settings and Mix lock where available. Loading changes only that module; other modules, modulation and routing stay unchanged. Files for another module or invalid files are rejected before any settings change. Each module has its own Presets/Modules/GRANULIZER, BEAT REPEATER, RESLICE or GATER folder. Save module preset… opens that module’s folder by default. Its .gdmodule files appear alphabetically in the dropdown and load with one click. Use Open module preset folder… to add files; reopen the menu to refresh the list. Load module preset… also accepts files elsewhere. The module menu also includes Default: it immediately restores all registered defaults for that module, including loop, power and applicable Mix lock settings, without changing other modules or routing. Full-rack `.gdspreset` files remain supported by the top preset menu. Drag the module number or grip to reorder modules. The Skin button has been removed.

Numeric entry accepts the displayed units where applicable, such as `123 ms`. Values outside the control's range are rejected.

The Modulation page includes **skin** and **zoom** menus. Windows zoom choices are 50, 60, 70, 75, 80, 90 and 100%. Windows display DPI and plugin zoom combine; the editor now uses one drawing transform for graphics and text. Opening the editor should not require minimizing or resizing it to line up the controls.

## 4. Signal path and module loops

The main signal path is:

**Input de-clicker → input stretch/transpose → the four rack modules in their displayed order → filter → reverb → output mixing/level matching → limiter.**

The four rack modules are Granulizer, Beat Repeater, Reslice and Gater. The first module is at the top. Reordering changes what each processor receives: a Gater before Reslice cuts the recorded source; a Gater after Reslice gates the resulting cuts. Old saved patches can retain their legacy parallel routing until a new rack order is selected.

The master dry branch comes from the input after the de-clicker's fixed delay; it is not the same as turning down only the last module's Mix.

Each rack module has **On**, **Loop**, and **Window** controls. Loop captures and repeats that module's output independently of the others. It is separate from Granulizer Freeze, Beat Repeater Hold, and Reslice's source buffer.

Loop windows are **2/1, 1/1, 1/2 and 1/4**: eight, four, two and one quarter-note beats. Feed audio and play long enough to fill the selected window before expecting capture. Turn Loop off to return to live processing. Changing Window releases the old capture and collects the newly selected duration. These durations are beat-based; outside 4/4, do not assume every `1/1` label means one host bar.

## 5. Input controls

| Control | Function |
|---|---|
| **Input De-clicker On** | Repairs brief isolated discontinuities before the sound processors. It is not a general noise remover. |
| **Sensitivity** | Higher settings make smaller discontinuities eligible for repair. Reduce it if wanted transients change. |
| **Transpose** | Input pitch shift, −48 to +48 semitones. |
| **Free Stretch** | Enables independent input stretching. |
| **Stretch** | Stretch playback speed, 25–400%; 100% is normal speed. |

The de-clicker has a fixed **32-sample lookahead**, including when repair is disabled, so switching repair on/off does not change the reported plugin latency. Cubase uses this for delay compensation.

## 6. Granulizer

The Granulizer reads small overlapping fragments from its input history.

| Control | Range and use |
|---|---|
| **Size** | 15–250 ms. Short sizes produce fine grain; longer sizes retain more source detail. |
| **Density** | 1–32 grains per timing step. Higher values create denser textures and can increase CPU. |
| **Lookback** | 15–2,000 ms behind the live write position. |
| **Chaos** | 0–100% randomness in grain generation. |
| **Pitch** | −48 to +48 semitones. |
| **Mix** | Granular contribution, 0–100%. |
| **Density Flow** | Spreads grain generation through the step instead of concentrating launches at its boundary. |
| **Buffer** | Input-history selection: 1, 2, 4, 8 or 16 seconds. |
| **Freeze** | Holds captured history while grains continue to read it. |
| **Pan Mode** | Manual, Alternate L/R, or Random L/R. |
| **Pan** | Manual position from left to right; visible in Manual mode. |
| **Lock** | Protects this Mix during supported preset/random operations. |

The waveform display shows the buffered signal and current grain activity. Changing Buffer while frozen selects a new frozen span with a transition. Freeze state can be saved, but a preset/project does **not** contain the recorded waveform itself; after reloading, new audio may need to fill the buffers.

## 7. Beat Repeater

The Beat Repeater captures a rhythmic fragment and plays it repeatedly.

| Control | Function |
|---|---|
| **Length** | Main repeat length; includes straight, triplet and dotted divisions. |
| **Hold** | Requests a sustained repeat capture. |
| **Grid On** | Uses the 16-step repeat pattern. |
| **Auto** | Requests repeat bursts automatically. |
| **Interval** | Time between automatic capture opportunities. |
| **Duration** | Burst duration, 1–32 sixteenth notes. |
| **Chance** | Probability of an automatic burst. |
| **Mix / Lock** | Repeater blend and its protection during preset/random operations. |
| **Step On** | Enables/disables the selected step. |
| **Division** | Repeat division for the selected step. |
| **Step Pitch** | Selected-step pitch, −48 to +48 semitones. |

Click a step to select it; double-click to enter its activity value. Shift-click restores the default. Then adjust the selected step's controls. The available divisions run from 1/1 to 1/128 and include triplet and dotted variants. A repeating texture can also be held using the independent module Loop.

## 8. Reslice: every control

Reslice records recent audio and makes tempo-synchronized cuts from it. The current interface has three generative modes; it is not a manually programmed 16-step slicer. The moving bars display generated source choices.

All **sound and capture controls in this section**, including On and module Loop/Window, are available in both the LFO destination menus and Morph X/Y menus. Mix Lock and the Random button are editing actions, not audio modulation destinations. Mode-specific destinations remain selectable even when their mode is inactive; switch to the corresponding mode to hear them.

### Main controls

| Control | Range / behavior |
|---|---|
| **On** | Enables Reslice processing. |
| **Mode** | **CutDSG**, **WarpDSG**, or **PushDSG**. |
| **Buffer** | Recent source-history window: 4/1, 2/1, 1/1 or 1/2 (16, 8, 4 or 2 quarter-note beats). |
| **Max Phrase** | Maximum generated phrase: 1, 2, 4 or 8 host bars. |
| **Min Phrase** | Minimum generated phrase: 1–8 host bars. Keep it no higher than Max Phrase for predictable setup. |
| **Subdiv** | 1–32; higher settings shorten the generated cut durations. |
| **Seed** | 1–65,535; selects the deterministic random sequence. Identical input, transport and settings are needed for repeatable results. |
| **Fade** | 0–50 ms shaping at cut and loop edges. Ramps shorten automatically for rapid rolls to preserve their body. |
| **Duty** | 0–100% audible portion of ordinary cuts. |
| **Fill Duty** | 0–100% audible portion of fill cuts. |
| **Repeat Chance** | 0–100% probability of reusing a cut. |

### Per-cut level, stereo position and pitch

| Control | Range |
|---|---|
| **Min Amp / Max Amp** | 0–2 linear gain; 1 is unity. |
| **Min Pan / Max Pan** | −100% to +100%. |
| **Min Pitch / Max Pitch** | −2,400 to +2,400 cents; 1,200 cents is one octave. |

The engine chooses values between each minimum/maximum pair at cut events. Matching the two values gives a fixed setting. Reversed endpoints are handled as a range, but keeping Min below Max is easier to read.

### Randomization

RANDOM changes the RESLICE sound controls, including Mode, crusher/comb and loop settings. MIX changes only when its lock is FREE; a locked MIX stays exactly as set. Module power and the lock itself remain unchanged. NEW PHRASE changes only the seed.

### CutDSG

CutDSG focuses on repeated cuts, stutters and movement through the source history. Repetitions retain their rhythmic duration until a bar boundary or phrase-ending fill. Fill Chance chooses whether each phrase receives an ending fill. Version 0.13.7 changes the RESLICE sound while preserving saved controls, automation and project compatibility.

| Control | Function |
|---|---|
| **Min Repeats / Max Repeats** | 0–16 forced repeat counts. |
| **Stutter** | 0–100% probability of shorter stuttering cuts. |
| **Area** | 0–100% source-history area available to the cutter. |

### WarpDSG

WarpDSG varies timing and playback speed.

| Control | Function |
|---|---|
| **Straight** | 0–100% preference for straight timing over irregular divisions. |
| **Regular** | 0–100% probability of selecting a regular cut length. |
| **Ritard** | 0–100% progressive lengthening through a phrase. |
| **Speed** | Normalized 0–1 control; centre is unity, lower values slow cuts and higher values speed them up. |

### PushDSG

**Activity** (0–100%) controls how much additional rhythmic subdivision is introduced into PushDSG's structured rhythm. Variation and Fill also affect the result.

### Bitcrusher

| Control | Function |
|---|---|
| **Crusher** | Enables the cut-level bitcrusher. |
| **Min Bits / Max Bits** | 1–32 bits. Lower values are more strongly quantized. |
| **Min Freq / Max Freq** | 100–48,000 Hz hold/sample-rate range, bounded by the actual processing rate. |

### Comb

| Control | Function |
|---|---|
| **Comb** | Enables the Reslice comb effect. |
| **Type** | Selects positive or negative feedback polarity. |
| **Feedback** | 0–95%; higher settings strengthen resonance and decay. |
| **Min Delay / Max Delay** | 1–100 ms cut-level delay range. |

This comb is inside Reslice. It is separate from the tuned Comb models in the main filter.

### Variation and output

| Control | Function |
|---|---|
| **Variation** | 0–100% amount of variation in cut choices. |
| **Fill Chance** | 0–100% chance of fills near the end of a phrase. |
| **Reverse** | 0–100% probability of reversing a cut. |
| **Mix** | 0–100% Reslice blend. |
| **Loop / Window** | Captures the module output; see section 4. |
| **Lock** | Protects Reslice Mix during supported preset/random operations. |
| **Random** | Generates a new set of Reslice settings. Save first if you want to return to the exact previous patch. |

Discrete destinations (Mode, On, Seed, subdivisions, etc.) select quantized values under modulation. Continuous destinations vary continuously, but a cut's sampled attributes normally become audible at the next generated cut. Modulating Seed or Mode quickly can repeatedly change the generation sequence; start with slow, shallow modulation.

## 9. Gater

The Gater applies a rhythmic amplitude pattern.

| Control | Function |
|---|---|
| **Rate** | Step interval: 1/4, 1/8, 1/16 or 1/32. |
| **Length RND** | Randomizes audible lengths. |
| **Step RND** | Randomizes step activity. |
| **Chance** | Probability used for random gate activity. |
| **Min Length** | Lower length bound, 5–95%. |
| **Step Length** | Selected step's length, 5–100%. |
| **Sustain** | Selected step's additional tail, 0–500 ms. |
| **Latch** | Empty steps preserve the current gate state; Release steps close it. |

Click a step to toggle its activity and select it. **Alt-click** (Option-click on macOS) a step to toggle a Release marker. In normal mode, inactive steps close the gate. In Latch mode, use Release markers deliberately; otherwise an open gate may remain open across empty steps. A pattern with no active opening steps can silence the signal. The module's On switch and module Loop are separate.

## 10. Modulation

There are **four independent LFOs**, each with **six destination slots**. Select an LFO, enable it, choose a waveform and route at least one nonzero amount to a destination.

| Control | Function |
|---|---|
| **Wave** | 328 shaped waves plus Sample & Hold and Sample & Glide. The 200 new shapes are grouped into ten families. Saved Wave preserves old preset/automation selections. |
| **Sync** | Switches between host-tempo timing and free-running Hz. |
| **Length** | Synchronized cycle length; offers straight, triplet, dotted and long cycles. |
| **Rate** | Free-running rate, 0.01–40 Hz. |
| **Points** | 1–64 random values per cycle for the random wave modes. |
| **Depth** | Global LFO output depth. |
| **Phase** | Starting phase offset. |
| **Glide** | Smoothing of random transitions and waveform changes where applicable. |
| **Retrigger** | Resets modulation from the engine's rhythmic trigger. |
| **Speed** | 0.25×, 0.5×, 1× or 2× rate multiplier. |
| **Mod W.** | Tempo-quantized random waveform changes, or Off. |
| **Dest** | Selects a target; None disables that slot. |
| **Amount** | Routing depth; centre is zero. |
| **Polarity** | Bipolar, positive-only or negative-only modulation. |

The eight waveform families are Sine Warp, Skew Triangle, Pulse, Rise Curve, Fall Curve, Harmonic, Staircase and Random Curve. Sample & Hold jumps between random values; Sample & Glide interpolates between them.

Destinations include grain parameters, stretch and transpose, filter controls, repeat controls and individual repeat pitches, reverb controls, gate controls and individual gate lengths/sustains, master Dry/Wet, filter-sequencer controls, and the complete current Reslice sound/capture controls.

Multiple routes to one destination add together. Modulated values are limited to their legal normalized range. Use small amounts first; large amounts can pin a control at an endpoint for much of the cycle. Turning an LFO off removes its contribution without erasing its routing.

## 11. Filter and filter sequencer

The main filter follows the rack.

| Control | Function |
|---|---|
| **On** | Enables the filter. |
| **Cutoff** | Logarithmic frequency control, approximately 20 Hz–20 kHz, constrained by sample rate. |
| **Resonance** | 0–100% resonance. |
| **Drive** | 0–24 dB nonlinear input drive. |
| **Type** | Low-pass, high-pass or band-pass where applicable. |
| **Slope** | 12 or 24 dB selection where applicable. |
| **Alg** | Filter model. |

The 19 models are Classic, Northern VA, Transistor Ladder, Acid Ladder, Notch, Resonant Peak, All-pass, Comb +, Comb −, Formant A/E/I/O/U, Phaser 4, Phaser 8, Low + High, Low + Band and Band + High. These are original algorithms; model names do not imply exact emulations of another product. Type and Slope have model-dependent effects.

The dedicated **filter sequencer** adds movement to Cutoff:

- **On** enables it.
- **Mode** chooses an original 32-step pattern or Sample & Glide.
- **Pattern** selects one of 64 patterns across Rise, Fall, Triangle, Bounce, Stairs, Sine, Scatter and Pulse families.
- **Rate** sets step duration.
- **Depth** sets the amount of movement.
- **Glide** smooths it.

With Comb + or Comb −, **Root**, **Octave** and **Notes** select tuned note movement. Notes can follow a minor chord or the natural minor scale.

## 12. Reverb and master output

Reverb is after the filter. Its sequencer gates the **send into the reverb**, allowing previously generated tails to continue between unlit steps.

| Control | Function |
|---|---|
| **On** | Enables reverb contribution. |
| **Type** | Classic, Plate, Cosmic Space, Dark Space or Bloom Space. |
| **Mode** | For Classic: Room, Chamber, Hall, Ambient or Space character. |
| **Source** | Step Sequencer or Random Impulse. |
| **Rate** | Tempo-synchronized trigger interval. |
| **Amount** | Wet reverb level, 0–100%. |
| **Length** | Decay setting, 0.2–20 seconds. |
| **16 steps** | Click to choose which step-sequencer positions feed reverb. |
| **Kill Dry** | Removes the direct signal, leaving reverb. This can cause silence before a tail has been excited. |
| **Lock** | Protects Amount during supported preset/random operations. |

The master section contains:

- **Mix:** overall dry/effect balance. Reverb Kill Dry deliberately overrides the direct contribution.
- **Normalize:** adaptive wet-level matching; not offline peak normalization.
- **Limiter:** stereo-linked sample-peak limiting.
- **Limit:** 0, −6 or −10 dBFS ceiling.
- **Mix Lock:** protects the master Mix during supported preset/random operations.
- **Output meters:** left/right output activity.

The limiter is a sample-peak limiter, not an oversampled true-peak limiter. Use Cubase's output metering and leave headroom when required.

## 13. Morph XY

Morph lets you move two parameter offsets with one gesture.

1. Enable **On** on the Morph page.
2. Choose **X Dest** and **Y Dest**.
3. Set each axis's amount.
4. Drag inside the pad.

Left/right controls X; down/up controls Y. The centre produces zero offset. Positive amounts increase the destination toward the right/top and decrease it toward the left/bottom; negative amounts reverse that direction. A zero amount disables that axis's effect.

Morph offsets the saved base parameter; it does not replace the entire preset. LFO modulation then acts on the resulting settings. If both axes target the same parameter, their contributions combine within the legal range.

The appended Reslice destinations include Mix, source Buffer, algorithm, phrase limits, repeat/variation/fill/reverse, seed, every advanced cut/crusher/comb setting, module On and module Loop/Window. Discrete selections snap between their valid options.

Example: assign X to **Reslice Variation**, Y to **Reslice Mix**, use modest positive amounts, and move from the centre toward the top-right to increase both complexity and effect level.

## 14. Presets, randomization and automation

The header preset selector contains **Default** and **Saved Presets**. No factory files are installed, updated or deleted. Untouched old bundled files are hidden; edited and user-named files remain available. **Load** can still open any retained file. **Save** stores your own settings. The arrows cycle Default and saved presets.

Windows preset location:

```
%APPDATA%\GrainsDosage\Presets\
```

macOS uses `~/Library/Audio/Presets/GrainsDosage/`.

Presets contain parameter values, routing and applicable control states. They do not store recorded audio, frozen waveforms, or captured loops. Save your Cubase project as well as a plugin preset when you need host routing and automation.

The header **RANDOM** button generates a new sound across Granulizer, Beat Repeater, Reslice, Gater, filters, reverb, input pitch/stretch, Morph and all four modulation sources. It can choose all 330 waveforms and uses only visible LFO destinations. Mix locks, quality, output protection and input de-click settings remain unchanged. Freeze is released so the new sound accepts incoming audio. Each click updates the controls and host parameters immediately, even while transport is stopped.

Module **Random** buttons change that module's settings. The supported Mix locks preserve the selected mix controls during preset/random operations; they do not prevent ordinary manual adjustment or host automation. Save a named preset before experimenting if you need an exact return point.

The expanded destination lists have a new saved-state/preset format. Earlier presets and saved plugin state are migrated so their selected destinations remain meaningful. New presets require this release or newer. **Existing Cubase automation lanes that automate destination selectors themselves use the old list's normalized positions; check and reselect those destination points after upgrading.** Ordinary parameter IDs remain unchanged. Automating destination *amounts* is usually more predictable than rapidly changing the selected destination.

Use Cubase's Write/Read automation controls for recordable parameters. Page navigation is local UI state and is not an audio parameter. For an important existing project, retain a copy before upgrading and compare playback after loading it.

## 15. Practical starting patches

### Gentle granular widening

Enable Granulizer, use Size around 70–120 ms, moderate Density, low Chaos, and Alternate L/R pan. Keep module Mix modest and blend with master Mix. Leave Freeze off for a live texture.

### Rhythmic repeat fill

Enable Beat Repeater and Auto, choose a short Length, set an Interval of one bar-equivalent at 4/4, and reduce Chance. Raise Duration for a longer burst. Use individual step pitches for melodic variation.

### Reslice drum variations

Enable Reslice in CutDSG. Start with an 8- or 16-beat Buffer, Subdiv 8, Fade about 3 ms, matched unity amplitude, centred pan and zero pitch. Increase Variation, then Fill Chance and Repeat Chance. Add crusher or comb only after the basic rhythm is working.

### Slow warped texture

Choose WarpDSG, lengthen the phrase and lower Speed gradually. Use a small LFO amount on Reslice Speed or Min/Max Pitch. Add a long reverb and blend conservatively.

### Gated ambience

Place Gater late in the rack, select a few active steps and lengthen Sustain. Add sequenced reverb afterward. Try Kill Dry only after hearing a healthy reverb tail.

## 16. Troubleshooting and CPU

### No audio

1. Bypass the insert in Cubase to confirm the track itself has audio.
2. Check the Trial countdown in the header, or the activation status in the Licensed edition.
3. Confirm system date/time is correct.
4. Disable Gater and Reverb Kill Dry temporarily; check Mix, step patterns and module states.
5. Release Freeze and module Loops, then feed audio while the transport plays.
6. Verify Cubase is loading the updated three-page trial from the standard VST3 folder, not another copy.

If a valid trial unexpectedly goes silent, record the time, preset, sample rate, buffer size, number of plugin instances, and whether reopening restores sound. There is no intended one-hour timeout. This update retries temporary protected-storage access failures and can recover when validation succeeds; it still enforces the original trial deadline on each audio block.

Do not delete licensing files to troubleshoot. If the trial has expired, install and activate the Licensed edition to continue.

### GUI opens at the wrong scale

This update fixes the double graphics scale and applies actual host dimensions at attachment, including VST3 DPI notifications. Close Cubase while installing it, then reopen the editor. Check Cubase's plugin information/path if the old behavior remains. Use the plugin's Zoom menu for readability; minimizing should not be necessary.

### High Cubase CPU or audio dropouts

The audio engine continues processing when the window is closed. This release caches unchanged filter/reverb calculations and protects x86 audio processing against costly subnormal values during silence. It does not reduce audio quality or voice limits.

Cubase's real-time meter depends on your CPU, audio driver, buffer size, sample rate, track routing and other plugins. Benchmark numbers from a build runner are not a promise about your computer. Higher sample rates and smaller buffers increase real-time pressure; dense grains, heavily modulated filters and complex reverbs can add work. Compare the same preset and project before/after updating. During mixing, a larger ASIO buffer or rendering a completed track can provide more processing headroom.

### Changes are not audible

Check module On, module Mix and master Mix. For an LFO, check its On, Depth, destination, Amount and polarity. For Morph, check On, destination and axis amount. A mode-specific Reslice control only affects its relevant mode; cut-level values can wait until the next cut. A held module loop can keep repeating older output while you change controls upstream.

## 17. Publisher: issuing offline trials

The automatic Trial needs no issued code. The instructions below are for legacy signed trials and purchased Licensed activations.

For normal issuance on macOS Mojave 10.14 Intel, use **GrainsDosage License Generator.app**. Its [publisher guide](docs/license-generator.md) covers importing an existing RSA private key or creating one on the publisher’s Mac, pairing its public key with updated builds, the persistent SQLite register, 14-day trials, and permanent licenses for up to three devices. It exports only `GrainsDosage-Trial-Key.txt` or `GrainsDosage-Permanent-Key.txt`. Reissuing a trial for the same machine retains the original dates. Windows registration accepts either TXT and shows whether the license is trial or permanent. Signed GDDEV2 trials and Windows permanent codes require the matching updated plugin build.

The command-line alternative below is for existing Windows publisher workflows; it does not automatically update the native generator’s ledger. Keep one authoritative issuance history.

Customers need the signed license, not the private signing key. On the private issuing computer, install Python and `cryptography`, then use the existing private key matching the public key included in this build:

```text
python tools/issue_trial_license.py issue --key /private/path/publisher-private.pem --request GrainsDosage-machine-request.json --out trial.license
```

Default start is issuance time. To schedule the customer's full trial window, use an explicit timezone-aware start:

```text
python tools/issue_trial_license.py issue --key /private/path/publisher-private.pem --request GrainsDosage-machine-request.json --start 2026-11-01T12:00:00Z --out trial.license
```

Expiry is always 14 days after that start. Keep a private registration ledger and the signing key outside this repository. Send only the license file to the registered customer. Windows `.license` files and macOS activation codes have different formats.

See [OFFLINE-TRIAL.md](OFFLINE-TRIAL.md) for the signing-key setup and protection details. The Windows trial workflow publishes the installer, three-page screenshots and a CPU comparison report. Download retention on GitHub does not determine a customer's license expiry.

The editor requests 90% size on every opening. Use the Size menu to adjust it during the current session; reopening requests 90% again. Windows applies the host DPI scale on top of this UI size.

Seed changes (including New Phrase) take effect at the next scheduled Reslice cut rather than interrupting the current audio. This preserves the current cut’s fade and avoids seed-edit discontinuities.

## Output quality

Output Quality: Draft reduces CPU with a 16-grain budget and slower filter-coefficient refresh; Normal (default) uses 64 grains; High allows 128 grains and sample-rate coefficient refresh. All modes keep the host sample rate and latency. Change Quality in the Modulation page’s Output panel; the choice is saved with projects and rack presets. Existing sounding grains finish naturally when lowering quality.

The current interface uses charcoal panels, orange controls and cyan values, steps and displays throughout all three pages.

Each modulator has **FROM** and **TO** beneath MOD W. to limit random wave selection to an inclusive range from **1 to 330**, matching the wave menu. For example, 123–287 selects only waves in that interval. Equal endpoints select one wave; reversed endpoints use the smaller-to-larger interval. The range is saved per modulator in presets and sessions. Older presets default to the full range. These bounds apply only when wave randomization is enabled; header RANDOM preserves them.

Header **RANDOM** sets each unlocked Granulizer, Beat Repeater, Reslice and Reverb mix to 100% when enabled, or 0% when disabled. Existing Mix locks take precedence. All four module loops are switched off, and generated LFO/Morph routes cannot target Loop On. Per-module Random buttons retain their existing behavior.
