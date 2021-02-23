
ppm.c - OpenLDAP password policy module

2016    David Coutadeur <david.coutadeur@gmail.com>
        Daly Chikhaoui - Janua <dchikhaoui@janua.fr>

version 1.5

ppm.c is an OpenLDAP module for checking password quality when they are modified.
Passwords are checked against the presence or absence of certain character classes.

This module is used as an extension of the OpenLDAP password policy controls,
see slapo-ppolicy(5) section pwdCheckModule.



INSTALLATION
------------

See INSTALL file


USAGE
-----

See INSTALL file


Password checks
---------------

- 4 character classes are defined by default:
upper case, lower case, digits and special characters.

- more character classes can be defined, just write your own.

- passwords must match the amount of quality points.
A point is validated when at least m characters of the corresponding
character class are present in the password.

- passwords must have at least n of the corresponding character class
present, else they are rejected

- the two previous criterias are checked against any specific character class
defined.

- if a password contains any of the forbidden characters, then it is
rejected.

- if a password contains tokens from the RDN, then it is rejected

- if a password is too long, it can be rejected.


Configuration file
------------------

The configuration file (/etc/openldap/ppm.conf by default) contains
parameters for the module. The PPM_CONFIG_FILE environment variable,
if defined, overloads the configuration file path.
If the file is not found, parameters are given their default value.

The syntax of the file is :
parameter value [min] [minForPoint]

with spaces being delimiters. Parameter names ARE case sensitive

The default configuration file is the following:

