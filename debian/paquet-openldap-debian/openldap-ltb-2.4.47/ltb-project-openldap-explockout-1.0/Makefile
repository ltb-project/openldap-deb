# contrib/slapd-modules/explockout/Makefile
# Copyright 2018 David Coutadeur, Paris. All Rights Reserved.
#

CC=gcc

# Path of OpenLDAP sources
OLDAP_SOURCES=../../..
# Where the explockout configuration file should be installed
CONFIG=/opt/openldap-2.4.46-el/etc/openldap/explockout.conf
# Path of OpenLDAP installed libs, where the explockout library should be installed
LIBDIR=/opt/openldap-2.4.46-el/lib64
LIBTOOL = $(OLDAP_SOURCES)/libtool
DEFS = -DSLAPD_OVER_EXPLOCKOUT=SLAPD_MOD_DYNAMIC

OPT=-g -O2 -Wall -fpic 						\
	-DCONFIG_FILE="\"$(CONFIG)\""				\
	-DDEBUG

# Where to find the OpenLDAP headers.

LDAP_INC=-I$(OLDAP_SOURCES)/include \
	 -I$(OLDAP_SOURCES)/servers/slapd

# Where to find the OpenLDAP libraries.

LDAP_LIBS=-L$(OLDAP_SOURCES)/libraries/liblber/.libs \
	  -L$(OLDAP_SOURCES)/libraries/libldap_r/.libs

INCS=$(LDAP_INC)

LDAP_LIB=-lldap_r -llber

LIBS=$(LDAP_LIB)

PROGRAMS = explockout.la
LTVER = 0:0:0

.SUFFIXES: .c .o .lo

.c.lo:
	$(LIBTOOL) --mode=compile $(CC) $(OPT) $(DEFS) $(INCS) -c $<

all: $(PROGRAMS)

explockout.la: explockout.lo
	$(LIBTOOL) --mode=link $(CC) $(OPT) -version-info $(LTVER) \
	-rpath $(LIBDIR) -module -o $@ $? $(LIBS) $(LDAP_LIBS)


install: $(PROGRAMS)
	mkdir -p $(LIBDIR)
	for p in $(PROGRAMS) ; do \
		$(LIBTOOL) --mode=install cp $$p $(LIBDIR) ; \
	done

.PHONY: clean

clean:
	$(RM) -f *.o *.lo *.la *.so
	$(RM) -rf .libs





