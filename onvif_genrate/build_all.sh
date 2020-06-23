#! /bin/sh
ROOT_PATH=`pwd`
INSTALL_PATH=$ROOT_PATH/install
GSOAP_PATH=$ROOT_PATH/gsoap

#tar -zxvf gsoap.tar.gz

rm $INSTALL_PATH -rf

mkdir $INSTALL_PATH
cd $INSTALL_PATH

$GSOAP_PATH/wsdl2h -o onvif.h -c -s -t ../typemap.dat  https://www.onvif.org/ver10/network/wsdl/remotediscovery.wsdl https://www.onvif.org/onvif/ver20/ptz/wsdl/ptz.wsdl https://www.onvif.org/onvif/ver10/media/wsdl/media.wsdl \
       https://www.onvif.org/onvif/ver10/device/wsdl/devicemgmt.wsdl https://www.onvif.org/onvif/ver10/event/wsdl/event.wsdl \
       https://www.onvif.org/ver20/media/wsdl/media.wsdl
sed -i '/#import "wsa5.h"/i\#import "wsse.h"' onvif.h
$GSOAP_PATH/soapcpp2 ./onvif.h -x -c -L -I ../gsoap/import -I ../gsoap

mv MediaBinding.nsmap namespace.h
rm *.nsmap

cp -f namespace.h  onvif.h  soapC.c  soapClient.c  soapH.h  soapStub.h ../libonvif

cd ../libonvif
bash build.sh
