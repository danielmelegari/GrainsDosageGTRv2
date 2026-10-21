# Display transport and Ableton responsiveness

Introduced in version 0.13.16; check this version in the editor after installation.

The custom editor previously received waveform, meter and modulation values
through VST3 outputParameterChanges. One waveform refresh could contain hundreds
of read-only parameter notifications, even though they are not user automation.
Rate limiting and suppression of identical values reduced this traffic but did
not remove it from the host's parameter path.

The controller now requests display data through standard VST3 IConnectionPoint
messages on an SDK main-thread timer. Once a round trip succeeds, display values
stop entering outputParameterChanges. No pointer is exchanged between components;
the message carries a versioned, size-checked binary payload. Editable parameters,
parameter IDs, state serialization, and audio processing remain unchanged.

The audio thread publishes latest values to per-instance lock-free atomic cells.
It does not allocate messages, send messages, take locks or wait for the UI.
Waveform extraction is requested by the editor and remains capped at 10 Hz.
Polling stops when no editor retains the controller's editor state. A host without
working message delivery keeps the existing parameter-based display fallback.

Regression coverage in test_display_transport.cpp:
- zero output parameter notifications after display-message negotiation;
- zero host edit/restart callbacks caused by a display reply;
- zero allocations or host message creation during process();
- bit-identical 32-bit and 64-bit output versus the same processor without a sink;
- display values reach the intended controller and do not leak across instances;
- malformed packets are rejected and editable controls/navigation are untouched;
- disconnected components return to the compatibility fallback;
- polling stops when the last simulated editor is released.

The test exercises the SDK message interfaces with a test host. It is not an
Ableton Live test. Successful native builds and SDK validation do not by themselves
prove that a particular installed Live version is responsive. The reported
Ableton-only stall is the motivation; the exact behavior in Live remains a host
verification item.

## Follow-up audit: 0.13.17

Two independently reproduced processor defects were corrected:

- A stopped host reporting a constant musical position triggered a transport
  reset every audio block. The host timeline is now tracked separately from the
  free-running live-input clock. Seeks, loop wraps and stop/resume transitions
  still reset once; zero-sample automation flushes do not change that tracking.
  Before the fix, identical stopped live input rendered with 128 versus 512
  frames differed by a maximum of 0.143414 (normalized sample amplitude). After
  the fix the difference is zero in the regression test.
- Exhausted automation queues were scanned on every remaining sample. They are
  now removed from the active list while future events keep their sample offsets.
  For one offset-zero event in a 2048-frame block, getPointCount calls fell from
  2049 to 1. This measures removed queue work, not total CPU improvement.

Both regressions failed against the preceding implementation before their fixes.
The wrapper tests also cover seeks, looping, stopping, resuming, missing context,
automation-only flushes, and multiple automation events within a block.

Neither finding establishes the cause of the reported one-second Ableton UI
stall. That host-specific symptom has not been reproduced in this environment.
