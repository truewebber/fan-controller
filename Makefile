PROJECT := $(shell pwd)

NATIVE_LIBS  := $(PROJECT)/.pio/libdeps/native
NATIVE_CMD   := c++ -std=c++17 \
    -I$(PROJECT)/include \
    -I$(PROJECT)/src \
    -I$(NATIVE_LIBS)/ArduinoJson/src \
    -I$(NATIVE_LIBS)/Unity/src \
    -c test_main.cpp

TEST_DIR     := $(PROJECT)/test/test_config_parse

.PHONY: test build-config compile-commands help

help:
	@echo "Targets:"
	@echo "  test              Run native C++ unit tests"
	@echo "  build-config      Convert config/config.yaml → data/config.json"
	@echo "  flash-fs          Build and upload LittleFS image (config.json) to ESP"
	@echo "  compile-commands  Regenerate compile_commands.json (ESP + native test)"

test:
	platformio test -e native

build-config:
	bash tools/config_to_json.sh

flash-fs: build-config
	platformio run -t buildfs -e esp12f
	platformio run -t uploadfs -e esp12f

compile-commands: _compiledb-esp _compiledb-native

_compiledb-esp:
	platformio run -t compiledb -e esp12f

_compiledb-native:
	@test -d "$(NATIVE_LIBS)/ArduinoJson" || \
	    platformio test -e native --without-uploading --without-testing
	@CMD="$(NATIVE_CMD)"; \
	jq -n \
	    --arg dir  "$(TEST_DIR)" \
	    --arg file "$(TEST_DIR)/test_main.cpp" \
	    --arg cmd  "$$CMD" \
	    '[{"directory":$$dir,"command":$$cmd,"file":$$file}]' \
	> $(TEST_DIR)/compile_commands.json
	@echo "Written: test/test_config_parse/compile_commands.json"
