# $FreeBSD$

.PATH: ${.CURDIR}/../../dev/urtwm

SYSDIR?=/usr/src/sys
.include "${SYSDIR}/conf/kern.opts.mk"

KMOD    = if_urtwm
SRCS    = if_urtwm.c if_urtwmreg.h if_urtwmvar.h \
	  bus_if.h device_if.h \
	  opt_bus.h opt_usb.h opt_wlan.h usb_if.h usbdevs.h

.include <bsd.kmod.mk>
