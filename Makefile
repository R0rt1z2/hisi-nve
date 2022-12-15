# Linux
CC := gcc
HOST_ARCH := $(shell uname -m)
CFLAGS := $(shell cat Android.mk | grep LOCAL_CFLAGS | cut -d'=' -f2)

# Android
NDK_BUILD := NDK_PROJECT_PATH=. ndk-build NDK_APPLICATION_MK=./Application.mk

# Name for the release ZIP
GIT_SHA_FETCH := $(shell git rev-parse HEAD | cut -c 1-8)
ZIP_NAME := hisi-nve-$(GIT_SHA_FETCH).zip

# Retrieve binary name from Android.mk
BIN := $(shell cat Android.mk | grep LOCAL_MODULE  | head -n1 | cut -d' ' -f3)

# Out folder, where binaries are built to
BIN_PATH := libs/arm64-v8a/$(BIN)
HOST_BIN_PATH := libs/linux-$(shell uname -m)

all: linux android

$(BIN_PATH):
	$(NDK_BUILD)

linux: hisi_nve.o sha256.o memory.o
	@echo "Building Linux"
	mkdir -p $(HOST_BIN_PATH)
	$(CC) -o $(HOST_BIN_PATH)/$(BIN) $^

android:
	@echo "Building Android"
	$(NDK_BUILD) 2> /dev/null || \
		{ if [ "$(MAKECMDGOALS)" = "all" ]; then \
			echo "Skipping Android build"; \
		else \
			echo "Android build failed: ndk-build not found"; \
			exit 1; \
		fi; }

hisi_nve.o: src/nve/hisi_nve.c src/nve/include/hisi_nve.h src/nve/include/hisi_nve.h src/crypto/include/sha256.h src/memory/include/memory.h
	$(CC) -c src/nve/hisi_nve.c -Isrc/nve/include -Isrc/crypto/include -Isrc/memory/include

sha256.o: src/crypto/sha256.c src/crypto/include/sha256.h
	$(CC) -c src/crypto/sha256.c -Isrc/crypto/include

memory.o: src/memory/memory.c src/memory/include/memory.h
	$(CC) -c src/memory/memory.c -Isrc/memory/include

release: all
	zip -r $(ZIP_NAME) libs

clean:
	$(NDK_BUILD) clean

distclean: clean
	$(RM) -rf libs obj *.zip