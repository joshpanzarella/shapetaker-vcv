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
# turns that into a build failure.  It is skipped, not failed, where python3 is
# unavailable, so it can never break the library build on VCV's machines.
.PHONY: check-panels
check-panels:
	@if command -v python3 >/dev/null 2>&1; then \
		python3 scripts/check_panel_svgs.py; \
	else \
		echo "check-panels: python3 not found, skipping panel SVG check"; \
	fi

all: check-panels
dist: check-panels
