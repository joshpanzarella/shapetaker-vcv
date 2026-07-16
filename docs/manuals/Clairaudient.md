# Clairaudient — Operating Manual

*A polyphonic dual sigmoid oscillator with an internal voice engine for the VCV Rack environment — part of the Shapetaker series.*

## Description

The Clairaudient module is a precision polyphonic dual-oscillator system (supporting up to 6 simultaneous voices) engineered for rich timbral movement and expansive stereophonic imaging. The instrument is centered around two distinct oscillator cores, designated **V** and **Z**, which may be seamlessly blended, shaped, and set against one another.

Departing from traditional analog wave generation, Clairaudient builds its waveform around a mathematical sigmoid curve: a sawtooth whose central edge can be reshaped from a gentle ramp to a razor-sharp transition. What distinguishes the instrument is that this edge does not have to sit still. The onboard **Voice Engine** modulates the curve's slope and symmetry at audio rate, locked to the pitch of each oscillator, so the harmonics inside every note shift and swirl on their own — no external modulation required. The effect is reminiscent of vowels, brass, and voices emerging from within the tone; hence the name.

## Initial Operation

1. Connect a standard 1V/Octave pitch source to the **V/OCT V** input jack.
2. Route the **L** and **R** audio outputs to a suitable mixer or voltage-controlled amplifier (VCA).
3. Set the central **Crossfade** control to its midpoint so both oscillators are audible.
4. The factory settings of the Voice Engine column (DEPTH, RATIO, ASYM, WIDTH) are already active — you should hear a moving, vocal quality immediately. Sweep the **V Shape** and **Z Shape** controls to audition the range from smooth ramp to hard edge.
5. Flip the **REV. SYNC** switch to its middle position and listen to the character turn feral. Return it to OFF when decorum must be restored.

## Architecture

### Oscillator V
Core V is the first of the twin oscillator paths.
- **Oscillator V Pitch (Octave):** By default, this rotary switch snaps precisely to musical octaves (±2 octaves) for stable octave layering.
- **Fine Tune:** Precise pitch adjustment over a ±20 cent range.
- **Shape:** Sweeps the sigmoid curve. The morph from sawtooth to a square-like wave completes in roughly the first half of the knob's travel; the remainder of the sweep sharpens the transition edge further and further, ending well past "square" in genuinely aggressive territory. The journey is the point — the middle of this knob is where the Voice Engine has the most material to work with.

### Oscillator Z
Core Z operates in parallel with Core V.
- **Oscillator Z Pitch (Semitone):** Snaps to discrete semitones over a four-octave range (±24 semitones) for immediate interval selection.
- **Fine Tune / Shape:** Operate identically to their Core V counterparts, fully independent.

*Note: The primary rotary controls are equipped with dedicated CV attenuverters located near them.*

### The Voice Engine

The right-hand column is the heart of the instrument: an internal modulator, running at audio rate and locked to each oscillator's own pitch, that animates the sigmoid curve from within. Because it tracks pitch, the character it imparts stays consistent across an entire melody — play two independent lines into V and Z and each keeps its voice.

- **DEPTH (Knob, CV & Attenuverter):** How far the modulator bends the curve within each waveform cycle. At zero the waveform is frozen and traditional. As depth rises, a formant-like sweep opens up inside every note. The lower quarter of the knob is subtle by nature; the voice truly clears its throat from the midpoint onward. Depth accepts CV per polyphonic voice — an envelope here makes each note "speak."
- **RATIO (Knob):** The modulator's speed as a multiple of the oscillator's pitch, snapped to musical ratios (×0.5 through ×7). Low integers (×2, ×3) give vowel and brass formants; high ones (×5, ×7) turn metallic and bell-like; ×0.5 repeats its pattern every *two* cycles, adding a growling octave-down component.
- **ASYM (Knob):** Skews the wave's symmetry. A symmetric square-like wave contains only odd harmonics — the hollow, chiptune sound. Asymmetry pours in the missing even harmonics: brassy, reedy, vocal. Its effect is strongest when Shape is high (where the wave is most symmetric) and nearly inaudible when Shape is low (a sawtooth already owns every harmonic). In PWM mode this knob takes on a related duty: see Waveform Selection below.
- **WIDTH (Knob):** A true mono-to-wide control, displayed 0–200%. At 0% the output collapses to mono — a useful reference point. 100% (center) is natural stereo. Beyond center, the side content is amplified and the Voice Engine's motion in the two channels is driven progressively out of phase, until at 200% the left channel's formant sweeps upward while the right sweeps downward. Extreme settings gain a touch of output saturation by design.
- **CHANCE (Small Knob):** Governs the Reverse Sync behavior described below.

