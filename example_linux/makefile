# project name (generate executable with this name)
TARGET       = app.bin
WP_TARGET    = hsm_worker_example.bin
WP_TEST      = worker_pool_test

CC       = gcc

# Platform selection - can be overridden: make PLATFORM=rtos
PLATFORM ?= linux

# compiling flags here
CFLAGS   = -std=gnu99 -Wall -I. -I../core -I../platform -I../platform/$(PLATFORM)

LINKER   = gcc -o

# linking flags here
LFLAGS   = -lrt -lpthread

# change these to set the proper directories where each files should be

# Directories containing the source files
OBJDIR   = obj
BINDIR   = bin

# Core objects (platform independent)
CORE_SOURCES := $(wildcard ../core/*.c)
CORE_OBJECTS := $(patsubst ../core/%.c,$(OBJDIR)/%.o,$(CORE_SOURCES))

# Platform-specific objects
PLATFORM_SOURCES := $(wildcard ../platform/$(PLATFORM)/*.c)
PLATFORM_OBJECTS := $(patsubst ../platform/$(PLATFORM)/%.c,$(OBJDIR)/%.o,$(PLATFORM_SOURCES))

# Original example objects
SIM_SOURCES  := hsm_smuilate.c
SIM_OBJECTS  := $(patsubst %.c,$(OBJDIR)/%.o,$(SIM_SOURCES))

# Worker pool example objects
WP_SOURCES   := hsm_woker_example.c
WP_OBJECTS   := $(patsubst %.c,$(OBJDIR)/%.o,$(WP_SOURCES))

# Worker pool test objects
WP_TEST_SOURCES := worker_pool_test.c
WP_TEST_OBJECTS := $(patsubst %.c,$(OBJDIR)/%.o,$(WP_TEST_SOURCES))

INCLUDES := $(wildcard $(SRCDIR)/*.h)
rm       = rm -f

# Default target: build original example
all: $(BINDIR)/$(TARGET)

# Original HSM simulation example
$(BINDIR)/$(TARGET): $(CORE_OBJECTS) $(PLATFORM_OBJECTS) $(SIM_OBJECTS)
	@mkdir -p $(BINDIR)
	@$(LINKER) $@ $(LFLAGS) $(CORE_OBJECTS) $(PLATFORM_OBJECTS) $(SIM_OBJECTS) -lm
	@echo "Linking $(TARGET) complete!"

# Worker pool example
worker: $(BINDIR)/$(WP_TARGET)

$(BINDIR)/$(WP_TARGET): $(CORE_OBJECTS) $(PLATFORM_OBJECTS) $(WP_OBJECTS)
	@mkdir -p $(BINDIR)
	@$(LINKER) $@ $(LFLAGS) $(CORE_OBJECTS) $(PLATFORM_OBJECTS) $(WP_OBJECTS) -lm
	@echo "Linking $(WP_TARGET) complete!"

# Worker pool test
worker-test: $(BINDIR)/$(WP_TEST)

$(BINDIR)/$(WP_TEST): $(CORE_OBJECTS) $(PLATFORM_OBJECTS) $(WP_TEST_OBJECTS)
	@mkdir -p $(BINDIR)
	@$(LINKER) $@ $(LFLAGS) $(CORE_OBJECTS) $(PLATFORM_OBJECTS) $(WP_TEST_OBJECTS) -lm
	@echo "Linking $(WP_TEST) complete!"

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

# Example files compilation
$(OBJDIR)/%.o : ./%.c
	@mkdir -p $(OBJDIR)
	@$(CC) $(CFLAGS) -c $< -o $@
	@echo "Compiled "$<" successfully!"

.PHONY: clean
clean:
	@$(rm) $(OBJDIR)/*.o
	@echo "Cleanup complete!"

.PHONY: remove
remove: clean
	@$(rm) $(BINDIR)/$(TARGET) $(BINDIR)/$(WP_TARGET) $(BINDIR)/$(WP_TEST)
	@echo "Executables removed!"

.PHONY: run
run: $(BINDIR)/$(TARGET)
	@./$(BINDIR)/$(TARGET)

.PHONY: run-worker
run-worker: $(BINDIR)/$(WP_TARGET)
	@./$(BINDIR)/$(WP_TARGET)

.PHONY: run-test
run-test: $(BINDIR)/$(WP_TEST)
	@./$(BINDIR)/$(WP_TEST)
