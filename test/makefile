# OSA Unit Test Makefile

TARGET       = test_runner.bin

CC       = gcc

# Platform selection
PLATFORM ?= linux

# Include paths
CFLAGS   = -std=gnu99 -Wall -I. -I../core -I../platform -I../platform/$(PLATFORM)

LINKER   = gcc -o

# Linking flags
LFLAGS   = -lrt -lpthread

# Directories
OBJDIR   = obj
BINDIR   = bin

# Unity framework
UNITY_SOURCES := unity/unity.c
UNITY_OBJECTS := $(OBJDIR)/unity/unity.o

# Core objects
CORE_SOURCES := $(wildcard ../core/*.c)
CORE_OBJECTS := $(patsubst ../core/%.c,$(OBJDIR)/%.o,$(CORE_SOURCES))

# Platform objects
PLATFORM_SOURCES := $(wildcard ../platform/$(PLATFORM)/*.c)
PLATFORM_OBJECTS := $(patsubst ../platform/$(PLATFORM)/%.c,$(OBJDIR)/%.o,$(PLATFORM_SOURCES))

# Test objects
TEST_SOURCES := test_main.c test_error.c test_sync.c test_worker.c
TEST_OBJECTS := $(patsubst %.c,$(OBJDIR)/%.o,$(TEST_SOURCES))

rm       = rm -f

# Default target
all: $(BINDIR)/$(TARGET)

$(BINDIR)/$(TARGET): $(UNITY_OBJECTS) $(CORE_OBJECTS) $(PLATFORM_OBJECTS) $(TEST_OBJECTS)
	@mkdir -p $(BINDIR)
	@$(LINKER) $@ $(LFLAGS) $^ -lm
	@echo "Linking $(TARGET) complete!"

# Unity compilation
$(OBJDIR)/unity/unity.o : unity/unity.c
	@mkdir -p $(OBJDIR)/unity
	@$(CC) $(CFLAGS) -c $< -o $@
	@echo "Compiled "$<" successfully!"

# Core files compilation
$(OBJDIR)/%.o : ../core/%.c
	@mkdir -p $(OBJDIR)
	@$(CC) $(CFLAGS) -c $< -o $@
	@echo "Compiled "$<" successfully!"

# Platform files compilation
$(OBJDIR)/%.o : ../platform/$(PLATFORM)/%.c
	@mkdir -p $(OBJDIR)
	@$(CC) $(CFLAGS) -c $< -o $@
	@echo "Compiled "$<" successfully!"

# Test files compilation
$(OBJDIR)/%.o : ./%.c
	@mkdir -p $(OBJDIR)
	@$(CC) $(CFLAGS) -c $< -o $@
	@echo "Compiled "$<" successfully!"

.PHONY: clean
clean:
	@$(rm) -rf $(OBJDIR)/*.o $(OBJDIR)/unity/*.o
	@echo "Cleanup complete!"

.PHONY: remove
remove: clean
	@$(rm) $(BINDIR)/$(TARGET)
	@echo "Executable removed!"

.PHONY: run
run: $(BINDIR)/$(TARGET)
	@./$(BINDIR)/$(TARGET)

.PHONY: test
test: $(BINDIR)/$(TARGET)
	@./$(BINDIR)/$(TARGET)
