# project name (generate executable with this name)
TARGET   = hsm_app.bin

CC       = gcc

# compiling flags here
CFLAGS   = -std=gnu99 -Wall -I.

LINKER   = gcc -o

# linking flags here
#LFLAGS   = -L../development/lib
LFLAGS   =

# change these to set the proper directories where each files should be

# Directories containing the source files
SRCDIR   = core example_linux

OBJDIR   = obj
BINDIR   = bin

SOURCES  := $(wildcard ../core/*.c) $(wildcard ./*.c)
OBJECTS  := $(patsubst ../core/%.c,$(OBJDIR)/%.o,$(wildcard ../core/*.c)) \
             $(patsubst ./%.c,$(OBJDIR)/%.o,$(wildcard ./*.c))
INCLUDES := $(wildcard $(SRCDIR)/*.h)
rm       = rm -f

$(BINDIR)/$(TARGET): $(OBJECTS)
	@mkdir -p $(BINDIR)
	@$(LINKER) $@ $(LFLAGS) $(OBJECTS) -lm
	@echo "Linking complete!"

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
	@$(rm) $(OBJECTS)
	@echo "Cleanup complete!" 
	@echo $(OBJECTS)

.PHONY: remove
remove: clean
	@$(rm) $(BINDIR)/$(TARGET)
	@echo "Executable removed!"
