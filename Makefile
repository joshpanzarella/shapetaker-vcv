RACK_DIR ?= ../Rack-SDK

FLAGS +=
CFLAGS +=
CXXFLAGS +=
LDFLAGS +=

SOURCES += $(wildcard src/*.cpp)
SOURCES += $(wildcard src/*/*.cpp)
SOURCES += $(wildcard src/*/*/*.cpp)

DISTRIBUTABLES += res
DISTRIBUTABLES += $(wildcard chord_packs)
DISTRIBUTABLES += $(wildcard presets)
DISTRIBUTABLES += $(wildcard LICENSE*)

include $(RACK_DIR)/plugin.mk

# Clairaudient's panel is drawn at 20 HP and displayed at 18 HP, which only works
# because nanosvg scales x and y independently when the root <svg> has no
# preserveAspectRatio.  Adding one silently misaligns every control.  This guard
# turns that into a build failure.
#
# It skips rather than fails whenever it cannot run, so it can never break the
# library build on VCV's machines.  Both escape hatches matter: scripts/ is
# development-only tooling that export_release.sh does not ship, while this
# Makefile is copied to the release tree verbatim, so the released build sees
# the target without the script behind it.
.PHONY: check-panels
check-panels:
	@if [ ! -f scripts/check_panel_svgs.py ]; then \
		echo "check-panels: scripts/check_panel_svgs.py not present, skipping panel SVG check"; \
	elif command -v python3 >/dev/null 2>&1; then \
		python3 scripts/check_panel_svgs.py; \
	else \
		echo "check-panels: python3 not found, skipping panel SVG check"; \
	fi

all: check-panels
dist: check-panels
