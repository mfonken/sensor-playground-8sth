CC      = gcc
CFLAGS  = -g -Wall -Iinclude -Itest/unity
SRC     = src/A.c
UNITY   = test/unity/src/unity.c
TEST    = test/test_A.c
OUT     = build/test_A

.PHONY: test clean

test: $(OUT)
	./$(OUT)

$(OUT): $(SRC) $(UNITY) $(TEST)
	mkdir -p build
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm -rf build