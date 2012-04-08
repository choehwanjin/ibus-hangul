# vim:set et sts=4 sw=4:
#
# ibus-hangul - The Hangul Engine For IBus
#
# Copyright (c) 2009-2011 Choe Hwanjin <choe.hwanjin@gmail.com>
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

import sys
import os
from gi.repository import GLib
from gi.repository import Gtk
from gi.repository import IBus
import locale
import gettext
import config
import subprocess
from keycapturedialog import KeyCaptureDialog

_ = lambda a : gettext.dgettext(config.gettext_package, a)

class Setup ():
    def __init__ (self, bus):
        self.__bus = bus
        self.__config = self.__bus.get_config()
	self.__config.connect("value-changed", self.on_value_changed, None)

	ui_file = os.path.join(os.path.dirname(__file__), "setup.ui")
	self.__builder = Gtk.Builder()
	self.__builder.set_translation_domain(config.gettext_package)
	self.__builder.add_from_file(ui_file)

	# Hangul tab
	pipe = subprocess.Popen([config.setuplibdir + '/hangul_keyboard_list'], stdout = subprocess.PIPE)
	list = pipe.communicate()[0].split('\n')
	
	self.__hangul_keyboard = self.__builder.get_object("HangulKeyboard")
	model = Gtk.ListStore(str, str, int)
	i = 0
	for line in list:
	    items = line.split('\t')
	    if len(items) > 1:
		model.append([items[1], items[0], i])
		i+=1

	self.__hangul_keyboard.set_model(model)
	renderer = Gtk.CellRendererText()
	self.__hangul_keyboard.pack_start(renderer, True)
	self.__hangul_keyboard.add_attribute(renderer, "text", 0)

	default = GLib.Variant.new_string("2")
	current = self.__read("HangulKeyboard", default).get_string()
	for i in model:
	    if i[1] == current:
		self.__hangul_keyboard.set_active(i[2])
		break

	self.__word_commit = self.__builder.get_object("WordCommit")

	default = GLib.Variant.new_boolean(False)
	word_commit = self.__read("WordCommit", default).get_boolean()
        self.__word_commit.set_active(word_commit)

	self.__auto_reorder = self.__builder.get_object("AutoReorder")
	default = GLib.Variant.new_boolean(True)
	auto_reorder = self.__read("AutoReorder", default).get_boolean()
        self.__auto_reorder.set_active(auto_reorder)

	# hanja tab
	button = self.__builder.get_object("HanjaKeyListAddButton")
	button.connect("clicked", self.on_hanja_key_add, None)

	button = self.__builder.get_object("HanjaKeyListRemoveButton")
	button.connect("clicked", self.on_hanja_key_remove, None)

	model = Gtk.ListStore(str)

	default = GLib.Variant.new_string("Hangul_Hanja,F9")
	keylist_str = self.__read("HanjaKeys", default).get_string()
	self.__hanja_key_list_str = keylist_str.split(',')
	for i in self.__hanja_key_list_str:
	    model.append([i])

	self.__hanja_key_list = self.__builder.get_object("HanjaKeyList")
	self.__hanja_key_list.set_model(model)
	column = Gtk.TreeViewColumn()
	column.set_title("key")
	renderer = Gtk.CellRendererText()
	column.pack_start(renderer, True)
	column.add_attribute(renderer, "text", 0)
	self.__hanja_key_list.append_column(column)


	# advanced tab
	notebook = self.__builder.get_object("SetupNotebook")
	notebook.remove_page(2)

	# setup dialog
	self.__window = self.__builder.get_object("SetupDialog")
	icon_file = os.path.join(config.pkgdatadir, "icons", "ibus-hangul.svg")
	self.__window.set_icon_from_file(icon_file)
	self.__window.connect("response", self.on_response, None)
	self.__window.show()

	ok_button = self.__builder.get_object("button_cancel")
	ok_button.grab_focus()

    def run(self):
	res = self.__window.run()
	if (res == Gtk.ResponseType.OK):
	    self.on_ok()
	self.__window.destroy()

    def apply(self):
	model = self.__hangul_keyboard.get_model()
	i = self.__hangul_keyboard.get_active()
	self.__write("HangulKeyboard", GLib.Variant.new_string(model[i][1]))

        word_commit = self.__word_commit.get_active()
        self.__write("WordCommit", GLib.Variant.new_boolean(word_commit))

	model = self.__hanja_key_list.get_model()
	str = ""
	iter = model.get_iter_first()
	while iter:
	    if len(str) > 0:
		str += ","
		str += model.get_value(iter, 0)
	    else:
		str += model.get_value(iter, 0)
	    iter = model.iter_next(iter)
	self.__write("HanjaKeys", GLib.Variant.new_string(str))

    def on_response(self, widget, id, data = None):
	if id == Gtk.ResponseType.APPLY:
	    self.apply()
	    widget.emit_stop_by_name("response")
	if id == Gtk.ResponseType.NONE:
	    widget.emit_stop_by_name("response")

    def on_ok(self):
	self.apply()

    def on_hanja_key_add(self, widget, data = None):
	dialog = KeyCaptureDialog(_("Select Hanja key"), self.__window)
	res = dialog.run()
	if res == Gtk.ResponseType.OK:
	    key_str = dialog.get_key_string()
	    if len(key_str) > 0:
		model = self.__hanja_key_list.get_model()
		iter = model.get_iter_first()
		while iter:
		    str = model.get_value(iter, 0)
		    if str == key_str:
			model.remove(iter)
			break
		    iter = model.iter_next(iter)

		model.append([key_str])
	dialog.destroy()

    def on_hanja_key_remove(self, widget, data = None):
	selection = self.__hanja_key_list.get_selection()
	(model, iter) = selection.get_selected()
	if model and iter:
	    model.remove(iter)

    def on_value_changed(self, config, section, name, value, data):
	if section == "engine/Hangul":
	    if name == "HangulKeyboard":
		model = self.__hangul_keyboard.get_model()
		for i in model:
		    if i[1] == value:
			self.__hangul_keyboard.set_active(i[2])
			break
	    elif name == "HanjaKeys":
		self.__hanja_key_list_str = value.split(',')

    def __read(self, name, v):
        value = self.__config.get_value("engine/Hangul", name)
        if value is None:
            return v
        return value

    def __write(self, name, v):
        return self.__config.set_value("engine/Hangul", name, v)

if __name__ == "__main__":
    locale.bindtextdomain(config.gettext_package, config.localedir)
    locale.bind_textdomain_codeset(config.gettext_package, "UTF-8")

    bus = IBus.Bus()
    if bus.is_connected():
        Setup(bus).run()
    else:
        message = _("IBus daemon is not started")
        dialog = Gtk.MessageDialog(type = Gtk.MessageType.ERROR,
                                   buttons = Gtk.ButtonsType.CLOSE,
                                   message_format = message)
        dialog.run()
        sys.exit(1)
