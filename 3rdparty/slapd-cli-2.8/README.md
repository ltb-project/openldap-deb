# slapd-cli

## Presentation

The slapd-cli project provides start, stop and other commands for OpenLDAP daemon. It requires:
* Logger, to forward messages to syslog
* Awk, sed for regular expression management
* OpenLDAP, for save, index,... tools

Configuration of this script can be done in an external file with the same name as the slapd-cli program

The main features are:
* start / stop / status of OpenLDAP daemon
* check configuration
* debug: start OpenLDAP in debug mode (stay attached)
* reindex
* backup / restore data
* backup / restore configuration
* check synchronization status
* import test data / test configuration

## Documentation

See http://ltb-project.org/wiki/documentation/slapd-cli

## Download

See http://ltb-project.org/wiki/download#slapd-cli
