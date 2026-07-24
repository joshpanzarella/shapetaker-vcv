# Shapetaker — pre-submission work for the VCV Rack Library

Handoff spec for an agent working in the **private dev repo**. The public release repo
(`joshpanzarella/shapetaker-vcv`) contains only Clairaudient; modules are being released
piecemeal. Fix things here, then port the result into the release repo.

Everything below was verified against the release repo's copy of the files at commit
`41e7c6d` and against VCV Rack's own source. The dev repo may have drifted — re-verify
the specific numbers before acting on them.

---

## Hard constraints

1. **The panel must render identically or better than it does today.** The owner is
   explicitly protective of the panel's appearance. Any change that touches rendered
   output must pass the A/B protocol in the appendix before it is committed.
2. **The plugin slug, `name`, and `brand` stay lowercase `shapetaker`.** This is a
   deliberate decision by the owner. Do not "fix" the capitalization.
3. Nothing here requires changing DSP behaviour. Don't touch the audio path.

---

## Already verified — do not redo this work

Checked, and clean:

- `plugin.json` is structurally valid: slug, `2.0.0` version, SPDX `GPL-3.0-or-later`,
  author/email/sourceUrl present. Module slug `Clairaudient` matches
  `createModel<ClairaudientModule, ClairaudientWidget>("Clairaudient")`.
- Tags `Oscillator`, `Dual`, `Polyphonic` all appear in Rack's official tag list
  (`VCVRack/Rack`, branch `v2`, `src/tag.cpp`).
- **Panel text is already outlined.** Rasterizing `res/panels/Clairaudient.svg` through
  nanosvg at 75 DPI (exactly what Rack does) renders every label as paths. The 23
  `<text>` elements in the file are inert — see Task 4.
- Every param, input and output is configured with a name and unit, so tooltips are
  complete. `dataToJson`/`dataFromJson` are implemented.
- Every `res/...` path referenced in `src/` exists, and every file in `res/` is
  referenced. No missing or orphaned assets.
- C++11-clean: no `std::clamp`, `make_unique`, `if constexpr`, structured bindings,
  generic lambdas, or SIMD intrinsics. No stray `printf`/`INFO` logging.

---

## How Rack actually renders this plugin (verified facts)

These drive the tasks below. Sources are `raw.githubusercontent.com/VCVRack/Rack/v2/...`.

| Fact | Where | Consequence |
|---|---|---|
| SVGs parsed at 75 DPI | `include/window/Svg.hpp` (`SVG_DPI`) | `91.44mm` → 270px = 18HP |
| nanosvg leaves `alignType` at `NSVG_ALIGN_NONE` unless the **root** `<svg>` carries `preserveAspectRatio` | `nanosvg.h` `nsvg__scaleToViewbox` | viewBox scaling is **non-uniform**; see Task 3 |
| `Window::loadImage()` caches by filename | `src/window/Window.cpp:803` | per-frame `loadImage` is a map lookup, **not** a decode |
| `Image::loadFile()` uses `NVG_IMAGE_REPEATX \| NVG_IMAGE_REPEATY` — **no `NVG_IMAGE_GENERATE_MIPMAPS`** | `src/window/Window.cpp:76` | large textures are minified by 2×2 bilinear with no mip chain → aliasing. Key to Task 1 |
| Zoom is clamped to `[2^-2, 2^2]` = 25%–400% | `src/app/RackScrollWidget.cpp:67` | max on-screen size of any asset is 4× its 1× size |
| SDK builds with `-std=c++11 -Wall -Wextra -Wno-unused-parameter` | `compile.mk:19,21` | C++14+ syntax fails the library build |

---

## Task 1 — Shrink `res/panels/panel_background.png` (highest value)

### Current state

- `2880 × 4553` RGBA, **8.65 MB on disk**, **52.5 MB of VRAM** once decoded.
- Only 330 distinct RGB values and 496 distinct RGBA values; 99.96% of pixels are fully
  opaque, with 136 distinct alpha values in the remaining 0.04%.
- It is the single largest thing in the plugin, shipped ×3 platforms in the library build.

### Why downscaling does not hurt the look — and probably helps

`ShapetakerModuleWidget::draw()` (in `src/plugin.hpp`, ~line 3596) tiles the texture at:

```
tileH = box.size.y + 2*BG_INSET  = 379.43 + 4 = 383.43 px
tileW = tileH * (2880/4553)      = 242.5 px
```

So at 1× zoom the texture is drawn at **242 × 383 px**, and at Rack's maximum 400% zoom
at **970 × 1534 px**. The source is 2880 px wide. Rack uploads it **without mipmaps**, so
the GPU is minifying ~12:1 using 2×2 bilinear taps — it samples roughly 1 texel in 144 and
throws the rest away. Most of that 8.65 MB never reaches a pixel, and the grain shimmers
when panning or zooming.

