# Vintage Slider Components

Vintage-inspired slider controls designed to match the Shapetaker plugin aesthetic. Features chrome/silver metallic handles with brass detailing and recessed metal track channels, inspired by vintage radio equipment and oscilloscopes.

## Files

### Horizontal Sliders
- **`vintage_slider_handle_horizontal.svg`** - Horizontal slider handle (40x24px)
  - Same design as vertical, rotated orientation

- **`vintage_slider_track_horizontal.svg`** - Horizontal slider track (120x14px)
  - Same design as vertical, rotated orientation

## Design Features

### Materials
- **Chrome/Silver**: Main handle body with realistic metallic gradients
- **Brass**: Center decorative stripe with vintage patina
- **Dark Steel**: Recessed track channel with shadow depth
- **Panel Metal**: Dark gray panel surface surrounding track

### Vintage Details
- Knurled grip texture (horizontal lines on handle)
- Wear marks in center of track (lighter area from use)
- Subtle scratches and aging on track
- Realistic shadows and highlights
- Edge beveling for 3D depth

## Usage in VCV Rack

### Basic Implementation
```cpp
// In your widget constructor
addParam(createParam<VintageSlider>(
    mm2px(Vec(x, y)),
    module,
    MODULE::PARAM_ID
));
```

### Horizontal Slider
```cpp
struct VintageSliderHorizontal : SvgSlider {
    VintageSliderHorizontal() {
        setBackgroundSvg(
            APP->window->loadSvg(
                asset::plugin(pluginInstance,
                "res/sliders/vintage_slider_track_horizontal.svg")
            )
        );
        setHandleSvg(
            APP->window->loadSvg(
                asset::plugin(pluginInstance,
                "res/sliders/vintage_slider_handle_horizontal.svg")
            )
        );
        maxHandlePos = mm2px(Vec(-40, 0)); // Horizontal range
        minHandlePos = mm2px(Vec(40, 0));
    }
};
```

## Sizing Guidelines

### Standard Sizes
- **Horizontal Track**: 120px wide x 14px tall (adjustable width)
- **Horizontal Handle**: 40px wide x 24px tall

### Scaling
The track SVGs can be scaled vertically/horizontally to fit your panel:
- Horizontal: Adjust width in SVG viewBox (keep height at 14)

## Color Palette

Matches Shapetaker plugin theming:
- **Chrome highlights**: `#b8bcc8`, `#ffffff`
- **Chrome midtones**: `#959aa8`, `#6b707d`
- **Chrome shadows**: `#4a4e58`, `#3d4148`
- **Brass highlights**: `#d4a857`, `#c9a34d`
- **Brass midtones**: `#9a7536`, `#7a5a2a`
- **Brass shadows**: `#6b4d24`, `#3d2a12`
- **Track recess**: `#1a1d24`, `#0a0b0d`
- **Panel surface**: `#2c2e33`, `#38393d`

## Design Philosophy

These sliders follow the Shapetaker hardware-first design principles:
- Professional hardware emulation
- Realistic metallic materials
- Vintage electronics aesthetic (1940s-1970s oscilloscopes and radio equipment)
- Careful attention to shadows, highlights, and depth
- Aged/worn details for authenticity

## Notes

- The handle has a subtle drop shadow for depth
- Track includes wear marks in the center travel area
- Knurling texture provides visual grip feedback
- Brass stripe adds vintage warmth to the chrome
- All gradients use multiple stops for realistic metallic appearance
- Compatible with VCV Rack's SvgSlider component
