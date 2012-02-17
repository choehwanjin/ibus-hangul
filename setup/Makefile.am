# vim:set noet ts=4:
#
# ibus-hangul - The Hangul Engine For IBus
#
# Copyright (c) 2009 Choe Hwanjin <choe.hwanjin@gmail.com>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

setup_hangul_PYTHON = \
	config.py \
	main.py \
	keycapturedialog.py \
	setup.ui \
	$(NULL)

nodist_setup_hangul_PYTHON = config.py

setup_hanguldir = $(pkgdatadir)/setup

bin_SCRIPTS = ibus-setup-hangul

setuplibdir = $(pkglibdir)/setup
setuplib_PROGRAMS = hangul_keyboard_list

hangul_keyboard_list_SOURCES = hangul_keyboard_list.c
hangul_keyboard_list_CFLAGS = $(HANGUL_CFLAGS)
hangul_keyboard_list_LDADD = $(HANGUL_LIBS)

desktop_in_files = ibus-setup-hangul.desktop.in
desktop_DATA = ibus-setup-hangul.desktop
desktopdir = $(datadir)/applications
@INTLTOOL_DESKTOP_RULE@

ibus-setup-hangul.desktop.in: ibus-setup-hangul.desktop.in.in
	( \
		pkgdatadir=${pkgdatadir}; \
		s=`cat $<`; \
		eval "echo \"$${s}\""; \
	) > $@

CLEANFILES = \
	ibus-setup-hangul \
	config.py \
	*.pyc \
	ibus-setup-hangul.desktop \
	$(NULL)

EXTRA_DIST = \
	config.py.in \
	ibus-setup-hangul.in \
	ibus-setup-hangul.desktop.in.in \
	$(NULL)

test:
	$(ENV) DBUS_DEBUG=true \
	       PYTHONPATH=$(abs_top_srcdir):$(pyexecdir) \
	       $(PYTHON) $(srcdir)/main.py

config.py: config.py.in Makefile
	sed -e 's&@SETUP_GETTEXT_PACKAGE@&$(GETTEXT_PACKAGE)&g' \
	    -e 's&@SETUP_LOCALEDIR@&$(localedir)&g' \
	    -e 's&@SETUP_PKGDATADIR@&$(pkgdatadir)&g' \
	    -e 's&@SETUP_PKGLIBDIR@&$(pkglibdir)&g' $< > $@

ibus-setup-hangul: ibus-setup-hangul.in config.py Makefile
	sed -e 's&@SETUP_PKGDATADIR@&$(pkgdatadir)&g' $< > $@