Measured, comparing a Lanczos-downscaled 1024 px source against the current 2880 px source
(RMSE in 0–255 units, over the RGB channels):

| Condition | RMSE | Reading |
|---|---|---|
| 1× zoom, **current** source vs. ideally-filtered reference | 4.54 | today's baseline |
| 1× zoom, **1024 px** source vs. ideally-filtered reference | **3.84** | *closer* to correct than today |
| 400% zoom, 1024 px source vs. current source | 2.83 | ~1% — visually indistinguishable |

A side-by-side crop at 400% zoom is indistinguishable by eye. The 1024 px source is not a
compromise; at normal zoom it is a mild improvement, because the filtering happens offline
with a proper kernel instead of on the GPU with 4 taps.

### What to do

1. Produce `res/panels/panel_background.png` at **1024 × 1619** (preserves the 2880/4553
   aspect to within 0.01%), using a **Lanczos** resample, keeping the RGBA alpha channel:

   ```python
   from PIL import Image
   src = Image.open('res/panels/panel_background.png').convert('RGBA')
   src.resize((1024, 1619), Image.LANCZOS).save('panel_background_new.png', optimize=True)
   ```

   Result: **1.23 MB on disk** (from 8.65 MB), **6.6 MB VRAM** (from 52.5 MB).
   Run `oxipng -o4` afterwards if available — it is lossless and will shave more.

2. Update the constant in `src/plugin.hpp`:

   ```cpp
   static constexpr float BG_TEXTURE_ASPECT = 1024.f / 1619.f;  // panel_background.png
   ```

   Recompute it from the new file's real dimensions — do not leave `2880.f / 4553.f`.
   Getting this wrong stretches the tile and shifts the seam-softening pass.

3. If the dev repo still has the master/source artwork for this texture, **keep the
   full-resolution original out of the plugin's `res/`** — the library ships everything
   under `DISTRIBUTABLES`.

### Acceptance test

Build, open Rack, and compare against a pre-change screenshot at **100% and 400% zoom**,
with two modules side by side so the horizontal tiling seam is visible. The seam must not
become more visible, and the grain must not look blurred at 400%. If either fails, step up
to 1440 × 2277 (still only 13 MB VRAM, ~2.4 MB on disk) and re-test.

---

## Task 2 — Hoist the per-frame image lookup (optional, no visual change)

`ShapetakerModuleWidget::draw()` calls:

```cpp
std::shared_ptr<Image> bg = APP->window->loadImage(asset::plugin(pluginInstance, "res/panels/panel_background.png"));
```

every frame. This does **not** re-decode the PNG — `Window::loadImage` is cached by
filename — but it does build a `std::string` path and hash-lookup it once per module per
frame. Cache the `std::shared_ptr<Image>` in a member, or make the path a
`static const std::string`. Low priority; it is a micro-optimisation, not a bug.

---

## Task 3 — The panel SVG's viewBox/width mismatch

### What is going on

`res/panels/Clairaudient.svg` declares:

```
width="91.44mm"  height="128.5mm"  viewBox="0 0 101.6 128.5"
```

91.44 mm is 18 HP; the viewBox is 101.6 units wide, which is 20 HP. The artwork was drawn
at 20 HP and is being displayed at 18 HP. This works **only** because nanosvg leaves
`alignType` at `NSVG_ALIGN_NONE` when the root `<svg>` has no `preserveAspectRatio`
attribute, so it scales x and y independently:

```
sx = 270.00 / 101.6 = 2.6575     sy = 379.43 / 128.5 = 2.9527
```

`ClairaudientWidget` compensates with a hardcoded `center.x *= 0.9f` (around
`src/clairaudient.cpp:1245`) applied to every control position read out of the SVG, since
`PanelSVGParser` returns raw file coordinates and `mm2px()` uses the y scale. Verified: for
`freq_v` (raw `cx="17" cy="19.981817"`), nanosvg puts the marker centre at x=45.18, y=59.0,
and the widget code computes exactly 45.18, 59.0. They agree today.

### Why it is worth fixing

- It is **silently fragile**. Adding `preserveAspectRatio` to the root `<svg>` — which
  Inkscape, SVGO, and most "optimise this SVG" tools may do — switches nanosvg to uniform
  MEET scaling, which would shrink the artwork to 2.6575 in both axes and letterbox it by
  ~19 px top and bottom while every control stays where it is. Total misalignment, with no
  compile error and no warning.
