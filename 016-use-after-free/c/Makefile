CC ?= cc
CFLAGS ?= -std=c11 -O2 -Wall -Wextra -Wpedantic -Werror
PYTHON ?= python3
CHALLENGES_DIR ?= ../../../challenges
PROGRAM = build/oxide-use-after-free
.DELETE_ON_ERROR:
.PHONY: all test check clean
all: $(PROGRAM)
$(PROGRAM): implementation.c adapter.c api.h
	mkdir -p "$(@D)"
	$(CC) $(CPPFLAGS) $(CFLAGS) implementation.c adapter.c $(LDFLAGS) $(LDLIBS) -o "$@"
test: all
	$(CC) $(CPPFLAGS) $(CFLAGS) -I. "$(CHALLENGES_DIR)/016-use-after-free/tests/api_test.c" implementation.c $(LDFLAGS) $(LDLIBS) -o build/api-test
	./build/api-test
	$(PYTHON) "$(CHALLENGES_DIR)/016-use-after-free/tests/test.py" "$(CURDIR)/$(PROGRAM)"
check:
	$(PYTHON) ../../tools/check_lock.py --checkout "$(CHALLENGES_DIR)"
	$(MAKE) test CHALLENGES_DIR="$(CHALLENGES_DIR)"
clean:
	$(RM) $(PROGRAM) build/api-test build/asan-api build/asan-program

.PHONY: sanitize
sanitize:
	mkdir -p build
	$(CC) -std=c11 -g -O1 -Wall -Wextra -Wpedantic -Werror -fsanitize=address,undefined -fno-omit-frame-pointer -I. "$(CHALLENGES_DIR)/016-use-after-free/tests/api_test.c" implementation.c -o build/asan-api
	ASAN_OPTIONS=detect_leaks=0 ./build/asan-api
	$(CC) -std=c11 -g -O1 -Wall -Wextra -Wpedantic -Werror -fsanitize=address,undefined -fno-omit-frame-pointer implementation.c adapter.c -o build/asan-program
	ASAN_OPTIONS=detect_leaks=0 $(PYTHON) "$(CHALLENGES_DIR)/016-use-after-free/tests/test.py" "$(CURDIR)/build/asan-program"
