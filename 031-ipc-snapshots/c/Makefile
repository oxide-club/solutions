CC ?= cc
CFLAGS ?= -std=c11 -O2 -g -Wall -Wextra -Wpedantic -Werror
PYTHON ?= python3
CHALLENGES_DIR ?= ../../../challenges
MATERIAL = $(CHALLENGES_DIR)/031-ipc-snapshots

.DELETE_ON_ERROR:
.PHONY: all test check clean

all: build/target build/observer build/target-test

build/target: target.c common.c common.h region.h
	mkdir -p "$(@D)"
	$(CC) $(CPPFLAGS) $(CFLAGS) target.c common.c $(LDFLAGS) -pthread $(LDLIBS) -o "$@"

build/observer: observer.c common.c common.h region.h
	mkdir -p "$(@D)"
	$(CC) $(CPPFLAGS) $(CFLAGS) observer.c common.c $(LDFLAGS) -pthread $(LDLIBS) -o "$@"

build/target-test: target.c common.c common.h region.h
	mkdir -p "$(@D)"
	$(CC) $(CPPFLAGS) $(CFLAGS) -DOX_TEST_HOOKS target.c common.c $(LDFLAGS) -pthread $(LDLIBS) -o "$@"

test: all
	$(MAKE) -C "$(MATERIAL)" build/region-probe
	$(PYTHON) "$(MATERIAL)/tests/check.py" "$(CURDIR)/build/target"
	$(PYTHON) "$(MATERIAL)/tests/check.py" --test-hooks "$(CURDIR)/build/target-test"
	$(PYTHON) "$(MATERIAL)/tests/observer.py" "$(CURDIR)/build/observer"
	./build/target --observer "$(CURDIR)/build/observer"
	./build/target --observer "$(CURDIR)/build/observer" --test-worker
	@./build/target --observer "$(CURDIR)/build/observer" --test-sequence-max >/dev/null 2>&1; test $$? -eq 2

check:
	$(PYTHON) ../../tools/check_lock.py --checkout "$(CHALLENGES_DIR)"
	$(MAKE) test CHALLENGES_DIR="$(CHALLENGES_DIR)"

clean:
	$(RM) build/target build/observer build/target-test
