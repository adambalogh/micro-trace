.PHONY: $(OUT)

LIB_NAME = trace.so
BUILD_DIR = build
OUT = $(addprefix $(BUILD_DIR)/, $(LIB_NAME))

SRCS_DIR = instrument
SRCS = instrumented_socket.cc trace.cc orig_functions.cc common.cc socket_callback.cc logger.cc
OBJ = $(addprefix $(BUILD_DIR)/,$(SRCS:.cc=.o))

TESTS = instrumented_socket_test.cc socket_callback_test.cc trace_test.cc
TEST_EXEC = $(addprefix $(BUILD_DIR)/,$(TESTS:.cc=))

INCLUDES = -I /usr/local/include -I $(PROTO_GEN_DIR)
LIBS = -L /usr/local/lib -lhttp_parser -ldl -pthread -lprotobuf -lpthread

CC = g++-5
CFLAGS = -Wall -std=c++17 -MP -MD
OVERRIDE_WARNING = -Wsuggest-override
LIBFLAGS = -fPIC -shared

PROTOC = protoc
PROTOLIB = -lprotobuf

PROTO_GEN_DIR = $(BUILD_DIR)/gen
PROTO_DIR = proto
PROTOS = request_log.proto
PROTO_OBJ = $(addprefix $(BUILD_DIR)/,$(PROTOS:.proto=.pb.o))
PROTO_GEN = $(addprefix $(PROTO_GEN_DIR)/,$(PROTOS:.proto=.pb.cc)) $(addprefix $(PROTO_GEN_DIR)/,$(PROTOS:.proto=.pb.h))

.PRECIOUS: $(PROTO_GEN)


# Shared library build
$(OUT): $(OBJ) $(PROTO_OBJ)
	$(CC) $(CFLAGS) $(LIBFLAGS) $(OBJ) $(PROTO_OBJ) -o $@ $(LIBS)

# Protoc compile
$(PROTO_GEN_DIR)/%.pb.cc: $(PROTO_DIR)/%.proto
	$(PROTOC) --cpp_out=$(PROTO_GEN_DIR) --proto_path=$(PROTO_DIR) $<

# Compile generated protobuf
$(BUILD_DIR)/%.pb.o: $(PROTO_GEN_DIR)/%.pb.cc
	$(CC) $(CFLAGS) -fPIC -c $< -o $@ $(PROTOLIB)

-include $(addprefix $(BUILD_DIR)/,$(SRCS:.cc=.d))

# Library files build
$(BUILD_DIR)/%.o: $(SRCS_DIR)/%.cc $(PROTO_GEN)
	$(CC) $(CFLAGS) $(LIBFLAGS) $(INCLUDES) -c $< -o $@ $(LIBS) 

# Test build
$(BUILD_DIR)/%: $(SRCS_DIR)/%.cc 
	$(CC) $(CFLAGS) $(INCLUDES) $(OBJ) $< -o $@ -lgtest -lgtest_main $(LIBS) $(PROTOLIB)

clean:
	@rm -f $(BUILD_DIR)/*.o
	@rm -f $(BUILD_DIR)/*.d
	@rm -f $(BUILD_DIR)/*_test
	@rm -f $(BUILD_DIR)/*.so
	@rm -f $(PROTO_GEN_DIR)/*.pb.*

test: $(TEST_EXEC)
	@echo 'tests compiled'

run-test: test
	./build/instrumented_socket_test
	./build/socket_callback_test

export FLASK_APP=apps/backend.py

node: 
	LD_PRELOAD=$(OUT) node apps/frontend.js & LD_PRELOAD=$(OUT) node apps/backend.js & LD_PRELOAD=$(OUT) flask run && fg
