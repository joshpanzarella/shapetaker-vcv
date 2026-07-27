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
