# Specula — Operating Manual

*A dual-channel vintage VU meter for the VCV Rack environment — part of the Shapetaker series.*

## Description

The **Specula** module is a precise, dual-channel level monitoring utility designed to evaluate audio and control voltages with the visual warmth and ballistic response of classic analog equipment. Functioning as a transparent pass-through, it allows the operator to insert metering at any point in a signal chain without disrupting the flow of voltages. 

## Initial Operation

To monitor a signal, patch your source into the **LEFT INPUT** or **RIGHT INPUT**. The corresponding meter will immediately display the signal amplitude. The input signal is buffered and passed transparently to the corresponding **LEFT OUTPUT** or **RIGHT OUTPUT**, allowing Specula to act as an inline monitor.

## Architecture

### Metering Topology
The metering circuit employs custom ballistics engineered to emulate the mechanical inertia of traditional needle movements. A fast attack time (15ms) ensures transients are captured accurately, while a slower release time (450ms) provides a smooth, readable decay. 

The meter dial represents a range from -20 dB to +3 dB. The module is calibrated such that a standard Rack audio level of 10 Vpp (±5V peak) corresponds to 0 VU. 

### Polyphonic Evaluation
Specula supports polyphonic signals of up to 6 channels per input. When a polyphonic cable is connected, the metering circuit evaluates the maximum peak voltage present across all active channels. This ensures that clipping or high-amplitude spikes on any individual channel are visibly reflected on the meter.

### Transparent Pass-Through
The internal routing acts as a direct, uncolored buffer. Signals patched to the inputs are duplicated at the outputs with zero latency or alteration, preserving their channel count and exact voltages.

## Connections

- **LEFT / RIGHT INPUT**: Main signal inputs. Polyphonic (up to 6 channels).
- **LEFT / RIGHT OUTPUT**: Buffered duplicates of the input signals, suitable for downstream patching. 

## Calibration and Advanced Settings

Right-click the module panel to access the context menu for advanced aesthetic controls.

- **Meter Display > Screen brightness**: Adjusts the simulated illumination of the VU meter dials. By default, this is set to 62% to mimic the warm glow of incandescent panel lamps.

## Application Notes

- **Mix Bus Monitoring**: Insert Specula just before your final output module to ensure your mix levels remain within standard operating ranges and avoid unwanted clipping.
- **Polyphonic Troubleshooting**: When working with complex, multi-channel patches, placing Specula inline can quickly confirm if any specific voice or channel is exceeding expected amplitude limits.

## Operating Considerations

- **Channel Limit**: Note that polyphonic pass-through and metering are capped at 6 channels. Any channels present beyond the 6th will be discarded at the output.
