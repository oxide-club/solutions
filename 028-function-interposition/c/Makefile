CC ?= cc
CFLAGS ?= -std=c11 -O0 -g -fno-lto -Wall -Wextra -Wpedantic -Werror
CPPFLAGS += -D_POSIX_C_SOURCE=200809L
PYTHON ?= python3
CHALLENGES_DIR ?= ../../../challenges
ARTIFACTS = build/libcalc.so build/libtrace.so build/app build/direct

.DELETE_ON_ERROR:
.PHONY: all test check clean symbols
all: $(ARTIFACTS)

build:
	mkdir -p "$@"
build/libcalc.so: calc.c calc.h | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -fPIC -shared calc.c $(LDFLAGS) -o "$@"
build/libtrace.so: trace.c calc.h | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -fPIC -shared trace.c -pthread -ldl $(LDFLAGS) -o "$@"
build/app: app.c calc.h build/libcalc.so
	$(CC) $(CPPFLAGS) $(CFLAGS) -rdynamic app.c -Lbuild -Wl,-rpath,'$$ORIGIN' -lcalc -pthread $(LDFLAGS) -o "$@"
build/direct: app.c calc.c calc.h | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -rdynamic app.c calc.c -pthread $(LDFLAGS) -o "$@"
symbols: all
	readelf -Ws build/libcalc.so build/libtrace.so build/app build/direct > build/symbols.txt
	objdump -d build/direct > build/direct-disassembly.txt

test: all
	$(PYTHON) "$(CHALLENGES_DIR)/028-function-interposition/tests/test.py" "$(CURDIR)/build"
	$(PYTHON) compare.py

check:
	$(PYTHON) ../../tools/check_lock.py --checkout "$(CHALLENGES_DIR)"
	$(MAKE) test CHALLENGES_DIR="$(CHALLENGES_DIR)"

clean:
	$(RM) $(ARTIFACTS) build/symbols.txt build/direct-disassembly.txt