```
# minQuality parameter
# Format:
# minQuality [NUMBER]
# Description:
# One point is granted for each class for which MIN_FOR_POINT criteria is fulfilled.
# defines the minimum point numbers for the password to be accepted.
minQuality 3

# maxLength parameter
# Format:
# maxLength [NUMBER]
# Description:
# The password must not be more than [NUMBER] long. 0 means no limit is set.
maxLength 0

# checkRDN parameter
# Format:
# checkRDN [0 | 1]
# Description:
# If set to 1, password must not contain a token from the RDN.
# Tokens are separated by the following delimiters : space tabulation _ - , ; £
checkRDN 0

# forbiddenChars parameter
# Format:
# forbiddenChars [CHARACTERS_FORBIDDEN]
# Description:
# Defines the forbidden characters list (no separator).
# If one of them is found in the password, then it is rejected.
forbiddenChars

# maxConsecutivePerClass parameter
# Format:
# maxConsecutivePerClass [NUMBER]
# Description:
# Defines the maximum number of consecutive character allowed for any class
maxConsecutivePerClass 0

# classes parameter
# Format:
# class-[CLASS_NAME] [CHARACTERS_DEFINING_CLASS] [MIN] [MIN_FOR_POINT]
# Description:
# [CHARACTERS_DEFINING_CLASS]: characters defining the class (no separator)
# [MIN]: If at least [MIN] characters of this class is not found in the password, then it is rejected
# [MIN_FOR_POINT]: one point is granted if password contains at least [MIN_FOR_POINT] character numbers of this class
class-upperCase ABCDEFGHIJKLMNOPQRSTUVWXYZ 0 1
class-lowerCase abcdefghijklmnopqrstuvwxyz 0 1
class-digit 0123456789 0 1
class-special <>,?;.:/!§ù%*µ^¨$£²&é~"#'{([-|è`_\ç^à@)]°=}+ 0 1
```

Example
-------

With this policy:
```
minQuality 4
forbiddenChars .?,
maxLength 0
checkRDN 1
class-upperCase ABCDEFGHIJKLMNOPQRSTUVWXYZ 0 5
class-lowerCase abcdefghijklmnopqrstuvwxyz 0 12
class-digit 0123456789 0 1
class-special <>,?;.:/!§ù%*µ^¨$£²&é~"#'{([-|è`_\ç^à@)]°=}+ 0 1
class-myClass :) 1 1``
```

the password

ThereIsNoCowLevel)

is working, because,
- it has 4 character classes validated : upper, lower, special, and myClass
- it has no character among .?,
- it has at least one character among : or )
- there is no size constraint (maxLength of 0)

but it won't work for the user uid=John Cowlevel,ou=people,cn=example,cn=com,
because the token "Cowlevel" from his RDN exists in the password (case insensitive).

Logs
----
If a user password is rejected by ppm, the user will get this type of message:

Typical user message from ldappasswd(5):
  Result: Constraint violation (19)
  Additional info: Password for dn=\"%s\" does not pass required number of strength checks (2 of 3)

A more detailed message is written to the server log.

Server log:

```
Jul 27 20:09:14 machine slapd[20270]: ppm: Opening file /etc/openldap/ppm.conf
Jul 27 20:09:14 machine slapd[20270]: ppm: Param = minQuality, value = 3, min = (null), minForPoint= (null)
Jul 27 20:09:14 machine slapd[20270]: ppm:  Accepted replaced value: 3
Jul 27 20:09:14 machine slapd[20270]: ppm: Param = forbiddenChars, value = , min = (null), minForPoint= (null)
Jul 27 20:09:14 machine slapd[20270]: ppm:  Accepted replaced value:
Jul 27 20:09:14 machine slapd[20270]: ppm: Param = class-upperCase, value = ABCDEFGHIJKLMNOPQRSTUVWXYZ, min = 0, minForPoint= 5
Jul 27 20:09:14 machine slapd[20270]: ppm:  Accepted replaced value: ABCDEFGHIJKLMNOPQRSTUVWXYZ
Jul 27 20:09:14 machine slapd[20270]: ppm: Param = class-lowerCase, value = abcdefghijklmnopqrstuvwxyz, min = 0, minForPoint= 12
Jul 27 20:09:14 machine slapd[20270]: ppm:  Accepted replaced value: abcdefghijklmnopqrstuvwxyz
Jul 27 20:09:14 machine slapd[20270]: ppm: Param = class-digit, value = 0123456789, min = 0, minForPoint= 1
Jul 27 20:09:14 machine slapd[20270]: ppm:  Accepted replaced value: 0123456789
Jul 27 20:09:14 machine slapd[20270]: ppm: Param = class-special, value = <>,?;.:/!Â§Ã¹%*Âµ^Â¨$Â£Â²&Ã©~"#'{([-|Ã¨`_\Ã§^Ã @)]Â°=}+, min = 0, minForPoint= 1
Jul 27 20:09:14 machine slapd[20270]: ppm:  Accepted replaced value: <>,?;.:/!Â§Ã¹%*Âµ^Â¨$Â£Â²&Ã©~"#'{([-|Ã¨`_\Ã§^Ã @)]Â°=}+
Jul 27 20:09:14 machine slapd[20270]: ppm: Param = class-myClass, value = :), min = 1, minForPoint= 1
Jul 27 20:09:14 machine slapd[20270]: ppm:  Accepted new value:
Jul 27 20:09:14 machine slapd[20270]: ppm: 1 point granted for class class-upperCase
Jul 27 20:09:14 machine slapd[20270]: ppm: 1 point granted for class class-lowerCase
Jul 27 20:09:14 machine slapd[20270]: ppm: 1 point granted for class class-digit
```


TODO
----
* integrate configuration file into cn=config


HISTORY
-------
* 2017-02-07 David Coutadeur <david.coutadeur@gmail.com>
  Adds maxConsecutivePerClass (idea from Trevor Vaughan / tvaughan@onyxpoint.com)
  Version 1.5
* 2016-08-22 David Coutadeur <david.coutadeur@gmail.com>
  Get config file from environment variable
  Version 1.4
* 2014-12-20 Daly Chikhaoui <dchikhaoui@janua.fr>
  Adding checkRDN parameter
  Version 1.3
* 2014-10-28 David Coutadeur <david.coutadeur@gmail.com>
  Adding maxLength parameter
  Version 1.2
* 2014-07-27 David Coutadeur <david.coutadeur@gmail.com>
  Changing the configuration file and the configuration data structure
  Version 1.1
* 2014-04-04 David Coutadeur <david.coutadeur@gmail.com>
  Version 1.0

