#
# Regular cron jobs for the openldap package
#
0 4	* * *	root	[ -x /usr/bin/openldap_maintenance ] && /usr/bin/openldap_maintenance
