CC ?= gcc
CFLAGS ?= -Wall -Wextra -std=gnu99 -D_POSIX_C_SOURCE=200809L -I.. -I./stubs
LDFLAGS ?= 

TESTS := test_fs test_path test_ini_parser

.PHONY: all clean test

all: $(TESTS)

test_fs: test_fs.c ../src/utils/fs.c ../src/utils/fs.h ../src/utils/utils.h
	$(CC) $(CFLAGS) -o $@ test_fs.c ../src/utils/fs.c $(LDFLAGS)

test_path: test_path.c ../src/menu/path.c ../src/menu/path.h
	$(CC) $(CFLAGS) -o $@ test_path.c ../src/menu/path.c $(LDFLAGS)

test_ini_parser: test_ini_parser.c ../src/menu/ini_parser.c ../src/menu/ini_parser.h
	$(CC) $(CFLAGS) -o $@ test_ini_parser.c ../src/menu/ini_parser.c $(LDFLAGS)

test: $(TESTS)
	@for t in $(TESTS); do \
		echo "Running $$t..."; \
		./$$t || exit 1; \
	done

clean:
	rm -f $(TESTS) *.o
