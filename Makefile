CC := gcc
CFLAGS := -c -Wall -Wextra -Wno-unused-parameter -Wno-unused-function -g

SRCDIR := src
SOURCES := p2p.c socklib.c
SOURCES := $(SOURCES:%=$(SRCDIR)/%)

OBJDIR := obj
OBJECTS := $(SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)

EXEC := p2p


all: MKDIR $(SOURCES) $(EXEC)

MKDIR:
	mkdir -p $(OBJDIR)

$(EXEC): $(OBJECTS) 
	$(CC) $(OBJECTS) -o $@

$(OBJECTS): $(OBJDIR)/%.o: $(SRCDIR)/%.c
	@echo "Compilation de "$<"..."
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm $(EXEC) $(OBJECTS)


.PHONY: all clean
