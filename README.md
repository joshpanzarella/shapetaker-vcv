# Shapetaker

Shapetaker is a collection of modules for [VCV Rack 2](https://vcvrack.com/)
focused on characterful oscillators and organic, hardware-inspired sound —
vintage-styled panels, CRT-style displays, and DSP with deliberate analog
imperfection baked in.

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
