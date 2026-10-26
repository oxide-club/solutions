CARGO ?= cargo
PYTHON ?= python3
RUSTFLAGS ?= -D warnings
CHALLENGES_DIR ?= ../../../challenges
CHALLENGE = 024-mini-refcell

.PHONY: all test check clean prepare-tests format miri

all:
	RUSTFLAGS="$(RUSTFLAGS)" $(CARGO) build --offline --locked --examples --target-dir build/library

prepare-tests:
	mkdir -p build/check
	cp Cargo.toml Cargo.lock build/check/
	cp -R src build/check/
	cp -R "$(CHALLENGES_DIR)/$(CHALLENGE)/tests" build/check/

test: all prepare-tests
	cd build/check && RUSTFLAGS="$(RUSTFLAGS)" $(PYTHON) tests/check.py

check:
	$(PYTHON) ../../tools/check_lock.py --checkout "$(CHALLENGES_DIR)"
	$(MAKE) test CHALLENGES_DIR="$(CHALLENGES_DIR)"

format:
	$(CARGO) fmt --check

miri: prepare-tests
	cd build/check && sh tests/miri.sh

clean:
	$(RM) -r build
