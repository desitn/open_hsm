# project name (generate executable with this name)
TARGET       = app.bin
TP_TARGET    = threadpool_example.bin

CC       = gcc

# compiling flags here
CFLAGS   = -std=gnu99 -Wall -I. -I../core

LINKER   = gcc -o

# linking flags here
LFLAGS   = -lrt -lpthread

# change these to set the proper directories where each files should be

# Directories containing the source files
SRCDIR   = core example_linux

OBJDIR   = obj
BINDIR   = bin

# Core objects (shared)
CORE_SOURCES := $(wildcard ../core/*.c)
CORE_OBJECTS := $(patsubst ../core/%.c,$(OBJDIR)/%.o,$(CORE_SOURCES))

# Original example objects
SIM_SOURCES  := hsm_smuilate.c
SIM_OBJECTS  := $(patsubst %.c,$(OBJDIR)/%.o,$(SIM_SOURCES))

# Thread pool example objects
TP_SOURCES   := hsm_threadpool_example.c
TP_OBJECTS   := $(patsubst %.c,$(OBJDIR)/%.o,$(TP_SOURCES))

INCLUDES := $(wildcard $(SRCDIR)/*.h)
rm       = rm -f

# Default target: build original example
all: $(BINDIR)/$(TARGET)

# Original HSM simulation example
$(BINDIR)/$(TARGET): $(CORE_OBJECTS) $(SIM_OBJECTS)
	@mkdir -p $(BINDIR)
	@$(LINKER) $@ $(LFLAGS) $(CORE_OBJECTS) $(SIM_OBJECTS) -lm
	@echo "Linking $(TARGET) complete!"

# Thread pool example
threadpool: $(BINDIR)/$(TP_TARGET)

$(BINDIR)/$(TP_TARGET): $(CORE_OBJECTS) $(TP_OBJECTS)
	@mkdir -p $(BINDIR)
	@$(LINKER) $@ $(LFLAGS) $(CORE_OBJECTS) $(TP_OBJECTS) -lm
	@echo "Linking $(TP_TARGET) complete!"

$(OBJDIR)/%.o : ../core/%.c
	@mkdir -p $(OBJDIR)
	@$(CC) $(CFLAGS) -c $< -o $@
	@echo "Compiled "$<" successfully!"

$(OBJDIR)/%.o : ./%.c
	@mkdir -p $(OBJDIR)
	@$(CC) $(CFLAGS) -c $< -o $@
	@echo "Compiled "$<" successfully!"

.PHONY: clean
clean:
	@$(rm) $(OBJECTS) $(CORE_OBJECTS) $(SIM_OBJECTS) $(TP_OBJECTS)
	@echo "Cleanup complete!" 

.PHONY: remove
remove: clean
	@$(rm) $(BINDIR)/$(TARGET) $(BINDIR)/$(TP_TARGET)
	@echo "Executables removed!"

.PHONY: run
run: $(BINDIR)/$(TARGET)
	@./$(BINDIR)/$(TARGET)

.PHONY: run-tp
run-tp: $(BINDIR)/$(TP_TARGET)
	@./$(BINDIR)/$(TP_TARGET)
