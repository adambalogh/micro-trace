.PHONY: $(OUT)

LIB_NAME = trace.so
BUILD_DIR = build
OUT = $(addprefix $(BUILD_DIR)/, $(LIB_NAME))

HDRS = socket.h trace.h
SRCS = socket.c trace.c
OBJ = $(addprefix $(BUILD_DIR)/,$(SRCS:.c=.o))

TESTS = socket_test.c
TEST_EXEC = $(addprefix $(BUILD_DIR)/,$(TESTS:.c=))

INCLUDES = -I /usr/local/include -I lib/
LIBS = -L /usr/local/lib -lhttp_parser -lpthread

CC = gcc
CFLAGS = -Wall
LIBFLAGS = -fPIC -shared

$(OUT): $(OBJ) $(HDRS)
	$(CC) $(CFLAGS) $(LIBFLAGS) $(OBJ) -o $@

$(BUILD_DIR)/%.o : %.c
	$(CC) $(CFLAGS) $(LIBFLAGS) $(INCLUDES) $(LIBS) -c $< -o $@

$(BUILD_DIR)/% : %.c $(OBJ)
	$(CC) $(CFLAGS) $(INCLUDES) $(OBJ) $(LIBS) -ldl  $< -o $@

clean:
	rm $(BUILD_DIR)/*

test: $(TEST_EXEC) ./lib/unittest.h
	@./build/socket_test

node:
	@LD_PRELOAD=$(OUT) node apps/frontend.js & \
	LD_PRELOAD=$(OUT) node apps/backend.js && \
	fg
