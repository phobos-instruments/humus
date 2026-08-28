# Humus - a small wrapper around CMake/Ninja. Run `make` to see the targets.
#
# Nothing here is required: `cmake -S engine -B engine/build -G Ninja` and
# `cmake --build engine/build` do the same job by hand.

ENGINE_DIR := engine
GENERATOR  := Ninja
BUILD_TYPE ?= Release

# ARCH - empty builds natively. Set ARCH=x86_64 on an Apple Silicon Mac to
# cross-build for Intel: Apple clang needs no extra toolchain and Rosetta 2
# runs the result. Each arch gets its own tree, so a bare `make` never
# rebuilds the world.
ARCH ?=
ifeq ($(ARCH),)
BUILD_DIR  := $(ENGINE_DIR)/build
else
BUILD_DIR  := $(ENGINE_DIR)/build-$(ARCH)
CMAKE_ARCH := -DCMAKE_OSX_ARCHITECTURES=$(ARCH)
endif

# MACOS_MIN - the deployment target. Empty uses the CMakeLists default (11.0).
# Reaching further back needs its own tree, or the cached target from the last
# configure silently wins.
MACOS_MIN ?=
ifneq ($(MACOS_MIN),)
BUILD_DIR  := $(BUILD_DIR)-min$(MACOS_MIN)
CMAKE_ARCH := $(CMAKE_ARCH) -DCMAKE_OSX_DEPLOYMENT_TARGET=$(MACOS_MIN)
endif

# Must match juce_add_gui_app(... PRODUCT_NAME ...) in engine/CMakeLists.txt.
GUI_NAME := Humus
UNAME_S  := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
GUI_BIN        := $(BUILD_DIR)/hum_gui_artefacts/$(BUILD_TYPE)/$(GUI_NAME).app/Contents/MacOS/$(GUI_NAME)
USER_PACKS_DIR := $(HOME)/Library/Humus/packs
else
GUI_BIN        := $(BUILD_DIR)/hum_gui_artefacts/$(BUILD_TYPE)/$(GUI_NAME)
USER_PACKS_DIR := $(HOME)/.config/Humus/packs
endif
CLI_BIN := $(BUILD_DIR)/hum
NINJA   := $(BUILD_DIR)/build.ninja

# Pass arguments to `run`: make run ARGS="--first-boot"
ARGS ?=
# Which pack to install: make install-pack PACK=<id>
PACK ?=

.DEFAULT_GOAL := help

.PHONY: help
help: ## Show this help (list of available commands)
	@awk 'BEGIN {FS = ":.*?## "; printf "Humus - make targets:\n\n"} \
	  /^[a-zA-Z0-9_-]+:.*?## / {printf "  \033[36m%-14s\033[0m %s\n", $$1, $$2}' \
	  $(MAKEFILE_LIST)
	@echo ""
	@echo "The command-line tool is built alongside the app:"
	@echo "  $(CLI_BIN) info    patch.hum"
	@echo "  $(CLI_BIN) render  patch.hum out.wav --seconds 10"
	@echo ""
	@echo "Intel Macs: append ARCH=x86_64 to any target (builds in $(ENGINE_DIR)/build-x86_64)"

.PHONY: configure
configure: $(NINJA) ## Configure the build (fetches JUCE on first run - needs a network)

$(NINJA):
	cmake -S $(ENGINE_DIR) -B $(BUILD_DIR) -G $(GENERATOR) -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) $(CMAKE_ARCH)

.PHONY: build
build: configure ## Build the app and the command-line tool
	cmake --build $(BUILD_DIR)

.PHONY: run
run: configure ## Build and launch the app in this terminal (ARGS="--first-boot" replays the tour)
	@# hum_gui_bundle, not hum_gui: linking leaves the executable with the
	@# LINKER's ad-hoc signature and stages nothing, so the app would run
	@# against whatever packs an older build left and, on macOS, without the
	@# camera and audio-input entitlements. The bundle target links, stages
	@# and seals.
	cmake --build $(BUILD_DIR) --target hum_gui_bundle
	$(GUI_BIN) $(ARGS)

.PHONY: pack
pack: configure ## Build the .humpack bundles (packs/sample is the tutorial)
	cmake --build $(BUILD_DIR) --target humpacks
	@echo "packs are in $(BUILD_DIR)/humpacks"

.PHONY: install-pack
install-pack: pack ## Install one built pack for the app: make install-pack PACK=<id>
	@test -n "$(PACK)" && test -f "$(BUILD_DIR)/humpacks/$(PACK).humpack" || { \
	  echo "usage: make install-pack PACK=<id>"; \
	  echo "built packs: $$(ls $(BUILD_DIR)/humpacks 2>/dev/null | sed 's/\.humpack$$//' | tr '\n' ' ')"; \
	  exit 1; }
	@# A .humpack is a zip. Clear the target first: unzip -o overwrites but
	@# never deletes, so a file the new bundle dropped would survive.
	rm -rf "$(USER_PACKS_DIR)/$(PACK)"
	mkdir -p "$(USER_PACKS_DIR)/$(PACK)"
	unzip -oq "$(BUILD_DIR)/humpacks/$(PACK).humpack" -d "$(USER_PACKS_DIR)/$(PACK)"
	@echo "installed $(PACK) to $(USER_PACKS_DIR)/$(PACK) - restart the app to see it"

.PHONY: clean
clean: ## Delete the build directory
	rm -rf $(BUILD_DIR)
