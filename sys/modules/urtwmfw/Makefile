# $FreeBSD$

.PATH:	${.CURDIR}/../../contrib/dev/urtwm

KMOD=	urtwm-rtl8821aufw
FIRMWS=	urtwm-rtl8821aufw.fw:urtwm-rtl8821aufw:111

CLEANFILES+=	urtwm-rtl8821aufw.fw

urtwm-rtl8821aufw.fw: ${.CURDIR}/../../contrib/dev/urtwm/urtwm-rtl8821aufw.fw.uu
	uudecode -p ${.CURDIR}/../../contrib/dev/urtwm/urtwm-rtl8821aufw.fw.uu > ${.TARGET}

.include <bsd.kmod.mk>
