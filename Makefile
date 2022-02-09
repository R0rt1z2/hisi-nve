ifeq (,$(findstring ndk,$(PATH)))
$(error "Please update your $PATH to include Android NDK: export PATH=$PATH:<path to>android-ndk")
endif

NDK_BUILD := NDK_PROJECT_PATH=. ndk-build NDK_APPLICATION_MK=./Application.mk

# Retrieve binary name from Android.mk
BIN := $(shell cat Android.mk | grep LOCAL_MODULE  | head -n1 | cut -d' ' -f3)

# Out folder, where binaries are built to
BIN_PATH := libs/arm64-v8a/$(BIN)

all: android

$(BIN_PATH):
	$(NDK_BUILD)

android:
	@echo "Building Android"
	$(NDK_BUILD)

clean:
	$(NDK_BUILD) clean

distclean: clean
	$(RM) -rf libs obj