### Blending and Synchronization
- **Crossfade:** The proportional mix between Oscillator V and Oscillator Z at the output stage.
- **XSYNC (Toggle):** When engaged, Core V hard-resets the phase of Core Z on each cycle. Modulating Core Z's pitch while active produces aggressive, tearing harmonic sweeps.
- **REV. SYNC (3-Position Switch):** The signature misbehavior.
  - **OFF:** Both cores run free.
  - **ON:** Each time Core V completes a cycle, it forces Core Z to reverse playback direction. The left and right copies of Z flip on *opposite* edges of V's cycle, so the resulting gnarl is spread across the stereo field.
  - **MUTUAL:** Z pushes back — its own cycle completions flip Core V's direction in return. The two oscillators shove each other around chaotically. This is the wildest setting on the module.
- **CHANCE:** At 100%, every reverse-sync flip happens on schedule (deterministic chaos). Reducing it makes each flip probabilistic — the gnarl loosens, lurches, and staggers. Sweeping CHANCE between roughly 60% and 100% while MUTUAL is engaged is the module's signature live move.

### Waveform Selection

Two waveform topologies, selectable via the context menu:

1. **Sigmoid Saw (Default):** The morphing sigmoid waveform described throughout this manual. All Voice Engine controls operate as documented above.
2. **PWM:** A classic band-limited pulse wave. Here the **Shape** control sets the static pulse width (5%–95% duty), **DEPTH** applies pitch-locked pulse-width modulation, and **ASYM** skews the *trajectory* of that modulation — at high settings the pulse lingers on its narrow side and snaps quickly through its wide side, turning even PWM shimmer into a lopsided throb. WIDTH and the sync switches operate normally.

### Stereo and Mixing Operation

The **Crossfade** control's spatial behavior is governed by the selected Crossfade Curve (context menu):

1. **Equal Power (Default):** A standard centered blend with consistent apparent volume.
2. **Stereo Swap:** Core V originates hard left, Core Z hard right; sweeping the crossfade causes them to transit across the field and trade places, with crossfeed to anchor the center image.

These interact pleasantly with the WIDTH control, which operates on the final stereo image regardless of crossfade mode.

## Connections

- **V OSC V/OCT:** Pitch tracking input for Core V (1 Volt per Octave).
- **Z OSC V/OCT:** Pitch tracking input for Core Z. *Internally normalized to the V OSC input — one pitch CV drives both cores unless this jack is patched. The normalization is per-voice: if a polyphonic cable here carries fewer channels than the V input, the uncovered voices follow their V counterparts rather than falling silent or detuning.*
- **V FINE CV / Z FINE CV:** Control voltage inputs for fine-tuning (attenuated for delicate vibrato work).
- **V SHAPE CV / Z SHAPE CV:** Control voltage inputs for waveform shaping. Audio-rate signals are fair game.
- **CROSSFADE CV:** Control voltage input for the master mixing stage.
- **DEPTH CV:** Control voltage input for Voice Engine depth, with attenuverter. Polyphonic.
- **L / R OUT:** Master stereophonic audio outputs. For monophonic operation, use the **L** output (or simply set WIDTH to 0%).

## Calibration and Advanced Settings

The context menu (right-click) is deliberately brief:

