PWD = $(shell pwd)

LIB_NAME = trace.so
BUILD_DIR = $(PWD)/build
OUT = $(addprefix $(BUILD_DIR)/, $(LIB_NAME))

SRCS_DIR = $(PWD)
SRCS = instrumented_socket.cc tracing.cc orig_functions.cc trace.cc socket_callback.cc logger.cc common.cc
OBJ = $(addprefix $(BUILD_DIR)/,$(SRCS:.cc=.o))

TESTS = instrumented_socket_test.cc socket_callback_test.cc tracing_test.cc trace_test.cc
TEST_EXEC = $(addprefix $(BUILD_DIR)/,$(TESTS:.cc=))

INCLUDES = -I /usr/local/include -I $(PROTO_GEN_DIR)
LIBS = -L /usr/local/lib -lhttp_parser -ldl -pthread -lprotobuf -lpthread

CXX = g++-5
CXXFLAGS = -Wall -std=c++17 -MP -MD
LIBFLAGS = -fPIC
OUT_FLAGS = -fvisibility=hidden -shared -DBUILDING_LIBMICROTRACE -DHAVE_VISIBILITY

PROTOC = protoc
PROTOLIB = -lprotobuf

PROTO_GEN_DIR = $(BUILD_DIR)/gen
PROTO_DIR = $(PWD)/../proto
PROTOS = request_log.proto
PROTO_OBJ = $(addprefix $(BUILD_DIR)/,$(PROTOS:.proto=.pb.o))
PROTO_GEN = $(addprefix $(PROTO_GEN_DIR)/,$(PROTOS:.proto=.pb.cc)) $(addprefix $(PROTO_GEN_DIR)/,$(PROTOS:.proto=.pb.h))

.PRECIOUS: $(PROTO_GEN)

all: $(OUT)

# Shared library build
$(OUT): $(OBJ) $(PROTO_OBJ)
	$(CXX) $(OUT_FLAGS) $(CXXFLAGS) $(LIBFLAGS) $(OBJ) $(PROTO_OBJ) -o $@ $(LIBS)

# Compile generated protobuf
$(BUILD_DIR)/%.pb.o: $(PROTO_GEN_DIR)/%.pb.cc
	$(CXX) $(CXXFLAGS) -fPIC -c $< -o $@ $(PROTOLIB)

# Protoc compile
$(PROTO_GEN_DIR)/%.pb.cc: $(PROTO_DIR)/%.proto
	$(PROTOC) --cpp_out=$(PROTO_GEN_DIR) --proto_path=$(PROTO_DIR) $<

-include $(addprefix $(BUILD_DIR)/,$(SRCS:.cc=.d))

# Library files build
$(BUILD_DIR)/%.o: $(SRCS_DIR)/%.cc $(PROTO_GEN)
	$(CXX) $(CXXFLAGS) $(LIBFLAGS) $(INCLUDES) -c $< -o $@ $(LIBS) 

# Test build
$(BUILD_DIR)/%: $(SRCS_DIR)/test/%.cc $(OUT)
	$(CXX) $(CXXFLAGS) -I $(SRCS_DIR) $(INCLUDES) $(OBJ) $(PROTO_OBJ) $< -o $@ -lgtest -lgtest_main $(LIBS) $(PROTOLIB)

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
