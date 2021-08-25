/* explockout.c - Lock user account until he has waited */
/* an exponential time after failed authentication attempts */
/* $OpenLDAP$ */
/*
 * Copyright 2018 David Coutadeur <david.coutadeur@gmail.com>.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted only as authorized by the OpenLDAP
 * Public License.
 *
 * A copy of this license is available in the file LICENSE in the
 * top-level directory of the distribution or, alternatively, at
 * <http://www.OpenLDAP.org/license.html>.
 */
/* ACKNOWLEDGEMENTS:
 * This work is loosely derived from the explockout overlay.
 */

#include "portable.h"

/*
 * This file implements an overlay that denies authentication to
 * users who have previously failed to authenticate, requiring them
 * to wait for an exponential time.
 *
 */

#ifdef SLAPD_OVER_EXPLOCKOUT

#include <ldap.h>
#include "lutil.h"
#include "slap.h"
#include <ac/errno.h>
#include <ac/time.h>
#include <ac/string.h>
#include <ac/ctype.h>
#include "slap-config.h"


#define ATTR_PWDFAILURETIME                "pwdFailureTime"
#define ATTR_NAME_MAX_LEN                  150

/* Per-instance configuration information */
/*
 * if (base time) ^ ( number of pwdFailureTime ) < max time
 *   waiting time = (base time) ^ ( number of pwdFailureTime )
 * if (base time) ^ ( number of pwdFailureTime ) >= max time
 *   waiting time = max time
 */
typedef struct explockout_info {
	/* basetime to compute waiting time */
	int basetime;
	/* maximum waiting time at any time */
	int maxtime;
} explockout_info;

/* Attribute Description pwdFailureTime */
static AttributeDescription *ad_pwdFailureTime;


/* configuration attribute and objectclass */
static ConfigTable explockoutcfg[] = {
	{ "explockout-basetime", "seconds", 2, 2, 0,
	  ARG_INT|ARG_OFFSET,
	  (void *)offsetof(explockout_info, basetime),
	  "( OLcfgCtAt:190.1 "
	  "NAME 'olcExpLockoutBaseTime' "
	  "DESC 'base time used for computing exponential lockout waiting time'"
	  "SYNTAX OMsInteger SINGLE-VALUE )", NULL, NULL },

	{ "explockout-maxtime", "seconds", 2, 2, 0,
	  ARG_INT|ARG_OFFSET,
	  (void *)offsetof(explockout_info, maxtime),
	  "( OLcfgCtAt:190.2 "
	  "NAME 'olcExpLockoutMaxTime' "
	  "DESC 'maximum time used for computing exponential lockout waiting time'"
	  "SYNTAX OMsInteger SINGLE-VALUE )", NULL, NULL },

	{ NULL, NULL, 0, 0, 0, ARG_IGNORED }
};

static ConfigOCs explockoutocs[] = {
	{ "( OLcfgCtOc:190.1 "
	  "NAME 'olcExpLockoutConfig' "
	  "DESC 'Exponential lockout configuration' "
	  "SUP olcOverlayConfig "
	  "MAY ( olcExpLockoutBaseTime $ olcExpLockoutMaxTime ) )",
	  Cft_Overlay, explockoutcfg, NULL, NULL },
	{ NULL, 0, NULL }
};

static time_t
parse_time( char *atm )
{
	struct lutil_tm tm;
	struct lutil_timet tt;
	time_t ret = (time_t)-1;

	if ( lutil_parsetime( atm, &tm ) == 0) {
		lutil_tm2time( &tm, &tt );
		ret = tt.tt_sec;
	}
	return ret;
}

int power(int base, unsigned int exp) {
    int i, result = 1;
    for (i = 0; i < exp; i++)
        result *= base;
    return result;
}



