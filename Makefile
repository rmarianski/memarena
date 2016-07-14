NAME = memarena
LIB = libmemarena
LIBRARY_VERSION = 0.0.1
HEADERS = memarena.h
OBJECTS = memarena.o
CFLAGS = -g -Wall -std=c11 -pedantic -fPIC -O3
LDLIBS =
DESTDIR = $(HOME)/opt

all: shared static

shared: $(OBJECTS)
	$(CC) -shared -o $(LIB).so.$(LIBRARY_VERSION) $<

static: $(OBJECTS)
	ar rcs $(LIB).a $<

clean:
	rm -f $(OBJECTS) $(LIB).so.$(LIBRARY_VERSION) $(LIB).a check.o check

check: $(OBJECTS) check.o
	$(CC) $(CFLAGS) -o check $(OBJECTS) check.o
	./check

install: all
	mkdir -p $(DESTDIR)/include $(DESTDIR)/lib $(DESTDIR)/lib/pkgconfig
	cp -f $(HEADERS) $(DESTDIR)/include
	cp -f $(LIB).so.$(LIBRARY_VERSION) $(LIB).a $(DESTDIR)/lib
	(cd $(DESTDIR)/lib && ln -sf $(LIB).so.$(LIBRARY_VERSION) $(LIB).so)
	cp -f $(NAME).pc $(DESTDIR)/lib/pkgconfig
	sed -i "s#HOME#$(HOME)#g" $(DESTDIR)/lib/pkgconfig/$(NAME).pc

.PHONY: all shared static clean check install
