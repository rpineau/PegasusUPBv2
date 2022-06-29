#!/bin/bash

PACKAGE_NAME="PegasusUPBv2_X2.pkg"
BUNDLE_NAME="org.rti-zone.PegasusUPBv2X2"

if [ ! -z "$app_id_signature" ]; then
    codesign -f -s "$app_id_signature" --verbose ../build/Release/libPegasusUPBv2Power.dylib
    codesign -f -s "$app_id_signature" --verbose ../build/Release/libPegasusUPBv2Focuser.dylib
fi

mkdir -p ROOT/tmp/PegasusUPBv2_X2/
cp "../PegasusUPBv2Power.ui" ROOT/tmp/PegasusUPBv2_X2/
cp "../PegasusAstro.png" ROOT/tmp/PegasusUPBv2_X2/
cp "../powercontrollist PegasusUPBv2Power.txt" ROOT/tmp/PegasusUPBv2_X2/
cp "../build/Release/libPegasusUPBv2Power.dylib" ROOT/tmp/PegasusUPBv2_X2/

cp "../PegasusUPBv2Focuser.ui" ROOT/tmp/PegasusUPBv2_X2/
cp "../PegasusAstro.png" ROOT/tmp/PegasusUPBv2_X2/
cp "../focuserlist PegasusUPBv2Focuser.txt" ROOT/tmp/PegasusUPBv2_X2/
cp "../build/Release/libPegasusUPBv2Focuser.dylib" ROOT/tmp/PegasusUPBv2_X2/

if [ ! -z "$installer_signature" ]; then
	# signed package using env variable installer_signature
	pkgbuild --root ROOT --identifier "$BUNDLE_NAME" --sign "$installer_signature" --scripts Scripts --version 1.0 "$PACKAGE_NAME"
	pkgutil --check-signature "./${PACKAGE_NAME}"
	xcrun notarytool submit "$PACKAGE_NAME" --keychain-profile "$AC_PROFILE" --wait
	xcrun stapler staple $PACKAGE_NAME
else
	pkgbuild --root ROOT --identifier $BUNDLE_NAME --scripts Scripts --version 1.0 $PACKAGE_NAME
fi


rm -rf ROOT
