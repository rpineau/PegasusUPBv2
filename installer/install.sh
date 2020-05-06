#!/bin/bash

TheSkyX_Install=~/Library/Application\ Support/Software\ Bisque/TheSkyX\ Professional\ Edition/TheSkyXInstallPath.txt
echo "TheSkyX_Install = $TheSkyX_Install"

if [ ! -f "$TheSkyX_Install" ]; then
    echo TheSkyXInstallPath.txt not found
    TheSkyX_Path=`/usr/bin/find ~/ -maxdepth 3 -name TheSkyX`
    if [ ! -d "$TheSkyX_Path" ]; then
	   echo TheSkyX application was not found.
    	exit 1
	 fi
else
	TheSkyX_Path=$(<"$TheSkyX_Install")
fi

echo "Installing to $TheSkyX_Path"


if [ ! -d "$TheSkyX_Path" ]; then
    echo TheSkyX Install dir not exist
    exit 1
fi

if [ -d "$TheSkyX_Path/Resources/Common/PlugIns64" ]; then
	PLUGINS_DIR="PlugIns64"
elif [ -d "$TheSkyX_Path/Resources/Common/PlugInsARM32" ]; then
	PLUGINS_DIR="PlugInsARM32"
elif [ -d "$TheSkyX_Path/Resources/Common/PlugInsARM64" ]; then
	PLUGINS_DIR="PlugInsARM64"
else
	PLUGINS_DIR="PlugIns"
fi

cp "./focuserlist PegasusUPB.txt" "$TheSkyX_Path/Resources/Common/Miscellaneous Files/"
cp "./PegasusUPB.ui" "$TheSkyX_Path/Resources/Common/$PLUGINS_DIR/FocuserPlugIns/"
cp "./PegasusAstro.png" "$TheSkyX_Path/Resources/Common/$PLUGINS_DIR/FocuserPlugIns/"
cp "./libPegasusUPB.so" "$TheSkyX_Path/Resources/Common/$PLUGINS_DIR/FocuserPlugIns/"

app_owner=`/usr/bin/stat -c "%u" "$TheSkyX_Path" | xargs id -n -u`
if [ ! -z "$app_owner" ]; then
	chown $app_owner "$TheSkyX_Path/Resources/Common/Miscellaneous Files/focuserlist PegasusUPB.txt"
	chown $app_owner "$TheSkyX_Path/Resources/Common/$PLUGINS_DIR/FocuserPlugIns/PegasusUPB.ui"
	chown $app_owner "$TheSkyX_Path/Resources/Common/$PLUGINS_DIR/FocuserPlugIns/PegasusAstro.png"
	chown $app_owner "$TheSkyX_Path/Resources/Common/$PLUGINS_DIR/FocuserPlugIns/libPegasusUPB.so"
fi
chmod  755 "$TheSkyX_Path/Resources/Common/$PLUGINS_DIR/FocuserPlugIns/libPegasusUPB.so"