static int
explockout_bind( Operation *op, SlapReply *rs )
{

	slap_overinst *on = (slap_overinst *) op->o_bd->bd_info;
	Entry *e;
	int rc;
	int nb_pwdFailureTime = 0;
	time_t now, pwdftime = (time_t)-1;
	Attribute *a;
	int i;
	int delay = 0;
	char result[1024];

	rc = overlay_entry_get_ov( op, &op->o_req_ndn, NULL, NULL, 0, &e, on );

	if ( rc != LDAP_SUCCESS ) {
		overlay_entry_release_ov( op, e, 0, on );
		return SLAP_CB_CONTINUE;
	}

	// get configuration parameters
	explockout_info *lbi = (explockout_info *) on->on_bi.bi_private;

	/* get the current time */
	now = slap_get_time();

	Log1(LDAP_DEBUG_ANY, LDAP_LEVEL_DEBUG, "explockout: basetime: %d\n", lbi->basetime );
	Log1(LDAP_DEBUG_ANY, LDAP_LEVEL_DEBUG, "explockout: maxtime: %d\n", lbi->maxtime);

	/* get pwdFailureTime attribute, if it exists */
	if ((a = attr_find( e->e_attrs, ad_pwdFailureTime)) != NULL) {
		nb_pwdFailureTime = a->a_numvals;

		Log1(LDAP_DEBUG_ANY, LDAP_LEVEL_DEBUG, "explockout: nb of pwdFailureTime: %d\n", nb_pwdFailureTime);

		// Compute exponential time
		delay = power(lbi->basetime, nb_pwdFailureTime);
		if( delay > lbi->maxtime )
		{
			delay = lbi->maxtime;
		}
		Log1(LDAP_DEBUG_ANY, LDAP_LEVEL_DEBUG, "explockout: computed waiting time: %d\n", delay);

		// For each pwdFailureTime, verify that
		// exponential time + pwdFailureTime < now
		for(i=0 ; i < nb_pwdFailureTime ; i++)
		{
			Log1( LDAP_DEBUG_ANY, LDAP_LEVEL_DEBUG, "explockout: verifying pwdFailureTime: %s\n",  a->a_nvals[i].bv_val );
						
			pwdftime = parse_time( a->a_nvals[i].bv_val );
			if( now < (pwdftime + delay) )
			{
				Log1( LDAP_DEBUG_ANY, LDAP_LEVEL_ERR, "explockout: error, you should wait for %d seconds before you can authenticate again\n", (int)(pwdftime + delay - now) );

				// send deny
				overlay_entry_release_ov( op, e, 0, on );
				sprintf(result, "password locked! You should wait %d seconds", (int)(pwdftime + delay - now));
				send_ldap_error( op, rs, LDAP_INVALID_CREDENTIALS, result );
				return LDAP_INVALID_CREDENTIALS;

			}
		}
	}

	overlay_entry_release_ov( op, e, 0, on );
	return SLAP_CB_CONTINUE;


}

static int
explockout_db_init(BackendDB *be, ConfigReply *cr)
{
	const char *err_msg;

	// register pwdFailureTime description
	if ( slap_str2ad( ATTR_PWDFAILURETIME, &ad_pwdFailureTime, &err_msg )
	     != LDAP_SUCCESS )
	{
		Log2( LDAP_DEBUG_ANY, LDAP_LEVEL_ERR, "explockout: attribute '%s': %s.\n", ATTR_PWDFAILURETIME, err_msg );
        	return -1;
	}

	slap_overinst *on = (slap_overinst *) be->bd_info;

	/* initialize private structure to store configuration */
	on->on_bi.bi_private = ch_calloc( 1, sizeof(explockout_info) );

	return 0;
}

static int
explockout_db_close(
	BackendDB *be,
	ConfigReply *cr
)
{
	slap_overinst *on = (slap_overinst *) be->bd_info;
	explockout_info *lbi = (explockout_info *) on->on_bi.bi_private;

	/* free private structure to store configuration */
	free( lbi );

	return 0;
}

static slap_overinst explockout;

int explockout_initialize()
{
	int code;

	explockout.on_bi.bi_type = "explockout";
	explockout.on_bi.bi_db_init = explockout_db_init;
	explockout.on_bi.bi_db_close = explockout_db_close;
	explockout.on_bi.bi_op_bind = explockout_bind;

	// register configuration directives
	explockout.on_bi.bi_cf_ocs = explockoutocs;
	code = config_register_schema( explockoutcfg, explockoutocs );
	if ( code ) return code;

	return overlay_register( &explockout );
}

#if SLAPD_OVER_EXPLOCKOUT == SLAPD_MOD_DYNAMIC
int init_module(int argc, char *argv[]) {
	return explockout_initialize();
}
#endif

#endif	/* defined(SLAPD_OVER_EXPLOCKOUT) */
