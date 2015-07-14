#!/bin/bash

# This script uses the tarball of all debian packages, sign them and create distribution tarballs

VERSION="2.4.41"

tar zxvf paquets-openldap-debian.tar.gz

dpkg-sig -p --sign builder */*.deb

for i in 'lenny-64' 'wheezy-32' 'wheezy-64' 'squeeze-32' 'squeeze-64' 'jessie-64'; do
	cd $i/
        rm -rf *.tar.gz
	tar cvf berkeleydb-ltb-$VERSION-$i.tar berkeleydb*deb
        gzip berkeleydb-ltb-$VERSION-$i.tar
	tar cvf openldap-ltb-$VERSION-$i.tar openldap*deb
        gzip openldap-ltb-$VERSION-$i.tar
        cd ../
done


