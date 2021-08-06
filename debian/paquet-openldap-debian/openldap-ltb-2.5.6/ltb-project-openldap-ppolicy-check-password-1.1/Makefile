# contrib/slapd-modules/check_password/Makefile
# Copyright 2007 Michael Steinmann, Calivia. All Rights Reserved.
# Updated by Pierre-Yves Bonnetain, B&A Consultants, 2008
#

CC=gcc

# Where to look for the CrackLib dictionaries
#
CRACKLIB=/usr/share/cracklib/pw_dict

# Path to the configuration file
#
CONFIG=/etc/openldap/check_password.conf

OPT=-g -O2 -Wall -fpic 						\
	-DHAVE_CRACKLIB -DCRACKLIB_DICTPATH="\"$(CRACKLIB)\""	\
	-DCONFIG_FILE="\"$(CONFIG)\""					\
	-DDEBUG

# Where to find the OpenLDAP headers.
#
LDAP_INC=-I/home/pyb/tmp/openldap-2.3.39/include \
	 -I/home/pyb/tmp/openldap-2.3.39/servers/slapd

# Where to find the CrackLib headers.
#
CRACK_INC=

INCS=$(LDAP_INC) $(CRACK_INC)

LDAP_LIB=-lldap_r -llber

# Comment out this line if you do NOT want to use the cracklib.
# You may have to add an -Ldirectory if the libcrak is not in a standard
# location
#
CRACKLIB_LIB=-lcrack

LIBS=$(LDAP_LIB) $(CRACKLIB_LIB)

LIBDIR=/usr/lib/openldap/


all: 	check_password

check_password.o:
	$(CC) $(OPT) -c $(INCS) check_password.c

check_password: clean check_password.o
	$(CC) -shared -o check_password.so check_password.o $(CRACKLIB_LIB)

install: check_password
	cp -f check_password.so ../../../usr/lib/openldap/modules/

clean:
	$(RM) check_password.o check_password.so check_password.lo
	$(RM) -r .libs

