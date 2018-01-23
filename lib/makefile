CC := cc
SRCD := src
INCD := src
BIND := bin
OBJD := obj
LFLAGS := `pkg-config --libs pangocairo xcb` -lxcb-ewmh -lcairo -lpthread
CFLAGS := -std=c11 -g -pedantic -Wall -D_XOPEN_SOURCE=700 `pkg-config --cflags pangocairo xcb` -I$(INCD)

libexcalibar : $(BIND)/libexcalibar.so

$(BIND)/libexcalibar.so: $(OBJD)/tag.o $(OBJD)/font.o $(OBJD)/win.o $(OBJD)/draw.o
	@echo "compiling $@"
	@mkdir -p $(BIND)
	@$(CC) $(CFLAGS) $(LFLAGS) $(OBJD)/tag.o $(OBJD)/font.o $(OBJD)/win.o $(OBJD)/draw.o -shared -o $(BIND)/libexcalibar.so

$(OBJD)/%.o : $(SRCD)/%.c $(INCD)/%.h
	@echo "building object $@"
	@mkdir -p $(@D)
	@$(CC) $(CFLAGS) -fPIC -c $< -o $@

clean:
	@echo "cleaning workspace"
	@rm -rf $(BIND)
	@rm -rf $(OBJD)

install: $(BIND)/libexcalibar.so
	@echo "installing library"
	@install -dZ ${DESTDIR}/usr/include/excalibar
	@install -DZ $(SRCD)/tag.h -t ${DESTDIR}/usr/include/excalibar
	@install -DZ $(SRCD)/excalibar.h -t ${DESTDIR}/usr/include/excalibar
	@install -DZ $(SRCD)/font.h -t ${DESTDIR}/usr/include/excalibar
	@install -DZ $(SRCD)/draw.h -t ${DESTDIR}/usr/include/excalibar
	@install -DZ $(SRCD)/win.h -t ${DESTDIR}/usr/include/excalibar
	@install -DZ $(BIND)/libexcalibar.so -t ${DESTDIR}/usr/lib
	@install -DZ excalibar.pc -t ${DESTDIR}/usr/lib/pkgconfig

uninstall:
	rm -f ${DESTDIR}/usr/lib/libexcalibar.so
	rm -f ${DESTDIR}/usr/pkgconfig/excalibar.pc
	rm -rf ${DESTDIR}/usr/include/excalibar
