PNAME=smallfry
CC = gcc
CFLAGS=-Wall
LDFLAGS=-s
LIBS=-lm
ARC = ar rcs
LN = @ln -fsv
RM = @rm -fv
RMDIR = @rm -frv
PLIB = lib$(PNAME)
VERSION_MAJOR = 0
VERSION_MINOR = 0
VERSION_RELEASE = 2
PREFIX = /usr/local
INCPREFIX = $(PREFIX)/include
LIBPREFIX = $(PREFIX)/lib
DOCPREFIX = $(PREFIX)/share/doc/$(PLIB)$(VERSION_MAJOR)
INSTALL = install

SRCDIR=./src
SRC= $(SRCDIR)/smallfry.c
LIBOBJ = $(SRC:.c=.o)

all: lib

$(PLIB).a: $(LIBOBJ)
	$(ARC) $@ $(LIBOBJ)

$(PLIB).so: $(LIBOBJ)
	$(CC) -Wl,-soname,$@.$(VERSION_MAJOR) -shared -o $@ $^ $(LDFLAGS) $(LIBS)

lib: $(PLIB).a $(PLIB).so

clean:
	$(RM) $(SRCDIR)/*.o
	$(RM) ./$(PLIB).a
	$(RM) ./$(PLIB).so

install: lib
	$(INSTALL) -d $(LIBPREFIX)
	$(INSTALL) -m 0644 $(PLIB).a $(LIBPREFIX)/$(PLIB).a
	$(INSTALL) -m 0644 $(PLIB).so $(LIBPREFIX)/$(PLIB).so.$(VERSION_MAJOR).$(VERSION_MINOR).$(VERSION_RELEASE)
	$(LN) $(PLIB).so.$(VERSION_MAJOR).$(VERSION_MINOR).$(VERSION_RELEASE) $(LIBPREFIX)/$(PLIB).so.$(VERSION_MAJOR)
	$(LN) $(PLIB).so.$(VERSION_MAJOR) $(LIBPREFIX)/$(PLIB).so
	$(INSTALL) -d $(INCPREFIX)
	$(INSTALL) -m 0644 $(SRCDIR)/$(PNAME).h $(INCPREFIX)
	$(INSTALL) -d $(LIBPREFIX)/pkgconfig
	$(INSTALL) -m 0644 $(PNAME).pc $(LIBPREFIX)/pkgconfig
	$(INSTALL) -d $(DOCPREFIX)
	$(INSTALL) -m 0644 LICENSE README.md $(DOCPREFIX)

uninstall:
	$(RM) $(LIBPREFIX)/$(PLIB).a
	$(RM) $(LIBPREFIX)/$(PLIB).so.$(VERSION_MAJOR).$(VERSION_MINOR).$(VERSION_RELEASE)
	$(RM) $(LIBPREFIX)/$(PLIB).so.$(VERSION_MAJOR)
	$(RM) $(LIBPREFIX)/$(PLIB).so
	$(RM) $(LIBPREFIX)/pkgconfig/$(PNAME).pc
	$(RM) $(INCPREFIX)/$(PNAME).h
	$(RMDIR) $(DOCPREFIX)
