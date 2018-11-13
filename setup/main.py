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
import gi
from gi.repository import Gio
from gi.repository import GLib
gi.require_version('Gtk', '3.0')
from gi.repository import Gtk
gi.require_version('IBus', '1.0')
from gi.repository import IBus
import locale
import gettext
import config
from keycapturedialog import KeyCaptureDialog

_ = lambda a : gettext.dgettext(config.gettext_package, a)


def get_hangul_keyboard_list():
    from ctypes import CDLL, c_int, c_char_p
    libhangul = CDLL('libhangul.so.1')
    libhangul.hangul_ic_get_n_keyboards.argtypes = []
    libhangul.hangul_ic_get_n_keyboards.restype = c_int
    libhangul.hangul_ic_get_keyboard_id.argtypes = [c_int]
    libhangul.hangul_ic_get_keyboard_id.restype = c_char_p
    libhangul.hangul_ic_get_keyboard_name.argtypes = [c_int]
    libhangul.hangul_ic_get_keyboard_name.restype = c_char_p

    n = libhangul.hangul_ic_get_n_keyboards()
    list = []
    for i in range(n):
        id = libhangul.hangul_ic_get_keyboard_id(i).decode('UTF-8')
        name = libhangul.hangul_ic_get_keyboard_name(i).decode('UTF-8')
        list.append((id, name))
    return list


