#!/bin/bash -xe
echo "Creating the installer"

VERSION=$(grep -oP "(?<=Mcenter VERSION )[0-9.]*" ../CMakeLists.txt)

DIST_DIR="Mcenter-$VERSION-bin"
APP_DIR="$DIST_DIR/app"

rm -rf $DIST_DIR
rm -f $DIST_DIR.tar.gz

mkdir $DIST_DIR
mkdir $APP_DIR

cp ../build/mcenter $APP_DIR
cp ../build/helper/mcenter-helper $APP_DIR

strip --strip-unneeded $APP_DIR/mcenter
strip --strip-unneeded $APP_DIR/mcenter-helper

cp ../src/helper/mcenter-helper.conf ../src/helper/mcenter.helper.service $APP_DIR
cp ../resources/mcenter.desktop ../resources/mcenter.svg $APP_DIR

cp ./install.sh ./uninstall.sh $DIST_DIR

tar -czvf $DIST_DIR.tar.gz $DIST_DIR

rm -r $DIST_DIR

echo "Installer created successfully"
