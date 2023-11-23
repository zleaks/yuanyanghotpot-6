#!/bin/bash
#
# Copyright (c) CPU and Fundamental Software Research Center, CAS
#
# This software is provided by the copyright holders - CPU and Fundamental Software Research Center,
# CAS. The above copyright is protected by copyright laws in all countries and the Universal Copyright
# Convention. CPU and Fundamental Software Research Center, CAS will reserve the right to
# take any necessary legal means against the unauthorized use of this software or any use that appears
# outside of CPU and Fundamental Softwre Research Center's intellectual property declaration.
#
# This software is published by the license of CPU-OS License，you can use and distibuted
# this software under this License. See CPU-OS Liense for more detail.
#
# You should have receive a copy of CPU-OS License. If not, please contact us
# by email <support_os@cpu-os.ac.cn>
#


echo '
---------------------------------------------------------------
Funtion: build yuanyang package.
run with
./autodeb.sh /path/to/yuanyang_chroot_Base version

check your param. Enter to continue.

---------------------------------------------------------------

' &>/dev/null

if [[ $EUID -ne 0 ]]; then
  echo "This script must be run as  root, run with:
sudo -s
./autodeb.sh /path/to/32/install/dir /path/to/64/install/dir version"
  exit 1
fi

# init
installeddir32="$1"
version="$2"

if [ -z "$version" ]; then
  exit 1
fi

curpath=`pwd`
yuanyangdebs="$curpath/hotpot6_$version"_amd64

rm -rf $yuanyangdebs
if [ ! -d "$yuanyangdebs" ]; then
  mkdir -p $yuanyangdebs
fi

# deb
cp -r DEBIAN $yuanyangdebs
sed -i "s/Version:.*/Version: $version/g" $yuanyangdebs/DEBIAN/control
echo $yuanyangdebs/DEBIAN
chmod 755 -R $yuanyangdebs/DEBIAN

# wine bin lib share
echo "Start engine..."
cp -rf $installeddir32/* $yuanyangdebs/
echo "End engine..."

#echo "copy 50-rules."
# mkdir -p "$yuanyangdebs/etc/udev/rules.d"
# cp -rf $curpath/uitools/core/etc/udev/rules.d/* $yuanyangdebs/etc/udev/rules.d

# fonts
echo "Start fonts mono gecko..."
# mkdir -p "$yuanyangdebs/usr/local/share/wine"
# cp -rf $curpath/wine/* $yuanyangdebs/usr/local/share/wine/
rm -rf $yuanyangdebs/opt/winux/hotpot6/usr/local/share/wine/fonts
ln -s /opt/winux/fonts $yuanyangdebs/opt/winux/hotpot6/usr/local/share/wine/fonts
ln -s /opt/winux/mono $yuanyangdebs/opt/winux/hotpot6/usr/local/share/wine/mono
ln -s /opt/winux/gecko $yuanyangdebs/opt/winux/hotpot6/usr/local/share/wine/gecko
# chmod 777 -R "$yuanyangdebs/usr/local/share/wine/fonts/"
echo "fonts mono gecko."

# directories && desktop && icons
# echo "Start desktop and icons..."
# mkdir -p "$yuanyangdebs/usr/share"
# cp -rf $curpath/uitools/core/usr/share/* $yuanyangdebs/usr/share
# echo "End desktop and icons."

# resource
# echo "Start res..."
# mkdir -p  $yuanyangdebs/opt/winux/res/deploy/packages/addons
# cp -rf $curpath/uitools/core/opt/winux/res/deploy/packages/addons/* $yuanyangdebs/opt/winux/res/deploy/packages/addons
# mkdir -p  $yuanyangdebs/etc/skel/.cache/wine
# cp -rf $curpath/uitools/core/opt/winux/res/deploy/packages/addons/* $yuanyangdebs/etc/skel/.cache/wine
# echo "End res."

# tools
# echo "Start tools..."
# mkdir -p $yuanyangdebs/opt/winux/tools
# cp -rf $curpath/appmanager $yuanyangdebs/opt/winux/tools/
# echo "End tools."

#server dir 
# echo "Start server ..."
# mkdir -p $yuanyangdebs/opt/winux/server
# echo "End server."

# chown 1000:1000 -R "$yuanyangdebs/opt/winux"
# chmod 777 -R "$yuanyangdebs/opt/winux"

# deb
echo "Start debs..."
dpkg -b $yuanyangdebs .
mv hotpot6_""$version""_amd64.deb /home/"$(logname)"/workspace/deb/
echo "End debs core."

echo -e "\e[1;31m/home/"$(logname)"/workspace/deb/hotpot6_""$version""_amd64.deb\e[0m"
echo "Succeed."
