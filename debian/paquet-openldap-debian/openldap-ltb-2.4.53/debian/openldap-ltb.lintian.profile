# The default profile for "openldap-ltb"
Profile: debian/openldap-ltb
# It has all the checks and settings from the "debian" profile
Extends: debian/main
#Disable-Tags-From-Check: file-in-usr-marked-as-conffile

Tags: file-in-usr-marked-as-conffile
Overridable: yes
