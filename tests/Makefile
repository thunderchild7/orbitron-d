CC ?= cc
CFLAGS := -std=c99 -Wall -Wextra -Werror -I../src/c -DHOST_TEST
APP_CFLAGS := -std=c99 -Wall -Wextra -Istubs -I../src/c \
	-Wno-unused-function -Wno-unused-variable -Wno-unused-const-variable \
	-Wno-unused-parameter -Wno-int-to-void-pointer-cast -Wno-void-pointer-to-int-cast

.PHONY: test clean

test: test_face_logic
	./test_face_logic
	node config_page_test.js
	$(CC) $(APP_CFLAGS) -c ../src/c/face_logic.c -o /tmp/orbitron-d-face-logic.o
	$(CC) $(APP_CFLAGS) -c ../src/c/TextWatch.c -o /tmp/orbitron-d-textwatch.o
	rm -f /tmp/orbitron-d-face-logic.o /tmp/orbitron-d-textwatch.o

test_face_logic: test_face_logic.c ../src/c/face_logic.c ../src/c/face_logic.h
	$(CC) $(CFLAGS) -o $@ test_face_logic.c ../src/c/face_logic.c

clean:
	rm -f test_face_logic
