# Shapetaker

Shapetaker is a collection of modules for [VCV Rack 2](https://vcvrack.com/)
focused on characterful sound sources and processors — vintage-styled panels,
CRT-style displays, and DSP with deliberate analog imperfection baked in.

## Modules

### Clairaudient

A dual sigmoid oscillator (V/Z) with morphing waveforms and stereo
crossfading.

- Two oscillators, each running a symmetric-detuned stereo pair, with
  saw-to-sigmoid waveform morphing (or PWM mode) and polyBLEP/oversampled
  anti-aliasing.
- **Voice engine**: internal audio-rate modulation of the sigmoid slope and
  transition center, ratio-locked to each oscillator's pitch — DEPTH, RATIO,
  ASYM, and a true mono-to-wide WIDTH control.
- Cross-sync and reverse sync (off / on / mutual) with a CHANCE control for
  probabilistic direction flips.
- Built-in oscilloscope display (waveform or Lissajous) with selectable CRT
  themes, equal-power or stereo-swap crossfade curves, and a Vintage
  character macro (drift, voice character, output color) in the context
  menu.

See the [Clairaudient manual](docs/manuals/Clairaudient.md).

### Chiaroscuro

A polyphonic stereo VCA fused to a five-topology distortion core.

- Stereo VCA with linear or exponential response, a LINK switch that collapses
  to mono, and headroom above unity — a positive CV against an open knob
  amplifies rather than clipping flat.
- Five distortion topologies: Hard Clip, Tube Sat, Wave Fold, Bit Crush, and
  Ring Mod, each with its own indicator colour.
- DIST, DRIVE, and MIX each have a dedicated CV input and attenuverter, with
  adaptive makeup gain holding apparent level steady as you morph from clean
  to destroyed.
- Up to 16 voices, with the voice count taken from the audio inputs alone.

See the [Chiaroscuro manual](docs/manuals/Chiaroscuro.md).

### Involution

Two morphable liquid filters and a chaotic filter field, in one stereo
instrument.

- Each lane is a 6th-order filter with a global resonance path; **Form** sweeps
  it continuously from deep 6-pole lowpass through resonant bandpass to 2-pole
  highpass, with an Opposed Form mode that runs the two lanes in mirror image.
- **Couple** circulates each filter's output into the other through a bounded,
  saturated return — shared movement at low settings, interacting resonant
  peaks and controlled instability at high ones.
- **Spread** separates the two cutoffs symmetrically across up to three
  octaves.
- An internal Lorenz attractor animates the field, with a selectable
  destination (cutoff, cutoff + resonance, or the full field), a freeze gate,
  and four routing topologies (independent, either serial order, or mid/side).

See the [Involution manual](docs/manuals/Involution.md).

### Specula

A dual vintage VU meter with transparent pass-through.

- Custom ballistics emulating mechanical needle inertia: 15 ms attack, 450 ms
  release, over a -20 dB to +3 dB dial calibrated so ±5 V peak reads 0 VU.
- Inputs are buffered straight to the outputs, so metering can be inserted
  anywhere in a chain without disturbing it.
- Polyphonic to 6 channels; the needle follows the peak across all active
  channels.

See the [Specula manual](docs/manuals/Specula.md).

### Utility Panel

A resizable blanking panel in the Shapetaker leather finish.

- Drag either edge to resize from 2 HP to 64 HP in 1 HP steps.
- Fits itself to the surrounding gap when first placed; double-click the panel
  (or use the context menu) to re-fit after the rack around it changes.
- Screws reposition themselves as the panel width changes.

See the [Utility Panel manual](docs/manuals/UtilityPanel.md).

## Building

Build like any VCV Rack plugin, with the
[Rack SDK](https://vcvrack.com/manual/Building#Building-Rack-plugins):

```sh
export RACK_DIR=/path/to/Rack-SDK
make
make install
```

## License

Source code is licensed under
[GPL-3.0-or-later](https://spdx.org/licenses/GPL-3.0-or-later.html) — see
[LICENSE](LICENSE).
