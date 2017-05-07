.PHONY: $(OUT)

LIB_NAME = trace.so
BUILD_DIR = build
OUT = $(addprefix $(BUILD_DIR)/, $(LIB_NAME))

SRCS = socket.c trace.c
OBJ = $(addprefix $(BUILD_DIR)/,$(SRCS:.c=.o))

INCLUDES = -I /usr/local/include -I lib/
LIBS = -L /usr/local/lib -lhttp_parser -lpthread

CC = gcc
CFLAGS = -Wall -fPIC -shared


$(BUILD_DIR)/%.o : %.c
	$(CC) $(CFLAGS) $(INCLUDES) $(LIBS) -c $< -o $@

$(OUT): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $@

clean:
	rm $(BUILD_DIR)/*o


compile-test:
	gcc -Wall socket_test.c socket.c -I$(shell pwd)/lib -o build/socket_test -lcheck

test: compile-test
	./build/socket_test

node:
	@LD_PRELOAD=$(OUT) node apps/frontend.js & \
	LD_PRELOAD=$(OUT) node apps/backend.js && \
	fg
