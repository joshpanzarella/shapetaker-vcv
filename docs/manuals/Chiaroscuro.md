# Chiaroscuro — Operating Manual

*A polyphonic voltage-controlled amplifier and stereophonic distortion core for the VCV Rack environment — part of the Shapetaker series.*

## Description

The Chiaroscuro module is a versatile stereophonic signal processor (supporting up to 16 simultaneous polyphonic voices) that pairs a high-fidelity voltage-controlled amplifier (VCA) with a sophisticated, multi-topology distortion engine. 

Designed to provide everything from pristine level control to catastrophic signal destruction, Chiaroscuro utilizes advanced adaptive makeup gain and level-tracking algorithms to maintain consistent apparent volume regardless of the selected distortion type or drive amount. This allows the operator to dynamically morph between clean and heavily saturated states without introducing uncontrolled volume spikes.

## Initial Operation

1. Connect a monophonic or stereophonic audio source to the **L** and **R AUDIO IN** jacks.
2. Route the **L** and **R AUDIO OUT** jacks to your mixer or final output stage.
3. Advance the primary **VCA GAIN** control to 12 o'clock to establish a baseline signal level.
4. Ensure the **DIST** (Distortion Amount) and **DRIVE** controls are fully counter-clockwise (off) for a clean signal.
5. Apply a control voltage envelope to the **VCA CV** input to articulate the signal, adjusting the large VCA knob to set the resting floor.

## Architecture

The front panel is conceptually divided into an amplification stage and a non-linear processing stage. 

### Amplification (VCA)
The upper portion of the module governs the primary signal level before it enters the distortion core.
- **VCA Gain (Main Rotary):** Sets the baseline amplification level, from silence at fully counter-clockwise to unity at fully clockwise. This control sums with the VCA CV input, and the sum is permitted to exceed unity: a positive control voltage against an open knob will amplify up to twice the input level, or four times through the exponential curve. The output stage soft-clips beyond roughly 8 V, so this headroom is usable for deliberate overdrive rather than a route to raw digital clipping.
- **LIN / EXP Switch:** Selects the mathematical response curve of the VCA. Linear (LIN) is recommended for control voltage processing or slow tremolo, while Exponential (EXP) provides the snappy, percussive response typical of classic analog synthesizers.
- **LINK L/R Switch:** Forces the right channel to process the left input signal, collapsing the module to mono regardless of what is patched into **R AUDIO IN**. Note that normalling happens independently of this switch: if **R AUDIO IN** is left unpatched, the left signal feeds both channels automatically. LINK is therefore only meaningful when a signal *is* present at the right input and you wish to override it.

### Non-Linear Processing (Distortion Core)
The lower portion of the module governs the character and intensity of the harmonic distortion.
- **TYPE (Rotary Selector):** A five-position switch selecting the active distortion topology (see *Distortion Topologies* below).
- **DIST (Distortion Amount):** Dictates the primary depth or intensity of the selected distortion algorithm.
- **DRIVE:** Controls how aggressively the VCA output is pushed into the distortion core. High drive settings will result in earlier onset of clipping and richer harmonic generation.
- **MIX:** A master wet/dry crossfader. Fully counter-clockwise yields the pure VCA output; fully clockwise yields only the distorted signal.

*Note: The DIST, DRIVE, and MIX controls are each equipped with dedicated control voltage (CV) attenuverters located directly below them.*

### Visual Feedback
Chiaroscuro provides two jewel indicators for immediate visual telemetry:
- **Left Jewel (Gain):** Indicates the effective amplitude of the VCA stage (knob plus CV). Brightness covers the range from silence up to unity gain; beyond unity the lens cannot grow brighter, so it instead runs hot toward white to show how far into the above-unity boost region the VCA has been driven. Its colour is set by the display theme — by default the module follows the global Shapetaker theme, but an independent colour may be chosen from the context menu (see *Calibration and Advanced Settings*).
- **Right Jewel (Distortion):** Illuminates to indicate the intensity of the distortion processing. Its color is tied to the active distortion topology (e.g., magenta purple for 'Ring Mod'), and its brightness reflects the actual harmonic density being generated.

### Distortion Topologies

The central **TYPE** selector provides access to five distinct non-linear algorithms:

1. **Hard Clip (Teal):** A brutal, unyielding clipping stage that sharply truncates waveforms exceeding the threshold. Excellent for aggressive, transistor-style fuzz.
2. **Tube Sat (Aqua):** A softer, asymmetric saturation curve modeled after overdriven vacuum tubes. Generates pleasing even-order harmonics and warmth.
3. **Wave Fold (Cyan Blue):** Instead of clipping, the waveform is inverted and folded back upon itself when driven hard, creating complex, metallic overtone sweeps typical of West Coast synthesis.
4. **Bit Crush (Deep Blue):** Deliberately degrades the digital resolution and sample rate of the signal, introducing harsh quantization noise and aliasing artifacts reminiscent of early digital samplers.
5. **Ring Mod (Magenta Purple):** Multiplies the signal against an internal carrier or itself, producing inharmonic sidebands and bell-like, metallic timbres.

## Connections

- **AUDIO IN (L / R):** Stereophonic audio inputs. Polyphonic signals are fully supported, and the voice count is taken from these jacks alone — a polyphonic cable at any other input will not by itself cause the module to process more than one voice.
- **AUDIO OUT (L / R):** Master stereophonic audio outputs, carrying the same number of voices as the input.
- **VCA CV:** Primary amplitude modulation input. Polyphonic: each voice is amplified by its own channel of this input, so a polyphonic envelope articulates each note independently.
- **DIST CV / DRIVE CV / MIX CV:** Control voltage inputs for the distortion stage parameters.
- **DIST TYPE CV:** Allows for voltage-controlled switching between the five distortion topologies. The full 0-10 V range spans all five positions, summed with the knob.
- **SIDECHAIN DETECT:** An envelope follower input used to modulate the distortion characteristics dynamically (see *Calibration and Advanced Settings*).

