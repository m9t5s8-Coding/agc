BUILD_DIR := build
EXE_NAME := agc

all: build_install

configure:
	@cmake $(BUILD_DIR)

build: configure
	@cmake --build $(BUILD_DIR)

install: 
	@sudo cmake --install build

build_install: build install

