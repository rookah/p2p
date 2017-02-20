CC := gcc
CFLAGS := -c -Wall -Wextra -Wno-unused-parameter -Wno-unused-function

SRCDIR := src
SOURCES := p2p.c socklib.c
SOURCES := $(SOURCES:%=$(SRCDIR)/%)

OBJDIR := obj
OBJECTS := $(SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)

EXEC := p2p


all: $(SOURCES) $(EXEC)

$(EXEC): $(OBJECTS) 
	$(CC) $(OBJECTS) -o $@

$(OBJECTS): $(OBJDIR)/%.o : $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@
	@echo "Compiled "$<" successfully!"

clean:
	rm $(EXEC) $(OBJECTS)


.PHONY: all clean