**Polyphony and the distortion stage.** The audio path is fully polyphonic — every voice receives its own independent distortion engine on each side of the stereo field, which is what keeps a chord from collapsing into intermodulation mud. The distortion *settings*, however, are global: DIST, DRIVE, MIX, DIST TYPE and SIDECHAIN DETECT each read a single control value and apply it identically to every voice, exactly as a single outboard distortion unit would treat everything passing through it. All voices are always distorted equally; what is not available is per-voice variation of the distortion, so a polyphonic cable patched into DIST CV will drive all voices from its first channel rather than giving each note a different amount. Ordinary monophonic modulation — an LFO into DRIVE CV, an envelope into MIX CV — behaves exactly as expected.

## Calibration and Advanced Settings

Access the context menu (right-click) to configure advanced behavioral characteristics:

- **Oversampling:** Selectable internal processing rates (1x, 2x, 4x, or 8x). Higher rates utilize cascaded anti-aliasing filters to ensure pristine audio quality and eliminate unwanted digital artifacts when applying heavy distortion. 4x is the recommended default. Note that this setting audibly alters the character of some topologies rather than merely cleaning them up — see *Oversampling as a Tonal Control* under *Operating Considerations*.
- **Sidechain Mode:** Configures the behavior of the **SIDECHAIN DETECT** input:
  - *Enhancement (Trigger):* A hot signal at the sidechain input *increases* the distortion and drive amounts. Ideal for emphasizing the transients of a drum break. Note that in this mode the knob positions become *maxima* rather than absolute settings: with the sidechain input patched but idle, distortion and drive fall to zero. This mode additionally imposes a wet floor of 80% on the MIX control, so that the sidechain-triggered distortion is actually heard; a MIX setting below 80% has no effect while Enhancement is active.
  - *Ducking (Inverse):* A hot signal at the sidechain input *decreases* the distortion and drive. Excellent for "cleaning up" a heavy bassline whenever a kick drum fires.
  - *Direct Control:* Bypasses the DIST knob and its CV entirely, allowing the sidechain envelope to directly dictate the distortion amount. DRIVE and MIX continue to operate normally.
- **Display Theme:** Sets the colour of the gain jewel. By default the module follows the global Shapetaker theme so that a rack of modules stays visually consistent; unticking that option allows an independent colour to be chosen for this instance.

## Application Notes

**Dynamic Drum Crushing:** 
Route a drum break through Chiaroscuro. Set the Waveform Type to **Bit Crush** or **Hard Clip**. Patch the kick drum trigger into the **SIDECHAIN DETECT** input, and set the Sidechain Mode to *Enhancement*. The drums will remain relatively clean, but every kick will trigger a burst of intense, gritty distortion.

**Polyphonic Wavefold Pads:** 
Route a polyphonic sine or triangle wave chord into the audio inputs. Select the **Wave Fold** topology. Slowly modulate the **DRIVE CV** with a triangle LFO. Because Chiaroscuro processes polyphony discretely per voice, the wavefolding will generate massive, complex harmonic movement without turning the chord into intermodulation mud.

**Screaming Acid Bass:** 
Select the **Tube Sat** topology. Set the VCA response to **EXP** and drive the VCA CV with a very fast, snappy envelope. Turn the **DRIVE** up to 80% and the **DIST** to 100%. The adaptive makeup gain will ensure the bass remains punchy without blowing out your speakers.

## Operating Considerations

- **Adaptive Level Matching:** The distortion core continuously tracks a smoothed power envelope of both the clean VCA output and the wet distorted output, automatically adjusting a makeup gain so that the two match. You can sweep the MIX knob from 0 to 100% without experiencing drastic volume drops or spikes. The tracking is deliberately unhurried where program material is concerned, so that the module levels the *effect* rather than compressing the performance; it switches to a fast correction only while a distortion control is actually being moved, which is what keeps a quick sweep of the DIST knob — or an LFO patched to DIST CV — from producing an audible jump in level.
- **Programme-Dependent Bite:** With the VCA opened past roughly nine-tenths of its travel and a hot signal present, a small additional gain and pre-drive boost are introduced automatically, so that a loud passage through a wide-open amplifier bites a little harder than a quiet one. The effect is subtle by design and follows the signal rather than any front-panel control; it is mentioned here only so that the resulting slight change in character is not mistaken for a fault.
- **Distortion Bypassing:** When the DIST parameter (knob + CV) drops effectively to zero, the distortion engine is entirely bypassed to save CPU resources.
- **Type Smoothing:** Switching distortion types (either via the knob or CV) is crossfaded over 12 milliseconds to prevent audible clicks or pops during live performance.
- **Oversampling as a Tonal Control:** The oversampling setting is not purely a quality option. Wave Fold in particular changes substantially with it — a bright, high-frequency input can measure some 16 dB louder at 1x than at 8x, because the lower setting is adding a great deal of aliasing energy that the higher one correctly removes. Ring Mod shifts by around 2 dB and the remaining topologies by less than 1.5 dB. If a patch is recalled sounding thinner or thicker than remembered, confirm the oversampling setting before suspecting anything else; 4x is the recommended default, and the step from 4x to 8x buys roughly a further decibel.
