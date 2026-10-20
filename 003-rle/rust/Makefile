RUSTC ?= rustc
RUSTFLAGS ?= --edition=2021 -O -D warnings
PYTHON ?= python3
CHALLENGES_DIR ?= ../../../challenges
PROGRAM = build/oxide-rle

.DELETE_ON_ERROR:
.PHONY: all test check clean

all: $(PROGRAM)

$(PROGRAM): main.rs
	mkdir -p "$(@D)"
	$(RUSTC) $(RUSTFLAGS) "$<" -o "$@"

test: all
	$(PYTHON) "$(CHALLENGES_DIR)/003-rle/tests/test.py" "$(CURDIR)/$(PROGRAM)"

check:
	$(PYTHON) ../../tools/check_lock.py --checkout "$(CHALLENGES_DIR)"
	$(MAKE) test CHALLENGES_DIR="$(CHALLENGES_DIR)"

clean:
	$(RM) $(PROGRAM)