class Setup ():
    def __init__ (self, bus):
        self.__bus = bus
        self.__settings = Gio.Settings(schema="org.freedesktop.ibus.engine.hangul")
        self.__settings.connect("changed", self.on_value_changed)

        ui_file = os.path.join(os.path.dirname(__file__), "setup.ui")
        self.__builder = Gtk.Builder()
        self.__builder.set_translation_domain(config.gettext_package)
        self.__builder.add_from_file(ui_file)

        # Hangul tab
        list = get_hangul_keyboard_list()
        self.__hangul_keyboard = self.__builder.get_object("HangulKeyboard")
        model = Gtk.ListStore(str, str, int)
        i = 0
        for (id, name) in list:
            model.append([name, id, i])
            i+=1

        self.__hangul_keyboard.set_model(model)
        renderer = Gtk.CellRendererText()
        self.__hangul_keyboard.pack_start(renderer, True)
        self.__hangul_keyboard.add_attribute(renderer, "text", 0)

        current = self.__read("hangul-keyboard").get_string()
        for i in model:
            if i[1] == current:
                self.__hangul_keyboard.set_active(i[2])
                break

        self.__start_in_hangul_mode = self.__builder.get_object("StartInHangulMode")
        initial_input_mode = self.__read("initial-input-mode").get_string()
        self.__start_in_hangul_mode.set_active(initial_input_mode == "hangul")

        self.__word_commit = self.__builder.get_object("WordCommit")
        word_commit = self.__read("word-commit").get_boolean()
        self.__word_commit.set_active(word_commit)

        self.__auto_reorder = self.__builder.get_object("AutoReorder")
        auto_reorder = self.__read("auto-reorder").get_boolean()
        self.__auto_reorder.set_active(auto_reorder)

        self.__esc_off_key = self.__builder.get_object("EscOffKey")
        esc_off_key = self.__read("off-keys").get_string()
        self.__esc_off_key.set_active(esc_off_key == "Escape")

        button = self.__builder.get_object("HangulKeyListAddButton")
        button.connect("clicked", self.on_hangul_key_add, None)

        button = self.__builder.get_object("HangulKeyListRemoveButton")
        button.connect("clicked", self.on_hangul_key_remove, None)

        model = Gtk.ListStore(str)

        keylist_str = self.__read("switch-keys").get_string()
        self.__hangul_key_list_str = keylist_str.split(',')
        for i in self.__hangul_key_list_str:
            model.append([i])

        self.__hangul_key_list = self.__builder.get_object("HangulKeyList")
        self.__hangul_key_list.set_model(model)
        column = Gtk.TreeViewColumn()
        column.set_title("key")
        renderer = Gtk.CellRendererText()
        column.pack_start(renderer, True)
        column.add_attribute(renderer, "text", 0)
        self.__hangul_key_list.append_column(column)

        # hanja tab
        button = self.__builder.get_object("HanjaKeyListAddButton")
        button.connect("clicked", self.on_hanja_key_add, None)

        button = self.__builder.get_object("HanjaKeyListRemoveButton")
        button.connect("clicked", self.on_hanja_key_remove, None)

        model = Gtk.ListStore(str)

        keylist_str = self.__read("hanja-keys").get_string()
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
        self.__window.connect("destroy", Gtk.main_quit)
        self.__window.show()

        button = self.__builder.get_object("button_apply")
        button.connect("clicked", self.on_apply, None)

        button = self.__builder.get_object("button_cancel")
        button.connect("clicked", self.on_cancel, None)
        #button.grap_focus()

        button = self.__builder.get_object("button_ok")
        button.connect("clicked", self.on_ok, None)

    def run(self):
        Gtk.main()

    def apply(self):
        model = self.__hangul_keyboard.get_model()
        i = self.__hangul_keyboard.get_active()
        self.__write("hangul-keyboard", GLib.Variant.new_string(model[i][1]))

        start_in_hangul_mode = self.__start_in_hangul_mode.get_active()
        if start_in_hangul_mode:
            self.__write("initial-input-mode", GLib.Variant.new_string("hangul"))
        else:
            self.__write("initial-input-mode", GLib.Variant.new_string("latin"))

        word_commit = self.__word_commit.get_active()
        self.__write("word-commit", GLib.Variant.new_boolean(word_commit))

        auto_reorder = self.__auto_reorder.get_active()
        self.__write("auto-reorder", GLib.Variant.new_boolean(auto_reorder))

        esc_off_key = self.__esc_off_key.get_active()
        if esc_off_key:
            self.__write("off-keys", GLib.Variant.new_string("Escape"))
        else:
            self.__write("off-keys", GLib.Variant.new_string(""))

        model = self.__hangul_key_list.get_model()
        str = ""
        iter = model.get_iter_first()
        while iter:
            if len(str) > 0:
                str += ","
                str += model.get_value(iter, 0)
            else:
                str += model.get_value(iter, 0)
            iter = model.iter_next(iter)
        self.__write("switch-keys", GLib.Variant.new_string(str))

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
        self.__write("hanja-keys", GLib.Variant.new_string(str))

    def on_apply(self, widget, data):
        self.apply()

    def on_cancel(self, widget, data):
        self.__window.destroy()
        Gtk.main_quit()

    def on_ok(self, widget, data):
        self.apply()
        self.__window.destroy()
        Gtk.main_quit()

    def on_hangul_key_add(self, widget, data = None):
        dialog = KeyCaptureDialog(_("Select Hangul toggle key"), self.__window)
        dialog.set_markup(_("Press any key which you want to use as hangul toggle key. "
                "The key you pressed is displayed below.\n"
                "If you want to use it, click \"Ok\" or click \"Cancel\""))
        res = dialog.run()
        if res == Gtk.ResponseType.OK:
            key_str = dialog.get_key_string()
            if len(key_str) > 0:
                model = self.__hangul_key_list.get_model()
                iter = model.get_iter_first()
                while iter:
                    str = model.get_value(iter, 0)
                    if str == key_str:
                        model.remove(iter)
                        break
                    iter = model.iter_next(iter)

                model.append([key_str])
        dialog.destroy()

    def on_hangul_key_remove(self, widget, data = None):
        selection = self.__hangul_key_list.get_selection()
        (model, iter) = selection.get_selected()
        if model and iter:
            model.remove(iter)

    def on_hanja_key_add(self, widget, data = None):
        dialog = KeyCaptureDialog(_("Select Hanja key"), self.__window)
        dialog.set_markup(_("Press any key which you want to use as hanja key. "
                "The key you pressed is displayed below.\n"
                "If you want to use it, click \"Ok\" or click \"Cancel\""))
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

    def on_value_changed(self, settings, key):
        value = settings.get_value(key)
        if key == "hangul-keyboard":
            model = self.__hangul_keyboard.get_model()
            for i in model:
                if i[1] == value.get_string():
                    self.__hangul_keyboard.set_active(i[2])
                    break
        elif key == "switch-keys":
            self.__hangul_key_list_str = value.get_string().split(',')
        elif key == "hanja-keys":
            self.__hanja_key_list_str = value.get_string().split(',')

    def __read(self, key):
        return self.__settings.get_value(key)

    def __write(self, key, v):
        self.__settings.set_value(key, v)

if __name__ == "__main__":
    locale.bindtextdomain(config.gettext_package, config.localedir)
    gettext.bind_textdomain_codeset(config.gettext_package, "UTF-8")

    GLib.set_prgname("ibus-setup-hangul")
    GLib.set_application_name(_("IBusHangul Setup"))

    bus = IBus.Bus()
    if bus.is_connected():
        Setup(bus).run()
    else:
        message = _("IBus daemon is not running.\nHangul engine settings cannot be saved.")
        dialog = Gtk.MessageDialog(type = Gtk.MessageType.ERROR,
                                   buttons = Gtk.ButtonsType.CLOSE,
                                   message_format = message)
        dialog.run()
        sys.exit(1)
