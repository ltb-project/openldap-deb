# contrib/slapd-modules/explockout/Makefile
# Copyright 2018 David Coutadeur, Paris. All Rights Reserved.
#

CC=gcc

# Path of OpenLDAP sources
OLDAP_SOURCES=..
# Final path of explockout installed
LIBDIR=/usr/local/openldap/libexec/openldap
# Where to deploy explockout overlay for now
DSTDIR=/opt/openldap-deb/debian/paquet-openldap-debian/openldap-ltb-2.4.47/debian/tmp/usr/local/openldap/libexec/openldap
LIBTOOL = $(OLDAP_SOURCES)/libtool
DEFS = -DSLAPD_OVER_EXPLOCKOUT=SLAPD_MOD_DYNAMIC

OPT=-g -O2 -Wall -fpic 						\
	-DCONFIG_FILE="\"$(CONFIG)\""				\
	-DDEBUG

# Where to find the OpenLDAP headers.

LDAP_INC=-I$(OLDAP_SOURCES)/include \
	 -I$(OLDAP_SOURCES)/servers/slapd

# Where to find the OpenLDAP libraries.

LDAP_LIBS_DIR=-L$(OLDAP_SOURCES)/libraries/liblber/.libs \
              -L$(OLDAP_SOURCES)/libraries/libldap_r/.libs

LDAP_LIBS_NAME=-lldap_r -llber

PROGRAMS = explockout.la
LTVER = 0:0:0

.SUFFIXES: .c .o .lo

.c.lo:
	$(LIBTOOL) --mode=compile $(CC) $(OPT) $(DEFS) $(LDAP_INC) -c $<

all: $(PROGRAMS)

explockout.la: explockout.lo
	$(LIBTOOL) --mode=link $(CC) $(OPT) -version-info $(LTVER) \
	-rpath $(LIBDIR) -module -o $@ $? $(LDAP_LIBS_NAME) $(LDAP_LIBS_DIR)


install: $(PROGRAMS)
	mkdir -p $(DSTDIR)
	for p in $(PROGRAMS) ; do \
		$(LIBTOOL) --mode=install cp $$p $(DSTDIR) ; \
	done

.PHONY: clean

clean:
	$(RM) -f *.o *.lo *.la *.so
	$(RM) -rf .libs





