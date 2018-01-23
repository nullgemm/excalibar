CC := cc
SRCD := src
INCD := src
BIND := bin
OBJD := obj
LFLAGS := `pkg-config --libs xcb` -lxcb-ewmh -lcairo -lpthread
CFLAGS := -std=c99 -g -pedantic -Wall -DINI_STOP_ON_FIRST_ERROR=1 -D_XOPEN_SOURCE=700 `pkg-config --cflags pangocairo xcb` -I$(INCD) -I../submodules/inih

excalibar: $(BIND)/excalibar

$(BIND)/excalibar: $(OBJD)/excalibar.o $(OBJD)/ini.o
	@echo "compiling $@"
	@mkdir -p $(BIND)
	@$(CC) $(CFLAGS) $(LFLAGS) -ldl -lexcalibar $(OBJD)/excalibar.o $(OBJD)/ini.o -o $(BIND)/excalibar

$(OBJD)/%.o : $(SRCD)/%.c
	@echo "building object $@"
	@mkdir -p $(@D)
	@$(CC) $(CFLAGS) -c $< -o $@

$(OBJD)/ini.o : ../submodules/inih/ini.c
	@echo "building object $@"
	@mkdir -p $(@D)
	@$(CC) $(CFLAGS) -c $< -o $@

clean:
	@echo "cleaning workspace"
	@rm -rf $(BIND)
	@rm -rf $(OBJD)

cpconfig:
	@echo "copying default configuration file"
	@mkdir -p ~/.config/excalibar/
	@cp excalibar.cfg ~/.config/excalibar/

install: $(BIND)/excalibar
	@echo "installing"
	@install -DZ $(BIND)/excalibar -t ${DESTDIR}/usr/bin

uninstall:
	rm -f ${DESTDIR}/usr/bin/excalibar
