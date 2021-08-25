# OpenLDAP Debian packages

Files needed to build LDAP Tool Box OpenLDAP Debian packages

## Documentation

See `http://ltb-project.org/wiki/documentation/openldap-deb`

## Download

See `http://ltb-project.org/wiki/download#packages_for_debianubuntu`


## Instructions for building openldap-ltb

Install dependencies:

```
apt install build-essential
apt install autoconf automake autotools-dev debhelper dh-make devscripts fakeroot file gnupg git lintian patch patchutils pbuilder curl
apt install libltdl7 libltdl-dev libsasl2-2 libsasl2-dev zlib1g zlib1g-dev openssl libssl-dev mime-support mawk libcrack2-dev libwrap0-dev libevent-dev libsodium23 libsodium-dev pandoc
```

get the sources:

```
cd /opt
git clone git@github.com:ltb-project/openldap-deb.git
```

get latest slapd release: (replace Y by slapd version)

```
curl -O ftp://ftp.openldap.org/pub/OpenLDAP/openldap-release/openldap-2.5.Y.tgz
tar xvzf openldap-2.5.Y.tgz
cp -r openldap-2.5.Y/* paquet-openldap-debian/openldap-ltb-2.5.Y
```

get the source of the latest slapd-cli release: (replace N by slapd-cli version)
```
cd 3rdparty
curl -O https://codeload.github.com/ltb-project/slapd-cli/tar.gz/refs/tags/vN
tar xvzf vX.Y
```


import some variables:

```
DEBEMAIL="david.coutadeur@gmail.com"
DEBFULLNAME="David Coutadeur"
export DEBEMAIL DEBFULLNAME
```


general parameters for building a package (being in the package directory):

```
debian/rules clean
debian/rules build
debian/rules binary
```

building the source package (with no signing). Take care to create `openldap-ltb_2.5.Y.orig.tar.gz` from original OpenLDAP package:

```
dpkg-buildpackage -us -uc
```



## Instructions for updating the package


On one environment, prepare the move to the new version and write the changes in the changelog file:

```
git mv openldap-ltb-2.5.X openldap-ltb-2.5.Y
cd openldap-ltb-2.5.Y
dch -v 2.5.Y.1
```

Update to new version in variable file:

```
cd debian
sed -i "s/2\.5\.X/2.5.Y/g" openldap-ltb.vars
```



On every environment, get the previous changes, and do the following:

Update distribution:

```
apt update
apt upgrade
```

Get the new archive, and extract it into the directory:

```
curl -O ftp://ftp.openldap.org/pub/OpenLDAP/openldap-release/openldap-2.5.Y.tgz
tar xvzf openldap-2.5.Y.tgz
cp -r openldap-2.5.Y/* openldap-ltb-2.5.Y/
```


Adapt control files for libsodium:

For Debian 9:
* in `debian/control` file, delete any reference to libsodium in Depends and Build-depends lines
* in `debian/openldap-ltb-contrib-overlays.install`, delete the line `usr/local/openldap/libexec/openldap/pw-argon2.*`


Optionally, update the 3dparty directory with new release of other tools (slapd-cli)


Regenerate the package thanks to the usual procedure

Test package

If package is ok, create tag:

```
git tag v2.5.Y
git push --tags origin
```


