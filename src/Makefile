# $Id$

# builds the libmad python binding for use in julie

PYTHON_PREFIX = $(shell python -c 'import sys; print sys.exec_prefix')
PYTHON_VERSION = $(shell python -c 'import sys; print sys.version[:3]')

INC := -I$(PYTHON_PREFIX)/include/python$(PYTHON_VERSION) -I/usr/local/include
LIB := -L/usr/local/lib

OBJS = madmodule.o pymadfile.o

madmodule.so: $(OBJS)
	$(CC) -shared $(OBJS) -o $@ -lmad

.c.o:
	$(CC) -Werror -Wall -fPIC $(INC) $(LIB) -o $@ -c $<

clean:
	-rm -f *.o *.so *.pyc *~

.PHONY: clean
