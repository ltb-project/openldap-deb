# OpenLDAP init script

## Presentation

The init script slapd provide start, stop and other commands for OpenLDAP daemon. It requires:
* Logger, to forward messages to syslog
* Awk, for regular expression management
* BerkeleyDB, for recover and archive tools
* OpenLDAP, for save, index, … tools

Configuration of this script can be done in an external file, with the same name as the init script in /etc/default.

A backup feature allows to save all data or configuration in an LDIF file, compressed or not. The restore feature import the last backup in the directory.

## Documentation

See http://ltb-project.org/wiki/documentation/openldap-initscript

## Download

See http://ltb-project.org/wiki/download#init_script