- **Settings:**
  - **Quantize:** Defeats the stepped behavior of the V (octaves) and Z (semitones) pitch controls for continuous sweeps.
  - **Oscilloscope Display:** Chooses what the central screen draws. **Waveform** (default) is a triggered trace of the left output with the right output ghosted behind it — stable, and ideal for watching the Voice Engine bend the sigmoid in real time. **Lissajous (X-Y)** plots left against right; a diagonal line means mono, an open ellipse means stereo, and a woolly cloud means the WIDTH and sync controls are doing their job.
  - **Oscilloscope Theme:** Visual styling of the central display, shared or per-module.
  - **Waveform / Crossfade Curve / Oversampling:** As described above. 4× oversampling is recommended for general operation.
- **Character:**
  - **Vintage:** A single macro governing the module's analog-modeled imperfections — slow thermal pitch drift, per-voice component tolerances (pitch, level, and pan offsets across polyphonic voices), stereo crosstalk, and bus saturation. **50% is the calibrated factory intent** — the amount a well-maintained vintage instrument would exhibit. Below that it cleans up toward digital precision; above it, the module ages considerably, ending at roughly four times the calibrated instability. Fully clockwise resembles a beloved instrument with a failing power supply.
  - **Oscillator Noise:** A hardware-like noise floor plus microscopic phase jitter that softens the waveform edges, replicating the lively instability of a vintage VCO.

## Application Notes

**Hearing Voices (the factory sound):**
Leave everything at default. Play a slow melody into V/OCT. The Voice Engine defaults (DEPTH 50%, RATIO ×2, ASYM 35%, WIDTH 100%) produce the module's signature vocal shimmer with no patching at all.

**Brass Séance:**
Sigmoid Saw mode, Shape at ~85%, DEPTH at 50%. Sweep ASYM slowly from zero: the tone transforms from hollow woodwind to full brass section as the even harmonics arrive. Add an envelope to DEPTH CV so each note swells into speech.

**The Broken Radio:**
Set REV. SYNC to MUTUAL and CHANCE to 100%. Detune Z a fifth from V. Now ride CHANCE between 60% and 100% — the two cores lurch between locked snarling and staggering collapse. WIDTH at 200% spreads the wreckage across the stereo field.

**Massive Stereo Ensemble:**
Crossfade Curve to Stereo Swap, Vintage to ~70%, WIDTH to 150%. Modulate Crossfade CV and Z Fine CV with slow, independent LFOs. The drift, voice tolerances, and opposing formant motion combine into an exceptionally wide, organic ensemble.

**Lopsided PWM Throb:**
Waveform Mode to PWM, Shape at ~40%, DEPTH at 60%, RATIO ×1. Sweep ASYM upward and the even pulse shimmer becomes a rhythmic, dub-flavored lean. WIDTH pushes the throb out of phase between the speakers.

## Polyphony and Chords

Clairaudient supports up to 6 voices. The voice count follows the widest polyphonic cable patched into any input, and both outputs carry that same channel count.

Polyphonic channels behave as **lanes**: voice *n* builds its V oscillator from channel *n* of the V/Oct V cable and its Z oscillator from channel *n* of the V/Oct Z cable, then crossfades, syncs, and voices that pair in isolation. With two chords patched in (for example, both sequences of a Transmutation), notes are therefore paired by channel position — the pairings evolve with each sequencer's voice-leading, and the sync behaviors react within each pair.

- A **mono** cable into either V/Oct input is broadcast to every voice — one note held against an entire chord.
- If the Z cable carries **fewer channels** than the V cable, the uncovered voices follow their V lane partners (per-voice normalization).


## Operating Considerations

- **Normalization:** The Z OSC V/OCT input automatically receives the V OSC signal if left unpatched, and covers any missing channels when a narrower polyphonic cable is patched.
- **Where the magic lives:** The Voice Engine is pitch-locked, so its character survives transposition — but it is still shaped by SHAPE. If DEPTH seems to be doing little, raise the Shape controls toward their middle and upper range; a pure sawtooth gives the engine nothing to bend.
- **Mono compatibility:** WIDTH at 0% is a true mono collapse and is safe at any other setting; the side content is derived from a mid/side stage rather than phase trickery.
- **CHANCE at exactly 100%** is deterministic and stable by design — an internal guard prevents the two cores from locking into degenerate rapid-fire flipping in MUTUAL mode.
