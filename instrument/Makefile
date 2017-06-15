PWD = $(shell pwd)

LIB_NAME = trace.so
BUILD_DIR = $(PWD)/build
BENCH_DIR = $(addprefix $(BUILD_DIR)/, benchmark)
OUT = $(addprefix $(BUILD_DIR)/, $(LIB_NAME))

SRCS_DIR = $(PWD)
SRCS = socket_adapter.cc tracing.cc orig_functions.cc context.cc instrumented_socket.cc \
	  client_socket.cc server_socket.cc logger.cc common.cc
OBJ = $(addprefix $(BUILD_DIR)/,$(SRCS:.cc=.o))

BENCHMARKS = tracing_benchmark.cc
BENCHMARK_EXEC = $(addprefix $(BENCH_DIR)/,$(BENCHMARKS:.cc=))

TESTS = context_test.cc tracing_test.cc socket_adapter_test.cc
TEST_EXEC = $(addprefix $(BUILD_DIR)/,$(TESTS:.cc=))

INCLUDES = -I /usr/local/include -I $(PROTO_GEN_DIR)
LIBS = -L /usr/local/lib -ldl -pthread -lprotobuf -lpthread

CXX = g++-5
CXXFLAGS = -Wall -std=c++17 -MP -MD -fext-numeric-literals
LIBFLAGS = -fPIC
OUT_FLAGS = -fvisibility=hidden -shared -DBUILDING_LIBMICROTRACE -DHAVE_VISIBILITY
NO_WHOLE_ARCHIVE = -Wl,-no-whole-archive

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
$(BUILD_DIR)/%.o: $(SRCS_DIR)/%.cc $(PROTO_OBJ)
	$(CXX) $(CXXFLAGS) $(LIBFLAGS) $(INCLUDES) -c $< -o $@

# tracing_test build
$(BUILD_DIR)/tracing_test: $(SRCS_DIR)/test/tracing_test.cc $(OBJ)
	$(CXX) $(CXXFLAGS) -I $(SRCS_DIR) $(INCLUDES) $(subst $(BUILD_DIR)/orig_functions.o,,$(OBJ)) $(PROTO_OBJ) $< -o $@ -lgtest -lgtest_main $(LIBS) $(PROTOLIB)

# Test build
$(BUILD_DIR)/%: $(SRCS_DIR)/test/%.cc $(OBJ)
	$(CXX) $(CXXFLAGS) -I $(SRCS_DIR) $(INCLUDES) $(OBJ) $(PROTO_OBJ) $< -o $@ -lgtest -lgtest_main $(LIBS) $(PROTOLIB)

# Benchmark build
$(BUILD_DIR)/%: $(SRCS_DIR)/test/%.cc $(OUT)
	$(CXX) $(CXXFLAGS) -I $(SRCS_DIR) $(INCLUDES) $< -o $@ -lbenchmark -lpthread

clean:
	@rm -f $(BUILD_DIR)/*.o
	@rm -f $(BUILD_DIR)/*.d
	@rm -f $(BUILD_DIR)/*_test
	@rm -f $(BUILD_DIR)/*.so
	@rm -f $(PROTO_GEN_DIR)/*.pb.*

test: $(TEST_EXEC)
	@echo 'tests compiled'

benchmark: $(BENCHMARK_EXEC)
	@echo 'benchmarks compiled'

run-test: test
	./build/socket_wrapper_test
	./build/socket_callback_test
