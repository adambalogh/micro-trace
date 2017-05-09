.PHONY: $(OUT)

LIB_NAME = trace.so
BUILD_DIR = build
OUT = $(addprefix $(BUILD_DIR)/, $(LIB_NAME))

HDRS = tracing_socket.h trace.h posix_defs.h helpers.h socket_interface.h
SRCS = tracing_socket.cc trace.cc
OBJ = $(addprefix $(BUILD_DIR)/,$(SRCS:.cc=.o))

TESTS = tracing_socket_test.cc
TEST_EXEC = $(addprefix $(BUILD_DIR)/,$(TESTS:.cc=))

INCLUDES = -I /usr/local/include -I lib/
LIBS = -L /usr/local/lib -lhttp_parser -lpthread

CC = g++
CFLAGS = -Wall -std=c++17
LIBFLAGS = -fPIC -shared

# Shared library build

$(OUT): $(OBJ) $(HDRS)
	$(CC) $(CFLAGS) $(LIBFLAGS) $(OBJ) -o $@

# Library files build

$(BUILD_DIR)/%.o : %.cc
	$(CC) $(CFLAGS) $(LIBFLAGS) $(INCLUDES) -c $< -o $@ $(LIBS) 

# Test build

$(BUILD_DIR)/% : %.cc $(OBJ)
	$(CC) $(CFLAGS) $(INCLUDES) $(OBJ) $< -o $@ $(LIBS) -lgtest -lgtest_main -ldl

clean:
	rm $(BUILD_DIR)/*

test: $(TEST_EXEC)
	@echo 'done'

run-test: test
	@./build/tracing_socket_test

node:
	@LD_PRELOAD=$(OUT) node apps/frontend.js & \
	LD_PRELOAD=$(OUT) node apps/backend.js && \
	fg
