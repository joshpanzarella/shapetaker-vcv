#!/usr/bin/env python3
"""Guard the panel SVGs against the silent-misalignment failure mode.

Rack parses panel SVGs with nanosvg at 75 DPI.  nanosvg leaves alignType at
NSVG_ALIGN_NONE unless the *root* <svg> carries a preserveAspectRatio attribute,
and NSVG_ALIGN_NONE scales x and y independently.  Some of our panels rely on
that: Clairaudient declares width="91.44mm" (18 HP) against viewBox="0 0 101.6
128.5" (20 HP), so the artwork is drawn at 20 HP and squashed to 18 HP on the x
axis, and ClairaudientWidget compensates for control positions with a hardcoded
`center.x *= 0.9f`.

If anything ever adds preserveAspectRatio to such a root <svg> — Inkscape, SVGO
and most "optimise this SVG" tools may do it — nanosvg switches to uniform MEET
scaling.  The artwork shrinks and letterboxes while every control stays put:
total misalignment, with no compile error and no warning.  This check turns that
into a build failure instead.

Run standalone, or via `make check-panels` (a prerequisite of the default target).
"""

import glob
import os
import re
import sys
import xml.etree.ElementTree as ET

SVG_NS = "{http://www.w3.org/2000/svg}"
MM_PER_HP = 5.08
# A viewBox aspect within this fraction of the width/height aspect is "uniform"
# enough that preserveAspectRatio would be a no-op.
ASPECT_TOLERANCE = 0.002


def parse_length(value):
    """Return a length in mm, or None if it is not a plain mm/px/unitless value."""
    if value is None:
        return None
    m = re.match(r"^\s*([0-9.eE+-]+)\s*([a-z%]*)\s*$", value)
    if not m:
        return None
    n, unit = float(m.group(1)), m.group(2)
    if unit in ("mm", ""):
        return n
    if unit == "px":
        return n * 25.4 / 96.0
    return None


def check(path):
    """Return (errors, warnings) for one panel SVG.

    Errors fail the build.  Warnings are printed and ignored — they flag things
    worth a look that are not the silent-misalignment trap.
    """
    errors, warnings = [], []
    try:
        root = ET.parse(path).getroot()
    except ET.ParseError as e:
        return ["%s: XML parse error: %s" % (path, e)], warnings

    if root.tag != SVG_NS + "svg":
        return ["%s: root element is %s, expected <svg>" % (path, root.tag)], warnings

    w = parse_length(root.get("width"))
    h = parse_length(root.get("height"))
    vb = root.get("viewBox")
    if w is None or h is None or not vb:
        # Nothing we can reason about; not an error in itself.
        return errors, warnings

    try:
        vx, vy, vw, vh = [float(v) for v in re.split(r"[,\s]+", vb.strip())]
    except ValueError:
        return ["%s: unparseable viewBox %r" % (path, vb)], warnings
    if vw <= 0 or vh <= 0:
        return ["%s: degenerate viewBox %r" % (path, vb)], warnings

    doc_aspect = w / h
    vb_aspect = vw / vh
    non_uniform = abs(doc_aspect - vb_aspect) / vb_aspect > ASPECT_TOLERANCE

    par = root.get("preserveAspectRatio")
    if non_uniform and par is not None:
        errors.append(
            "%s: root <svg> has preserveAspectRatio=%r, but width/height "
            "(%g x %g mm, aspect %.5f) does not match viewBox (%g x %g, aspect "
            "%.5f).\n"
            "    nanosvg will switch from independent x/y scaling to uniform "
            "MEET scaling and the artwork will no longer line up with the "
            "control positions.\n"
            "    Remove the preserveAspectRatio attribute, or rebake the "
            "coordinates so the two aspects agree."
            % (path, par, w, h, doc_aspect, vw, vh, vb_aspect)
        )

    # Panels are whole numbers of HP; a fractional width usually means the file
    # was resized by hand and the layout code has not been told.  Advisory only —
    # a few in-progress panels are already off, and that is not this check's job.
    hp = w / MM_PER_HP
    if abs(hp - round(hp)) > 0.01:
        warnings.append(
            "%s: width %gmm is %.3f HP, not a whole number of HP" % (path, w, hp)
        )

    return errors, warnings


def main():
    here = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    paths = sorted(glob.glob(os.path.join(here, "res", "panels", "*.svg")))
    if not paths:
        print("check_panel_svgs: no panels found under res/panels/", file=sys.stderr)
        return 1

    errors, warnings = [], []
    for p in paths:
        e, w = check(os.path.relpath(p, here))
        errors += e
        warnings += w

    for w in warnings:
        print("check_panel_svgs: warning: " + w, file=sys.stderr)

    if errors:
        print("check_panel_svgs: FAILED\n", file=sys.stderr)
        for e in errors:
            print("  " + e, file=sys.stderr)
        return 1

    print("check_panel_svgs: %d panel(s) OK" % len(paths))
    return 0


if __name__ == "__main__":
    sys.exit(main())
