# Copyright 1999-2006 Gentoo Foundation
# Distributed under the terms of the GNU General Public License v2

DESCRIPTION="A desktop sticky note program for the unix geek"
HOMEPAGE="http://xnots.sourceforge.net"
SRC_URI="mirror://sourceforge/xnots/${P}.tar.gz"

LICENSE="GPL-2"
SLOT="0"
KEYWORDS="~x86"
IUSE=""

RDEPEND="
	|| ( (
		x11-libs/libX11
		x11-libs/libXrender )
	virtual/x11 )
	x11-libs/pango
	>=virtual/linux-sources-2.6.14"

DEPEND="${RDEPEND}"

src_compile() {
	make
}

src_install() {
	make DESTDIR=${D} prefix=/usr install

	gzip -9 ${D}/usr/share/doc/${PF}/*
}
