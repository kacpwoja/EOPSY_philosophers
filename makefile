# Program name
NAME		= philo

# Compiler
CC		= gcc

# Directories
SRCDIR		= source
INCDIR		= headers
BUILDDIR	= build

# Flags etc.
CFLAGS		= -Wall -g
LIBS		=
INC		= -I$(INCDIR)

# ------------------
# Functionality
# ------------------
SRC		= $(shell find $(SRCDIR) -name '*.c')
OBJ		= $(patsubst $(SRCDIR)/%.c,$(BUILDDIR)/%.o,$(SRC))

# Default
compile: makedir $(NAME)

# Link
$(NAME): $(OBJ)
	$(CC) -o $(NAME) $^ $(LIBS)

# Clean objects
clean:
	@rm -f $(BUILDDIR)/*

# Clean the binary too
fullclean: clean
	@rm -f $(NAME)

# Full clean and make again
remake: fullclean compile

# Create Build directory
makedir:
	@mkdir -p $(BUILDDIR)

# Compile
$(BUILDDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -o $@ -c $< $(INC)

# PHONY
.PHONY: compile clean fullclean remake 
