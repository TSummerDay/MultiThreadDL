# Check whether the third-party library uri exists
LIB_DIR = thirdparty/uri
LIB_URI_INCLUDE_DIR = $(LIB_DIR)/include
URI_LIB = $(LIB_URI_INCLUDE_DIR)/network/uri.hpp
URI_BUILD_DIR = $(LIB_DIR)/_build

uri: $(URI_LIB)

$(URI_LIB):
	@if [ ! -d $(LIB_DIR) ]; then \
		echo "Url library not found. Cloning..."; \
		git clone https://github.com/cpp-netlib/uri.git $(LIB_DIR); \
	fi
	@if [ ! -d $(URI_BUILD_DIR) ]; then \
		echo "Uri library not built. Building..."; \
		cd $(LIB_DIR); \
		mkdir _build; \
		cd _build; \
		cmake ..; \
		make -j4; \
	fi


# Compiler to use
CXX = g++

# Define the compiler flags for debug and release builds
DEBUG_FLAGS = -g -O0 -DDEBUG
RELEASE_FLAGS = -O3 -DNDEBUG

# Use the 'DEBUG' environment variable to determine which flags to use
ifeq ($(DEBUG), 1)
    CXXFLAGS = -std=c++17 -Wall -Wno-deprecated-declarations -Iinclude $(DEBUG_FLAGS)
else
    CXXFLAGS = -std=c++17 -Wall -Wno-deprecated-declarations -Iinclude $(RELEASE_FLAGS)
endif


# Libraries to link against
LDLIBS = -lcurl -lcrypto -lssl
TEST_LDLIBS = -lgtest -lgtest_main -pthread

# Source and object files
SRC_DIR = src
OBJ_DIR = obj
SRC = $(wildcard $(SRC_DIR)/*.cc)
OBJ = $(SRC:$(SRC_DIR)/%.cc=$(OBJ_DIR)/%.o)

# Test files
TEST_DIR = tests
TEST_SRC = $(wildcard $(TEST_DIR)/*.cc)
TEST_OBJ = $(TEST_SRC:$(TEST_DIR)/%.cc=$(OBJ_DIR)/%.o)
TESTS = $(TEST_OBJ:$(OBJ_DIR)/%.o=%)

# The name of the main executable
EXE = multithread_dl

all: $(EXE)

$(EXE): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDLIBS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cc
	mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -o $@ -c $<

tests: $(TESTS)

$(TESTS): %: $(OBJ_DIR)/%.o $(filter-out $(OBJ_DIR)/$(EXE).o, $(OBJ))
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDLIBS) $(TEST_LDLIBS)

$(OBJ_DIR)/%.o: $(TEST_DIR)/%.cc
	mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -o $@ -c $<

clean:
	rm -f $(EXE) $(TESTS) $(OBJ) $(TEST_OBJ)