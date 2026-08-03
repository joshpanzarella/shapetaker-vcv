# Involution — Operating Manual

*A dual chaotic liquid-filter field for the VCV Rack environment — part of the Shapetaker series.*

## Description

Involution combines two morphable liquid filters into a single stereo instrument. Its two lanes can remain independent, feed one another through a bounded coupling loop, run in either serial order, or process the mid and side components of a stereo signal.

An internal Lorenz strange attractor gives the filter field organic, non-repeating motion. Separate axes move filters A and B while a third axis can animate resonance, Form, and Couple. The result can range from a stable pair of warm lowpass filters to a wide, animated bandpass field or a volatile, cross-coupled highpass texture.

## Initial Operation

1. Connect a spectrally rich signal to **Audio A**. Audio B is normalled from A when its own input is unused.
2. Connect **Audio A** and **Audio B** outputs to a stereo signal path.
3. Set both cutoff controls near noon and raise both resonance controls until their peaks are audible.
4. Turn **Spread** clockwise to separate the two cutoff frequencies symmetrically.
5. Move **Form** from lowpass through bandpass to highpass.
6. Raise **Couple** to circulate energy between the filter lanes, then add **Mod Depth** to animate the field.

## Architecture

### Dual Morphable Liquid Filters

Each lane begins with a 6th-order lowpass filter made from three cascaded 2-pole state-variable stages and a global ladder-style resonance path. The feedback path protects the low end and is bounded so high resonance and coupling can ring without numerical runaway.

The **Form** control changes the response inside the filter core:

- **0%:** Deep 6-pole lowpass.
- **50%:** Resonant bandpass.
- **100%:** 2-pole highpass.

The lowpass core remains active behind every Form position, so the resonance and liquid feedback retain a related character across the whole sweep.

With **Opposed Form** enabled, filter A follows the Form control normally while filter B moves in reverse. At 0% A is lowpass and B is highpass; both meet at bandpass at 50%; at 100% A is highpass and B is lowpass. Form CV and full-field chaos preserve this opposition.

In **Liquid** and **Volatile** character modes, input transients briefly open the cutoff while the resonant output creates a slower downward bloom. The interaction gives the filter its elastic seeking-and-settling motion. **Still** removes this level-dependent motion for a more controlled response.

### Couple

**Couple** sends the previous output of each filter into the other filter's input. The return is saturated and gain-bounded before it re-enters the filter core. Low settings add shared movement and stereo coherence; high settings create interacting resonant peaks, circulating harmonics, and controlled instability.

### Spread

**Spread** moves filter A downward and filter B upward around their respective cutoff settings. Its maximum range is three octaves, shown as the total A/B separation. Spread CV adds to the panel setting at 1 V = 10% of the range. Polyphonic voices retain a smaller internal micro-spread in addition to this panel control.

### Lorenz Filter Field

The internal Lorenz attractor supplies three related, non-repeating signals. Filters A and B receive different axes for independent cutoff motion. A third axis can move the remaining filter field in opposing directions. Voices 4 and above use inverted axes so polyphonic pairs do not share identical trajectories.

The **Chaos Destination** context-menu setting chooses how much of the field moves:

- **Cutoff:** Only the A and B cutoff frequencies move.
- **Cutoff + resonance:** Cutoff moves normally while resonance moves in opposite directions between A and B.
- **Full filter field:** Cutoff and resonance move, with subtler motion added to Form and Couple.

A slower internal drift source adds microscopic cutoff wander. Clicking the CRT freezes both chaos sources at their current values; click it again to release them. A high signal at **Freeze Gate** temporarily freezes the same field without changing the CRT/context-menu latch.

### Routing Topologies

The **Filter Routing** context menu offers four signal paths:

- **Parallel stereo:** A and B are filtered independently.
- **Serial A into B:** Filter A also drives filter B.
- **Serial B into A:** Filter B also drives filter A.
- **Mid / Side:** Filter A processes stereo mid and filter B processes stereo side before decoding back to A/B outputs.

## Connections

### Main Controls

- **Cutoff A / Cutoff B:** Set the base cutoff frequency for each filter lane. The control curve provides extra resolution through the musical midrange.
- **Resonance A / Resonance B:** Set the global resonance feedback for each lane.
- **Link Cutoff / Link Resonance:** Link each A/B parameter pair bidirectionally. The most recently moved control becomes the source.
- **Cutoff / Resonance Attenuverters:** Scale and invert the corresponding CV inputs.

### Filter-Field Controls

- **Couple:** Sets bounded cross-coupling between the two filters.
- **Spread:** Separates the A/B cutoffs by up to three octaves.
- **Mod Depth:** Sets the Lorenz modulation depth.
- **Mod Rate:** Sets the Lorenz calculation rate from 0.01 Hz to 10 Hz.
- **Form:** Morphs both filter responses from lowpass through bandpass to highpass.

### Inputs and Outputs

- **Audio A / B Inputs:** Polyphonic audio inputs. A single connected input is normalled to both lanes.
- **Audio A / B Outputs:** Polyphonic stereo outputs.
- **Cutoff A / B CV:** Attenuverted cutoff modulation or optional 1V/octave tracking.
- **Resonance A / B CV:** Attenuverted resonance modulation.
- **Rate CV:** Adds bipolar CV to Mod Rate.
- **Depth CV:** Adds CV to Mod Depth.
- **Couple CV:** Adds CV to Couple.
- **Form CV:** Adds CV to Form.
- **Spread CV:** Adds CV to Spread; 10 V spans the full three-octave separation range.
- **Freeze Gate:** A signal at or above 1 V holds both chaotic generators at their current values. The latched CRT/context-menu freeze remains independent.

## CRT

The CRT remains a unified chaotic particle field. Every filter-field control contributes to the same calculation: Couple tightens and twists the orbit, Spread separates interleaved particle bands, Form reshapes and recolors the contour, and the routing modes change its direction or symmetry. Opposed Form separates the two interleaved contours in opposite directions. Character controls the intensity of motion, while the selected Chaos Destination determines which layers respond most strongly.

Click anywhere on the CRT to freeze or release the chaos field. The Freeze Gate also pauses the CRT while held high.

## Context Menu

- **Screen Theme:** Select a local phosphor color or follow the shared Shapetaker display theme.
- **Filter Routing:** Choose Parallel, either serial order, or Mid / Side.
- **Opposed Form:** Reverse filter B's Form response around the shared bandpass midpoint.
- **Chaos Destination:** Limit chaos to cutoff, add resonance, or animate the full field.
- **Filter Character:** Choose Still, Liquid, or Volatile level-dependent behavior.
- **Freeze Chaos Field:** Hold or release the current chaotic modulation values.
- **Filter A / B 1V/oct Tracking:** Interpret the selected cutoff CV input as pitch tracking. Its attenuverter sets the tracking direction and amount.

## Operating Considerations

- **Polyphonic voice spread:** Voice 1 follows the exact panel position. Additional voices receive fixed offsets up to approximately ±1.5 semitones so resonant peaks do not stack perfectly.
- **Coupling and serial routing:** High resonance, high Couple, and either serial topology can produce large tonal changes. Internal saturation and the output guard keep these states bounded, but they are intentionally capable of ringing.
- **CV smoothing:** Panel changes and external modulation are smoothed to prevent zipper noise. Very fast modulation is therefore softened.
- **Output guard:** Normal modular signals within ±10 V pass without limiting. A soft knee catches extreme resonant peaks before they can clip the host.
