# vim:set noet ts=4:
#
# ibus-hangul - The Hangul engine for IBus
#
# Copyright (c) 2009 Choe Hwanjin <choe.hwanjin@gmail.com>
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this program; if not, write to the
# Free Software Foundation, Inc., 59 Temple Place, Suite 330,
# Boston, MA  02111-1307  USA

schemas_DATA = \
	org.freedesktop.ibus.engine.hangul.gschema.xml
	$(NULL)

schemasdir = $(datadir)/glib-2.0/schemas

symboltable_DATA = \
	symbol.txt \
	$(NULL)

symboltabledir = $(datadir)/ibus-hangul/data

install-data-hook:
	if test -z "$(DESTDIR)"; then \
	    glib-compile-schemas $(schemasdir); \
	fi

uninstall-hook:
	SCHEMAS_FILES=`ls $(schemasdir)/*.gschema.xml` || true;         \
	if test -z "$$SCHEMAS_FILES" &&                                 \
	    test -f $(schemasdir)/gschemas.compiled; then               \
	    rm $(schemasdir)/gschemas.compiled;                         \
	fi

EXTRA_DIST = \
	$(schemas_DATA) \
	$(symboltable_DATA) \
	$(NULL)
