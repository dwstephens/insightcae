#!/bin/bash

if [ ! -d TARBALLS ]; then
 echo "please execute from within thirdparty_src directory!"
 exit -1
fi

tar xzf TARBALLS/ParaView-v4.3.1-source.tar.gz &&  cd ParaView-v4.3.1-source && (

 INSTALLDIR=$(cd ../..; pwd)/thirdparty

 mkdir build
 cd build
 cmake ..  \
 -DCMAKE_C_COMPILER="$(which gcc)"\
 -DCMAKE_CXX_COMPILER="$(which g++)"\
 -DCMAKE_INSTALL_PREFIX="$INSTALLDIR"\
 -DPARAVIEW_ENABLE_PYTHON=ON\
 -DVTK_USE_X=OFF\
 -DVTK_OPENGL_HAS_OSMESA=ON\
 -DPARAVIEW_BUILD_QT_GUI=OFF\
 -DBUILD_TESTING=OFF\
 -DPARAVIEW_INSTALL_DEVELOPMENT_FILES=ON\
 -DPARAVIEW_DATA_EXCLUDE_FROM_ALL=ON

 make -j12
 make install
)