- The x axis of the file is not in millimetres while the y axis is, so any future panel
  work has to remember the 0.9.
- Circles in the artwork render as 10%-squashed ellipses (visible on the L/R jack rings).

### Option A — leave the geometry, pin it down (low risk)

Add a comment block at the top of the SVG and next to the `* 0.9f` explaining the coupling,
and add a build-time or test-time assertion that the root `<svg>` has no
`preserveAspectRatio` attribute. Cheapest, keeps rendering bit-identical.

### Option B — bake the 0.9 into the file (clean fix)

**The current look is the squashed look**, so baking the squash into the coordinates
preserves the appearance exactly — this is a coordinate-space change, not a design change.

1. Multiply every x coordinate in the drawing content by 0.9. Do this by *baking*, not by
   wrapping content in `<g transform="scale(0.9,1)">` — `PanelSVGParser` only understands
   `translate()` and `matrix()` offsets and ignores scale, so a wrapper group would leave
   the parser returning unscaled x values.
2. Set `viewBox="0 0 91.44 128.5"`, keep `width="91.44mm" height="128.5mm"`.
3. Delete the `center.x *= 0.9f` lambda in `ClairaudientWidget` and use the parser's
   `centerPx` directly.
4. Stroke widths and circle radii scale non-uniformly under a 0.9 x-squash. A true bake
   turns circles into ellipses and gives strokes direction-dependent widths — which is
   what is *already on screen today*, so it is correct, but it means the file can no longer
   be edited as if it were round. Note this in the SVG header comment.

**Acceptance test for Option B (mandatory):** the rasterised panel before and after must
match to within antialiasing noise, and every marker centre reported by the harness in the
appendix must move by less than 0.5 px. If either fails, revert to Option A.

---

## Task 4 — Strip Inkscape leftovers from the panel SVG

`res/panels/Clairaudient.svg` is 277 KB and carries, all inside `<defs>` and all inert in
Rack:

- **23 `<text>` elements** — leftovers from a component template ("Large Knob", "20mm ⌀",
  "4HP Module Outline", …). nanosvg skips `<defs>`, so they do not render, but they are the
  only remaining reference to the fonts `Neokurat` and `Routed Gothic Narrow`.
- a `<style>` block defining `.st-text` / `.st-stroke`, referenced by nothing (there are
  zero `class=` attributes in the file — and nanosvg does not support CSS classes anyway).
- two unused `<marker>` definitions (`Arrow2Sstart`, `Arrow2Send`).

Remove them. Do **not** run a general-purpose SVG optimiser over the file — see Task 3;
several of them add `preserveAspectRatio` or rewrite transforms. Hand-edit or script the
specific deletions, then re-run the render check.

Keep the `display:none` marker elements (`freq_v`, `oscope_screen`, `formant_depth`, …) —
those are load-bearing; `PanelSVGParser` reads control positions out of them.

---

## Task 5 — IP audit (owner input required)

"Do not misuse intellectual property" is one of only two hard rules VCV states for library
inclusion, and the plugin is GPL-3.0-or-later, so every shipped asset must be
redistributable under those terms. Inventory and confirm:

- **`Neokurat`** and **`Routed Gothic Narrow`** — the panel labels are already outlined, so
  no font file ships, but outlining a font still requires that its licence permit derivative
  distribution. Routed Gothic is freely licensed; **Neokurat's licence needs checking**.
- **`res/panels/panel_background.png`** — the leather texture. Confirm provenance and that
  its licence allows redistribution in a GPL work. If it came from a stock site, check
  whether the licence permits redistribution as part of a source-available product.
- Same audit for every knob/jack/meter SVG in `res/` if any were derived from third-party
  artwork.

The agent should produce the inventory; the owner decides.

---

## Task 6 — Build clean against the current SDK

Could not be done in the environment this spec was written in (`vcvrack.com` was
unreachable). Must be done locally before submission:

```sh
export RACK_DIR=/path/to/Rack-SDK        # 2.6.x
make clean && make dist
```

- The SDK compiles with `-std=c++11 -Wall -Wextra`. Treat new warnings as blockers — VCV
  builds this on macOS (x64 + arm64), Windows, and Linux, and anything platform-specific
  surfaces there, not here.
- **Relevant to the dev repo specifically:** the release repo is C++11-clean, but it only
  contains Clairaudient. Run the same C++11 scan over the not-yet-released modules before
  they get promoted — `std::clamp`, `std::make_unique`, `if constexpr`, structured
  bindings, and generic lambdas are the usual offenders and they will fail the library
  build even though they compile fine against a local toolchain defaulting to C++17.
