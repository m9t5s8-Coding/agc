BUILD_DIR := build
EXE_NAME := agc

all: build_install

configure:
	@if [ ! -d "$(BUILD_DIR)" ]; then cmake -B $(BUILD_DIR); fi

build: configure
	@cmake --build $(BUILD_DIR)

install: 
	@sudo cmake --install build

build_install: build install

