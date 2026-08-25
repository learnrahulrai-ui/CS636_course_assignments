PROB1_DIR = problem1
PROB2_DIR = problem2

.PHONY: default help all problem1 problem2 clean

default: help

# --- Help Message ---
help:
	@echo "Run below commands to get helper messages and cd problem1 or problem2 to compile and run the target programs."
	@echo "  make all      : View all commands"
	@echo "  make problem1 : View commands of problem 1"
	@echo "  make problem2 : View commands of problem 2"
	@echo "  make clean    : Clean all directories"

all: problem1 problem2 

# Command to build problem 1
problem1:
	$(MAKE) -C $(PROB1_DIR)

# Command to build problem 2
problem2:
	$(MAKE) -C $(PROB2_DIR)

# Clean up all directories
clean:
	$(MAKE) -C $(PROB1_DIR) clean
	$(MAKE) -C $(PROB2_DIR) clean