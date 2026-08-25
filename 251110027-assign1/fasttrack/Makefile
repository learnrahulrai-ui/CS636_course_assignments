# Variables
CXX := clang++
OPT := opt
LLVM_CONFIG := llvm-config

# Flags retrieved from llvm-config
# Note: We filter out some flags that might conflict or be unnecessary for simple plugins
CXXFLAGS := $(shell $(LLVM_CONFIG) --cxxflags)
LDFLAGS := $(shell $(LLVM_CONFIG) --ldflags)

# Project Files
TARGET_SRC ?= codes/racy1.cpp
PASS_SRC := 251110027_llvmpass.cpp
RUNTIME_SRC := 251110027_fasttrack.cpp

# Output Names
PLUGIN := llvmpass.so
PASS_NAME := fasttrack-pass
EXECUTABLE := race_detector

# Intermediate Files
TARGET_BC := main.bc
TARGET_INST_BC := main_inst.bc
RUNTIME_OBJ := fasttrack.o


# Default Target: Build everything
all: $(EXECUTABLE)

# 1. Compile the Runtime Library (fasttrack.cpp -> fasttrack.o)
$(RUNTIME_OBJ): $(RUNTIME_SRC)
	@echo "Compiling Runtime..."
	$(CXX) -c $(RUNTIME_SRC) -o $(RUNTIME_OBJ)

# 2. Build the LLVM Pass Plugin (InstrumentPass.cpp -> InstrumentPass.so)
$(PLUGIN): $(PASS_SRC)
	@echo "Building LLVM Pass..."
	$(CXX) -fPIC -shared $(PASS_SRC) -o $(PLUGIN) $(CXXFLAGS) $(LDFLAGS)

# 3. Emit LLVM IR for the Target (main.cpp -> main.bc)
$(TARGET_BC): $(TARGET_SRC)
	@echo "Generating LLVM IR..."
	$(CXX) -emit-llvm -O0 -fno-inline -fno-builtin -fno-omit-frame-pointer \
	       -c $(TARGET_SRC) -o $(TARGET_BC) -g

# 4. Instrument the IR using 'opt' (main.bc -> main_inst.bc)
$(TARGET_INST_BC): $(TARGET_BC) $(PLUGIN)
	@echo "Running Instrumentation Pass..."
	$(OPT) -load-pass-plugin=./$(PLUGIN) \
	       -passes="default<O0>,$(PASS_NAME)" \
	       $(TARGET_BC) -o $(TARGET_INST_BC)

# 5. Link everything together (main_inst.bc + fasttrack.o -> executable)
$(EXECUTABLE): $(TARGET_INST_BC) $(RUNTIME_OBJ)
	@echo "Linking Final Executable..."
	$(CXX) $(TARGET_INST_BC) $(RUNTIME_OBJ) -o $(EXECUTABLE) -lpthread

# Run the program
run: $(EXECUTABLE)
	@echo "Running Race Detector..."
	./$(EXECUTABLE)

# Clean up all generated files
clean:
	@echo "Cleaning up..."
	rm -f $(PLUGIN) $(TARGET_BC) $(TARGET_INST_BC) $(RUNTIME_OBJ) $(EXECUTABLE) *.o *.so *.bc