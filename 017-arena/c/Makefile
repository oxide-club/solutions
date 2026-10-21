CC ?= cc
CFLAGS ?= -std=c11 -O2 -Wall -Wextra -Wpedantic -Werror
PYTHON ?= python3
CHALLENGES_DIR ?= ../../../challenges
PROGRAM = build/oxide-arena
.DELETE_ON_ERROR:
.PHONY: all test check clean
all: $(PROGRAM)
$(PROGRAM): implementation.c adapter.c api.h
	mkdir -p "$(@D)"
	$(CC) $(CPPFLAGS) $(CFLAGS) implementation.c adapter.c $(LDFLAGS) $(LDLIBS) -o "$@"
test: all
	$(CC) $(CPPFLAGS) $(CFLAGS) -I. "$(CHALLENGES_DIR)/017-arena/tests/api_test.c" implementation.c $(LDFLAGS) $(LDLIBS) -o build/api-test
	./build/api-test
	$(PYTHON) "$(CHALLENGES_DIR)/017-arena/tests/test.py" "$(CURDIR)/$(PROGRAM)"
check:
	$(PYTHON) ../../tools/check_lock.py --checkout "$(CHALLENGES_DIR)"
	$(MAKE) test CHALLENGES_DIR="$(CHALLENGES_DIR)"
clean:
	$(RM) $(PROGRAM) build/api-test build/asan-api build/asan-program