- Then load the module in Rack: add it, save and reload a patch, right-click through the
  context menu, and check that every knob tooltip has a name and unit.

---

## Appendix A — Render the panel exactly as Rack does

This is the harness used for every claim above. It uses upstream nanosvg with Rack's
75 DPI, so it reproduces Rack's parse semantics including the non-uniform viewBox scaling.

```sh
curl -sSLO https://raw.githubusercontent.com/memononen/nanosvg/master/src/nanosvg.h
curl -sSLO https://raw.githubusercontent.com/memononen/nanosvg/master/src/nanosvgrast.h
```

```c
// render.c  —  gcc -O1 -o render render.c -lm && ./render panel.svg out.ppm
#include <stdio.h>
#include <stdlib.h>
#define NANOSVG_IMPLEMENTATION
#include "nanosvg.h"
#define NANOSVGRAST_IMPLEMENTATION
#include "nanosvgrast.h"
int main(int argc, char** argv) {
    NSVGimage* img = nsvgParseFromFile(argv[1], "px", 75.0f);   // Rack's SVG_DPI
    float s = 2.0f;                                             // supersample
    int w = (int)(img->width * s), h = (int)(img->height * s);
    printf("image: %.2f x %.2f px (%.3f HP)\n", img->width, img->height, img->width / 15.0f);
    unsigned char* buf = malloc(w * h * 4);
    for (int i = 0; i < w * h; i++) { buf[i*4]=40; buf[i*4+1]=40; buf[i*4+2]=48; buf[i*4+3]=255; }
    NSVGrasterizer* r = nsvgCreateRasterizer();
    nsvgRasterize(r, img, 0, 0, s, buf, w, h, w * 4);
    FILE* f = fopen(argv[2], "wb");
    fprintf(f, "P6\n%d %d\n255\n", w, h);
    for (int i = 0; i < w * h; i++) fwrite(buf + i*4, 1, 3, f);
    fclose(f);
    return 0;
}
```

For Task 3, dump every shape's post-scaling bounds by walking `img->shapes` and printing
`shape->id` with `shape->bounds[0..3]`; diff that listing before and after any SVG edit.
Every control marker must stay within 0.5 px.

## Appendix B — A/B protocol for texture changes

```python
from PIL import Image
import numpy as np
src  = Image.open('res/panels/panel_background.png').convert('RGBA')   # before
new  = Image.open('panel_background_new.png').convert('RGBA')          # after
tw, th   = 242, 383      # on-screen tile at 100% zoom
tw4, th4 = 970, 1534     # on-screen tile at 400% zoom (Rack's max)
def a(i): return np.asarray(i.convert('RGB'), dtype=np.float32)
def rmse(x, y): return float(np.sqrt(((a(x) - a(y)) ** 2).mean()))
# NEAREST approximates the GPU's mipmap-less minification; LANCZOS is the ideal reference
print('1x  vs ideal, before:', rmse(src.resize((tw, th), Image.NEAREST), src.resize((tw, th), Image.LANCZOS)))
print('1x  vs ideal, after :', rmse(new.resize((tw, th), Image.NEAREST), src.resize((tw, th), Image.LANCZOS)))
print('4x  after vs before :', rmse(new.resize((tw4, th4), Image.BILINEAR), src.resize((tw4, th4), Image.NEAREST)))
```

Pass criteria: the "after" 1× figure must not be worse than the "before" figure, and the
4× figure must stay under ~4.0. Always confirm by eye as well — crop the same 300 px region
from both 4× renders and view them side by side.

---

## Release-repo checklist (do these in `joshpanzarella/shapetaker-vcv`, not the dev repo)

Not code changes — the remaining gate before opening the submission issue:

1. **Confirm `pluginUrl` and `manualUrl` resolve.** `https://shapetaker.com` and
   `https://shapetaker.com/modules/clairaudient` could not be reached from the sandbox this
   was written in (DNS resolved to Cloudflare; the proxy blocked the fetch). If the module
   page is not live yet, point `manualUrl` at `docs/manuals/Clairaudient.md` on GitHub
   instead — VCV does check these links.
2. **Tag the release.** The repo currently has **no git tags**. The library builds from a
   commit hash you supply; tag `v2.0.0` at the submitted commit so the build is
   reproducible.
3. **Open exactly one issue** at <https://github.com/VCVRack/library/issues>, titled with
   the plugin slug (`shapetaker`). Comment with the plugin name, licence, URLs, and — for
   open source — the version number and commit hash to build. That thread is the permanent
   channel: every future release is a version bump in `plugin.json`, a push, and a new
   comment with the version and hash. No `shapetaker.json` exists in their `manifests/`
   directory yet, so the slug is free.
