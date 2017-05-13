.PHONY: $(OUT)

LIB_NAME = trace.so
BUILD_DIR = build
OUT = $(addprefix $(BUILD_DIR)/, $(LIB_NAME))

SRCS_DIR = instrument
SRCS = tracing_socket.cc trace.cc orig_functions.cc common.cc socket_event_handler.cc
OBJ = $(addprefix $(BUILD_DIR)/,$(SRCS:.cc=.o))

TESTS = tracing_socket_test.cc
TEST_EXEC = $(addprefix $(BUILD_DIR)/,$(TESTS:.cc=))

INCLUDES = -I /usr/local/include -I $(PROTO_GEN_DIR)
LIBS = -L /usr/local/lib -lhttp_parser -lpthread -ldl

CC = g++
CFLAGS = -Wall -std=c++17 -MP -MD
LIBFLAGS = -fPIC -shared

PROTO_GEN_DIR = $(BUILD_DIR)/gen
PROTO_DIR = proto
PROTOS = request_log.proto
PROTO_OBJ = $(addprefix $(BUILD_DIR)/,$(PROTOS:.proto=.pb.o))

PROTOC = protoc
PROTOLIB = -lprotobuf


# Shared library build
$(OUT): $(OBJ)
	$(CC) $(CFLAGS) $(LIBFLAGS) $(OBJ) -o $@ $(LIBS)

# Protoc compile
$(PROTO_GEN_DIR)/%.pb.cc: $(PROTO_DIR)/%.proto
	$(PROTOC) --cpp_out=$(PROTO_GEN_DIR) --proto_path=$(PROTO_DIR) $<

# Compile generated protobuf
$(BUILD_DIR)/%.pb.o: $(PROTO_GEN_DIR)/%.pb.cc
	$(CC) $(CFLAGS) -c $< -o $@ $(PROTOLIB)

-include $(addprefix $(BUILD_DIR)/,$(SRCS:.cc=.d))

# Library files build
$(BUILD_DIR)/%.o : $(SRCS_DIR)/%.cc $(PROTO_OBJ)
	$(CC) $(CFLAGS) $(LIBFLAGS) $(INCLUDES) -c $< -o $@ $(LIBS) 

# Test build
$(BUILD_DIR)/% : %.cc $(OBJ)
	$(CC) $(CFLAGS) $(INCLUDES) $(OBJ) $< -o $@ $(LIBS) -lgtest -lgtest_main -ldl

clean:
	@rm -f $(BUILD_DIR)/*.o
	@rm -f $(BUILD_DIR)/*.so
	@rm -f $(PROTO_GEN_DIR)/*.pb.*

test: $(TEST_EXEC)
	@echo 'done'

run-test: test
	@./build/tracing_socket_test

export FLASK_APP=apps/backend.py

node: 
	LD_PRELOAD=$(OUT) node apps/frontend.js & LD_PRELOAD=$(OUT) node apps/backend.js & LD_PRELOAD=$(OUT) flask run && fg
