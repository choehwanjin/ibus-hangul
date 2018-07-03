/* vim:set et sts=4: */
/* ibus-hangul - The Hangul Engine For IBus
 * Copyright (C) 2008-2009 Peng Huang <shawn.p.huang@gmail.com>
 * Copyright (C) 2009-2011 Choe Hwanjin <choe.hwanjin@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <ibus.h>
#include <stdlib.h>
#include <locale.h>
#include <hangul.h>

#include "i18n.h"
#include "engine.h"


static IBusBus *bus = NULL;
static IBusFactory *factory = NULL;

/* options */
static gboolean ibus = FALSE;
static gboolean verbose = FALSE;

static const GOptionEntry entries[] =
{
    { "ibus", 'i', 0, G_OPTION_ARG_NONE, &ibus, "component is executed by ibus", NULL },
    { "verbose", 'v', 0, G_OPTION_ARG_NONE, &verbose, "verbose", NULL },
    { NULL },
};


static void
ibus_disconnected_cb (IBusBus  *bus,
                      gpointer  user_data)
{
    g_debug ("bus disconnected");
    ibus_quit ();
}


static void
start_component (void)
{
    IBusComponent *component;
    IBusConfig* config;
    gboolean res;

    ibus_init ();

    bus = ibus_bus_new ();

    res = ibus_bus_is_connected (bus);
    if (!res) {
        g_warning ("Unable to connect to IBus");
        exit (2);
    }

    config = ibus_bus_get_config (bus);
    if (config == NULL) {
        g_warning ("Unable to connect to IBus config component");
        exit (3);
    }

    g_signal_connect (bus, "disconnected", G_CALLBACK (ibus_disconnected_cb), NULL);

    ibus_hangul_init (bus);

    component = ibus_component_new ("org.freedesktop.IBus.Hangul",
                                    N_("Korean input method"),
                                    "0.1.0",
                                    "GPL",
                                    "Peng Huang <shawn.p.huang@gmail.com>",
                                    "http://code.google.com/p/ibus/",
                                    "",
                                    "ibus-hangul");
    ibus_component_add_engine (component,
                               ibus_engine_desc_new ("hangul",
                                                     N_("Korean Input Method"),
                                                     N_("Korean Input Method"),
                                                     "ko",
                                                     "GPL",
                                                     "Peng Huang <shawn.p.huang@gmail.com>",
                                                     PKGDATADIR"/icon/ibus-hangul.svg",
                                                     "us"));

    factory = ibus_factory_new (ibus_bus_get_connection (bus));

    ibus_factory_add_engine (factory, "hangul", IBUS_TYPE_HANGUL_ENGINE);

    if (ibus) {
        ibus_bus_request_name (bus, "org.freedesktop.IBus.Hangul", 0);
    }
    else {
        ibus_bus_register_component (bus, component);
    }

    g_object_unref (component);

    ibus_main ();

    ibus_hangul_exit ();
}

int
main (gint argc, gchar **argv)
{
    GError *error = NULL;
    GOptionContext *context;

    setlocale (LC_ALL, "");

    bindtextdomain(GETTEXT_PACKAGE, LOCALEDIR);
    bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
    textdomain(GETTEXT_PACKAGE);

    context = g_option_context_new ("- ibus hangul engine component");

    g_option_context_add_main_entries (context, entries, GETTEXT_PACKAGE);

    if (!g_option_context_parse (context, &argc, &argv, &error)) {
        g_print ("Option parsing failed: %s\n", error->message);
        exit (-1);
    }

    start_component ();
    return 0;
}
