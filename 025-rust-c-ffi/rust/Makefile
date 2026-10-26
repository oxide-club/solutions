CC ?= cc
AR ?= ar
RUSTC ?= rustc
PYTHON ?= python3
CFLAGS ?= -std=c11 -O2 -Wall -Wextra -Wpedantic -Werror
CHALLENGES_DIR ?= ../../../challenges

.DELETE_ON_ERROR:
.PHONY: all test check clean format

all: build/liboxide_ffi.a build/liboxide_ffi.rlib

build/rotate.o: rotate.c oxide.h
	mkdir -p build
	$(CC) $(CPPFLAGS) $(CFLAGS) -c rotate.c -o $@

build/librotate.a: build/rotate.o
	$(AR) rcs $@ $<

build/liboxide_ffi.a build/liboxide_ffi.rlib &: lib.rs hooks.rs build/librotate.a
	$(RUSTC) --edition=2021 --crate-name oxide_ffi --crate-type staticlib --crate-type rlib -D warnings -C panic=unwind -C opt-level=2 lib.rs --out-dir build -L build -l static=rotate

test: all
	mkdir -p build/source/rust
	cp lib.rs hooks.rs build/source/rust/
	cp oxide.h rotate.c build/source/
	$(PYTHON) "$(CHALLENGES_DIR)/025-rust-c-ffi/tests/check.py" --source "$(CURDIR)/build/source" --build-dir "$(CURDIR)/build/check"

check:
	$(PYTHON) ../../tools/check_lock.py --checkout "$(CHALLENGES_DIR)"
	$(MAKE) test CHALLENGES_DIR="$(CHALLENGES_DIR)"

format:
	rustfmt --edition 2021 --check lib.rs

clean:
	$(RM) -r build
