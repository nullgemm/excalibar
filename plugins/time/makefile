CC := cc
NAME := time
SRCD := src
INCD := src
BIND := bin
OBJD := obj
CFLAGS := -std=c99 -g -pedantic -Wall -D_XOPEN_SOURCE=700 `pkg-config --cflags pangocairo xcb` -I$(INCD) -I../common

$(NAME): $(BIND)/$(NAME)

$(BIND)/$(NAME): $(OBJD)/$(NAME).o
	@echo "compiling $@"
	@mkdir -p $(BIND)
	@$(CC) $(CFLAGS) -lexcalibar $(OBJD)/$(NAME).o -shared -o $(BIND)/$(NAME)

$(OBJD)/%.o : $(SRCD)/%.c 
	@echo "building object $@"
	@mkdir -p $(@D)
	@$(CC) $(CFLAGS) -fPIC -c $< -o $@

clean:
	@echo "cleaning workspace"
	@rm -rf $(BIND)
	@rm -rf $(OBJD)
