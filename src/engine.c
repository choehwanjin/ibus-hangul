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
#include <hangul.h>
#include <string.h>
#include <ctype.h>

#include "i18n.h"
#include "engine.h"
#include "ustring.h"


typedef struct _IBusHangulEngine IBusHangulEngine;
typedef struct _IBusHangulEngineClass IBusHangulEngineClass;

typedef struct _HotkeyList HotkeyList;

enum {
    INPUT_MODE_DIRECT,
    INPUT_MODE_HANGUL,
    INPUT_MODE_COUNT,
};

struct _IBusHangulEngine {
    IBusEngine parent;

    /* members */
    HangulInputContext *context;
    UString* preedit;
    int input_mode;
    gboolean hanja_mode;
    HanjaList* hanja_list;
    int last_lookup_method;

    IBusLookupTable *table;

    IBusProperty    *prop_hangul_mode;
    IBusProperty    *prop_hanja_mode;
    IBusPropList    *prop_list;

    IBusText        *input_mode_symbols[INPUT_MODE_COUNT];
};

struct _IBusHangulEngineClass {
    IBusEngineClass parent;
};

struct KeyEvent {
    guint keyval;
    guint modifiers;
};

struct _HotkeyList {
    guint   all_modifiers;
    GArray *keys;
};

enum {
    LOOKUP_METHOD_EXACT,
    LOOKUP_METHOD_PREFIX,
    LOOKUP_METHOD_SUFFIX,
};

/* functions prototype */
static void     ibus_hangul_engine_class_init
                                            (IBusHangulEngineClass  *klass);
static void     ibus_hangul_engine_init     (IBusHangulEngine       *hangul);
static GObject*
                ibus_hangul_engine_constructor
                                            (GType                   type,
                                             guint                   n_construct_params,
                                             GObjectConstructParam  *construct_params);
static void     ibus_hangul_engine_destroy  (IBusHangulEngine       *hangul);
static gboolean
                ibus_hangul_engine_process_key_event
                                            (IBusEngine             *engine,
                                             guint                   keyval,
                                             guint                   keycode,
                                             guint                   modifiers);
static void ibus_hangul_engine_focus_in     (IBusEngine             *engine);
static void ibus_hangul_engine_focus_out    (IBusEngine             *engine);
static void ibus_hangul_engine_reset        (IBusEngine             *engine);
static void ibus_hangul_engine_enable       (IBusEngine             *engine);
static void ibus_hangul_engine_disable      (IBusEngine             *engine);
#if 0
static void ibus_engine_set_cursor_location (IBusEngine             *engine,
                                             gint                    x,
                                             gint                    y,
                                             gint                    w,
                                             gint                    h);
static void ibus_hangul_engine_set_capabilities
                                            (IBusEngine             *engine,
                                             guint                   caps);
#endif
static void ibus_hangul_engine_page_up      (IBusEngine             *engine);
static void ibus_hangul_engine_page_down    (IBusEngine             *engine);
static void ibus_hangul_engine_cursor_up    (IBusEngine             *engine);
static void ibus_hangul_engine_cursor_down  (IBusEngine             *engine);
static void ibus_hangul_engine_property_activate
                                            (IBusEngine             *engine,
                                             const gchar            *prop_name,
                                             guint                   prop_state);
#if 0
static void ibus_hangul_engine_property_show
                                                                                        (IBusEngine             *engine,
                                             const gchar            *prop_name);
static void ibus_hangul_engine_property_hide
                                                                                        (IBusEngine             *engine,
                                             const gchar            *prop_name);
#endif

static void ibus_hangul_engine_candidate_clicked
                                            (IBusEngine             *engine,
                                             guint                   index,
                                             guint                   button,
                                             guint                   state);

static void ibus_hangul_engine_flush        (IBusHangulEngine       *hangul);
static void ibus_hangul_engine_clear_preedit_text
                                            (IBusHangulEngine       *hangul);
static void ibus_hangul_engine_update_preedit_text
                                            (IBusHangulEngine       *hangul);

static void ibus_hangul_engine_update_lookup_table
                                            (IBusHangulEngine       *hangul);
static gboolean ibus_hangul_engine_has_preedit
                                            (IBusHangulEngine       *hangul);
static void ibus_hangul_engine_toggle_input_mode
                                            (IBusHangulEngine       *hangul);
static void ibus_hangul_engine_set_input_mode
                                            (IBusHangulEngine       *hangul,
                                             int                     input_mode);
static IBusText*
            ibus_hangul_engine_get_input_mode_symbol
                                            (IBusHangulEngine       *hangul,
                                             int                     input_mode);

static bool ibus_hangul_engine_on_transition
                                            (HangulInputContext     *hic,
                                             ucschar                 c,
                                             const ucschar          *preedit,
                                             void                   *data);

static void ibus_config_value_changed       (IBusConfig             *config,
                                             const gchar            *section,
                                             const gchar            *name,
                                             GVariant               *value,
                                             gpointer                user_data);

static void        lookup_table_set_visible (IBusLookupTable        *table,
                                             gboolean                flag);
static gboolean        lookup_table_is_visible
                                            (IBusLookupTable        *table);

static gboolean key_event_list_match        (GArray                 *list,
                                             guint                   keyval,
                                             guint                   modifiers);

static void     hotkey_list_init            (HotkeyList           *list);
static void     hotkey_list_fini            (HotkeyList           *list);
static void     hotkey_list_set_from_string (HotkeyList         *list,
                                             const char             *str);
static void     hotkey_list_append          (HotkeyList           *list,
                                             guint                   keyval,
                                             guint                   modifiers);
static gboolean hotkey_list_match           (HotkeyList           *list,
                                             guint                   keyval,
                                             guint                   modifiers);
static gboolean hotkey_list_has_modifier    (HotkeyList           *list,
                                             guint                   keyval);

static glong ucschar_strlen (const ucschar* str);

static IBusEngineClass *parent_class = NULL;
static HanjaTable *hanja_table = NULL;
static HanjaTable *symbol_table = NULL;
static IBusConfig *config = NULL;
static GString    *hangul_keyboard = NULL;
static HotkeyList hanja_keys;
static HotkeyList switch_keys;
static HotkeyList on_keys;
static HotkeyList off_keys;
static int lookup_table_orientation = 0;
static IBusKeymap *keymap = NULL;
static gboolean word_commit = FALSE;
static gboolean auto_reorder = TRUE;
static int initial_input_mode = INPUT_MODE_DIRECT;

static IBusProperty *_prop = NULL;
static gboolean _does_ibus_commit_preedit_text_before_reset = FALSE;

static glong
ucschar_strlen (const ucschar* str)
{
    const ucschar* p = str;
    while (*p != 0)
        p++;
    return p - str;
}

GType
ibus_hangul_engine_get_type (void)
{
    static GType type = 0;

    static const GTypeInfo type_info = {
        sizeof (IBusHangulEngineClass),
        (GBaseInitFunc)     NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc)    ibus_hangul_engine_class_init,
        NULL,
        NULL,
        sizeof (IBusHangulEngine),
        0,
        (GInstanceInitFunc) ibus_hangul_engine_init,
    };

    if (type == 0) {
            type = g_type_register_static (IBUS_TYPE_ENGINE,
                                           "IBusHangulEngine",
                                           &type_info,
                                           (GTypeFlags) 0);
    }

    return type;
}

void
ibus_hangul_init (IBusBus *bus)
{
    GVariant* value;

    hanja_table = hanja_table_load (NULL);

    symbol_table = hanja_table_load (IBUSHANGUL_DATADIR "/data/symbol.txt");

    config = ibus_bus_get_config (bus);
    if (config)
        g_object_ref_sink (config);

    hangul_keyboard = g_string_new_len ("2", 8);
    value = ibus_config_get_value (config, "engine/hangul",
                                         "hangul-keyboard");
    if (value != NULL) {
        const gchar* str = g_variant_get_string (value, NULL);
        g_string_assign (hangul_keyboard, str);
        g_variant_unref(value);
    }

    hotkey_list_init(&switch_keys);

    value = ibus_config_get_value (config, "engine/hangul",
                                         "switch-keys");
    if (value != NULL) {
        const gchar* str = g_variant_get_string (value, NULL);
        hotkey_list_set_from_string(&switch_keys, str);
        g_variant_unref(value);
    } else {
	hotkey_list_append(&switch_keys, IBUS_Hangul, 0);
	hotkey_list_append(&switch_keys, IBUS_space, IBUS_SHIFT_MASK);
    }

    hotkey_list_init(&hanja_keys);

    value = ibus_config_get_value (config, "engine/hangul",
                                         "hanja-keys");
    if (value != NULL) {
        const gchar* str = g_variant_get_string (value, NULL);
        hotkey_list_set_from_string(&hanja_keys, str);
        g_variant_unref(value);
    } else {
	hotkey_list_append(&hanja_keys, IBUS_Hangul_Hanja, 0);
	hotkey_list_append(&hanja_keys, IBUS_F9, 0);
    }

    hotkey_list_init (&on_keys);
    value = ibus_config_get_value (config, "engine/hangul", "on-keys");
    if (value != NULL) {
        const gchar* str = g_variant_get_string (value, NULL);
        hotkey_list_set_from_string (&on_keys, str);
        g_variant_unref (value);
    } else {
    }

    hotkey_list_init (&off_keys);
    value = ibus_config_get_value (config, "engine/hangul", "off-keys");
    if (value != NULL) {
        const gchar* str = g_variant_get_string (value, NULL);
        hotkey_list_set_from_string (&off_keys, str);
        g_variant_unref (value);
    } else {
        hotkey_list_append (&off_keys, IBUS_KEY_Escape, 0);
    }

    value = ibus_config_get_value (config, "engine/hangul",
                                         "word-commit");
    if (value != NULL) {
        word_commit = g_variant_get_boolean (value);
        g_variant_unref(value);
    }

    value = ibus_config_get_value (config, "engine/hangul", "auto-reorder");
    if (value != NULL) {
        auto_reorder = g_variant_get_boolean (value);
        g_variant_unref (value);
    }

    value = ibus_config_get_value (config, "engine/hangul", "initial-input-mode");
    if (value != NULL) {
        const gchar* str = g_variant_get_string (value, NULL);
        if (strcmp(str, "latin") == 0) {
            initial_input_mode = INPUT_MODE_DIRECT;
        } else if (strcmp(str, "hangul") == 0) {
            initial_input_mode = INPUT_MODE_HANGUL;
        }
        g_variant_unref (value);
    }

    value = ibus_config_get_value (config, "panel", "lookup-table-orientation");
    if (value != NULL) {
        lookup_table_orientation = g_variant_get_int32(value);
        g_variant_unref (value);
    }

    value = ibus_bus_get_ibus_property (bus, "CommitPreeditTextBeforeResettingIM");
    if (value != NULL) {
        _does_ibus_commit_preedit_text_before_reset = g_variant_get_boolean (value);
        g_variant_unref (value);
    }

    keymap = ibus_keymap_get("us");
}

void
ibus_hangul_exit (void)
{
    if (keymap != NULL) {
	g_object_unref(keymap);
	keymap = NULL;
    }

    hotkey_list_fini (&switch_keys);
    hotkey_list_fini (&hanja_keys);
    hotkey_list_fini (&on_keys);
    hotkey_list_fini (&off_keys);

    hanja_table_delete (hanja_table);
    hanja_table = NULL;

    hanja_table_delete (symbol_table);
    symbol_table = NULL;

    g_object_unref (config);
    config = NULL;

    g_string_free (hangul_keyboard, TRUE);
    hangul_keyboard = NULL;
}

static void
ibus_hangul_engine_class_init (IBusHangulEngineClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    IBusObjectClass *ibus_object_class = IBUS_OBJECT_CLASS (klass);
    IBusEngineClass *engine_class = IBUS_ENGINE_CLASS (klass);

    parent_class = (IBusEngineClass *) g_type_class_peek_parent (klass);

    object_class->constructor = ibus_hangul_engine_constructor;
    ibus_object_class->destroy = (IBusObjectDestroyFunc) ibus_hangul_engine_destroy;

    engine_class->process_key_event = ibus_hangul_engine_process_key_event;

    engine_class->reset = ibus_hangul_engine_reset;
    engine_class->enable = ibus_hangul_engine_enable;
    engine_class->disable = ibus_hangul_engine_disable;

    engine_class->focus_in = ibus_hangul_engine_focus_in;
    engine_class->focus_out = ibus_hangul_engine_focus_out;

    engine_class->page_up = ibus_hangul_engine_page_up;
    engine_class->page_down = ibus_hangul_engine_page_down;

    engine_class->cursor_up = ibus_hangul_engine_cursor_up;
    engine_class->cursor_down = ibus_hangul_engine_cursor_down;

    engine_class->property_activate = ibus_hangul_engine_property_activate;

    engine_class->candidate_clicked = ibus_hangul_engine_candidate_clicked;
}

static void
ibus_hangul_engine_init (IBusHangulEngine *hangul)
{
    IBusProperty* prop;
    IBusText* label;
    IBusText* tooltip;
    IBusText* symbol;

    hangul->context = hangul_ic_new (hangul_keyboard->str);
    hangul_ic_connect_callback (hangul->context, "transition",
                                ibus_hangul_engine_on_transition, hangul);

    hangul->preedit = ustring_new();
    hangul->hanja_list = NULL;
    hangul->input_mode = initial_input_mode;
    hangul->hanja_mode = FALSE;
    hangul->last_lookup_method = LOOKUP_METHOD_PREFIX;

    hangul->prop_list = ibus_prop_list_new ();
    g_object_ref_sink (hangul->prop_list);

    label = ibus_text_new_from_string (_("Hangul mode"));
    tooltip = ibus_text_new_from_string (_("Enable/Disable Hangul mode"));
    prop = ibus_property_new ("InputMode",
                              PROP_TYPE_TOGGLE,
                              label,
                              NULL,
                              tooltip,
                              TRUE, TRUE, PROP_STATE_UNCHECKED, NULL);
    symbol = ibus_hangul_engine_get_input_mode_symbol (hangul,
                                                       initial_input_mode);
    ibus_property_set_symbol(prop, symbol);
    g_object_ref_sink (prop);
    ibus_prop_list_append (hangul->prop_list, prop);
    hangul->prop_hangul_mode = prop;

    label = ibus_text_new_from_string (_("Hanja lock"));
    tooltip = ibus_text_new_from_string (_("Enable/Disable Hanja mode"));
    prop = ibus_property_new ("hanja_mode",
                              PROP_TYPE_TOGGLE,
                              label,
                              NULL,
                              tooltip,
                              TRUE, TRUE, PROP_STATE_UNCHECKED, NULL);
    g_object_ref_sink (prop);
    ibus_prop_list_append (hangul->prop_list, prop);
    hangul->prop_hanja_mode = prop;

    label = ibus_text_new_from_string (_("Setup"));
    tooltip = ibus_text_new_from_string (_("Configure hangul engine"));
    prop = ibus_property_new ("setup",
                              PROP_TYPE_NORMAL,
                              label,
                              "gtk-preferences",
                              tooltip,
                              TRUE, TRUE, PROP_STATE_UNCHECKED, NULL);
    ibus_prop_list_append (hangul->prop_list, prop);

    hangul->table = ibus_lookup_table_new (9, 0, TRUE, FALSE);
    g_object_ref_sink (hangul->table);

    g_signal_connect (config, "value-changed",
                      G_CALLBACK(ibus_config_value_changed), hangul);
}

static GObject*
ibus_hangul_engine_constructor (GType                   type,
                                guint                   n_construct_params,
                                GObjectConstructParam  *construct_params)
{
    IBusHangulEngine *hangul;

    hangul = (IBusHangulEngine *) G_OBJECT_CLASS (parent_class)->constructor (type,
                                                       n_construct_params,
                                                       construct_params);

    return (GObject *)hangul;
}


static void
ibus_hangul_engine_destroy (IBusHangulEngine *hangul)
{
    int i;
    IBusText **symbols;

    if (hangul->prop_hangul_mode) {
        g_object_unref (hangul->prop_hangul_mode);
        hangul->prop_hangul_mode = NULL;
    }

    if (hangul->prop_hanja_mode) {
        g_object_unref (hangul->prop_hanja_mode);
        hangul->prop_hanja_mode = NULL;
    }

    if (hangul->prop_list) {
        g_object_unref (hangul->prop_list);
        hangul->prop_list = NULL;
    }

    if (hangul->table) {
        g_object_unref (hangul->table);
        hangul->table = NULL;
    }

    if (hangul->context) {
        hangul_ic_delete (hangul->context);
        hangul->context = NULL;
    }

    symbols = hangul->input_mode_symbols;
    for (i = 0; i < INPUT_MODE_COUNT; ++i) {
        if (symbols[i] != NULL) {
            g_object_unref(symbols[i]);
            symbols[i] = NULL;
        }
    }

    IBUS_OBJECT_CLASS (parent_class)->destroy ((IBusObject *)hangul);
}

static void
ibus_hangul_engine_clear_preedit_text (IBusHangulEngine *hangul)
{
    IBusText *text;

    text = ibus_text_new_from_static_string ("");
    ibus_engine_update_preedit_text ((IBusEngine *)hangul, text, 0, FALSE);
}

static void
ibus_hangul_engine_update_preedit_text (IBusHangulEngine *hangul)
{
    const ucschar *hic_preedit;
    IBusText *text;
    UString *preedit;
    gint preedit_len;

    // ibus-hangul's preedit string is made up of ibus context's
    // internal preedit string and libhangul's preedit string.
    // libhangul only supports one syllable preedit string.
    // In order to make longer preedit string, ibus-hangul maintains
    // internal preedit string.
    hic_preedit = hangul_ic_get_preedit_string (hangul->context);

    preedit = ustring_dup (hangul->preedit);
    preedit_len = ustring_length(preedit);
    ustring_append_ucs4 (preedit, hic_preedit, -1);

    if (ustring_length(preedit) > 0) {
	IBusPreeditFocusMode preedit_option = IBUS_ENGINE_PREEDIT_COMMIT;

	if (hangul->hanja_list != NULL)
	    preedit_option = IBUS_ENGINE_PREEDIT_CLEAR;

        text = ibus_text_new_from_ucs4 ((gunichar*)preedit->data);
        // ibus-hangul's internal preedit string
        ibus_text_append_attribute (text, IBUS_ATTR_TYPE_UNDERLINE,
                IBUS_ATTR_UNDERLINE_SINGLE, 0, preedit_len);
        // Preedit string from libhangul context.
        // This is currently composing syllable.
        ibus_text_append_attribute (text, IBUS_ATTR_TYPE_FOREGROUND,
                0x00ffffff, preedit_len, -1);
        ibus_text_append_attribute (text, IBUS_ATTR_TYPE_BACKGROUND,
                0x00000000, preedit_len, -1);
        ibus_engine_update_preedit_text_with_mode ((IBusEngine *)hangul,
                                                   text,
                                                   ibus_text_get_length (text),
                                                   TRUE,
                                                   preedit_option);
    } else {
        text = ibus_text_new_from_static_string ("");
        ibus_engine_update_preedit_text ((IBusEngine *)hangul, text, 0, FALSE);
    }

    ustring_delete(preedit);
}

static void
ibus_hangul_engine_update_lookup_table_ui (IBusHangulEngine *hangul)
{
    guint cursor_pos;
    const char* comment;
    IBusText* text;

    // update aux text
    cursor_pos = ibus_lookup_table_get_cursor_pos (hangul->table);
    comment = hanja_list_get_nth_comment (hangul->hanja_list, cursor_pos);

    text = ibus_text_new_from_string (comment);
    ibus_engine_update_auxiliary_text ((IBusEngine *)hangul, text, TRUE);

    // update lookup table
    ibus_engine_update_lookup_table ((IBusEngine *)hangul, hangul->table, TRUE);
}

static void
ibus_hangul_engine_commit_current_candidate (IBusHangulEngine *hangul)
{
    guint cursor_pos;
    const char* key;
    const char* value;
    const ucschar* hic_preedit;
    glong key_len;
    glong hic_preedit_len;
    glong preedit_len;

    IBusText* text;

    cursor_pos = ibus_lookup_table_get_cursor_pos (hangul->table);
    key = hanja_list_get_nth_key (hangul->hanja_list, cursor_pos);
    value = hanja_list_get_nth_value (hangul->hanja_list, cursor_pos);
    hic_preedit = hangul_ic_get_preedit_string (hangul->context);

    key_len = g_utf8_strlen(key, -1);
    preedit_len = ustring_length(hangul->preedit);
    hic_preedit_len = ucschar_strlen (hic_preedit);

    if (hangul->last_lookup_method == LOOKUP_METHOD_PREFIX) {
        if (preedit_len == 0 && hic_preedit_len == 0) {
            /* remove surrounding_text */
            if (key_len > 0) {
                ibus_engine_delete_surrounding_text ((IBusEngine *)hangul,
                        -key_len , key_len);
            }
        } else {
            /* remove ibus preedit text */
            if (key_len > 0) {
                glong n = MIN(key_len, preedit_len);
                ustring_erase (hangul->preedit, 0, n);
                key_len -= preedit_len;
            }

            /* remove hic preedit text */
            if (key_len > 0) {
                hangul_ic_reset (hangul->context);
                key_len -= hic_preedit_len;
            }
        }
    } else {
        /* remove hic preedit text */
        if (hic_preedit_len > 0) {
            hangul_ic_reset (hangul->context);
            key_len -= hic_preedit_len;
        }

        /* remove ibus preedit text */
        if (key_len > preedit_len) {
            ustring_erase (hangul->preedit, 0, preedit_len);
            key_len -= preedit_len;
        } else if (key_len > 0) {
            ustring_erase (hangul->preedit, 0, key_len);
            key_len = 0;
        }

        /* remove surrounding_text */
        if (key_len > 0) {
            ibus_engine_delete_surrounding_text ((IBusEngine *)hangul,
                    -key_len , key_len);
        }
    }

    /* clear preedit text before commit */
    ibus_hangul_engine_clear_preedit_text (hangul);

    text = ibus_text_new_from_string (value);
    ibus_engine_commit_text ((IBusEngine *)hangul, text);

    ibus_hangul_engine_update_preedit_text (hangul);
}

static gchar*
h_ibus_text_get_substring (IBusText* ibus_text, glong p1, glong p2)
{
    const gchar* text;
    const gchar* begin;
    const gchar* end;
    gchar* substring;
    glong limit;
    glong pos;
    glong n;

    text = ibus_text_get_text (ibus_text);
    limit = ibus_text_get_length (ibus_text) + 1;
    if (text == NULL || limit == 0)
        return NULL;

    p1 = MAX(0, p1);
    p2 = MAX(0, p2);

    pos = MIN(p1, p2);
    n = ABS(p2 - p1);

    if (pos + n > limit)
        n = limit - pos;

    begin = g_utf8_offset_to_pointer (text, pos);
    end = g_utf8_offset_to_pointer (begin, n);

    substring = g_strndup (begin, end - begin);
    return substring;
}

static HanjaList*
ibus_hangul_engine_lookup_hanja_table (const char* key, int method)
{
    HanjaList* list;

    if (key == NULL)
        return NULL;

    switch (method) {
    case LOOKUP_METHOD_EXACT:
        if (symbol_table != NULL)
            list = hanja_table_match_exact (symbol_table, key);

        if (list == NULL)
            list = hanja_table_match_exact (hanja_table, key);

        break;
    case LOOKUP_METHOD_PREFIX:
        if (symbol_table != NULL)
            list = hanja_table_match_prefix (symbol_table, key);

        if (list == NULL)
            list = hanja_table_match_prefix (hanja_table, key);

        break;
    case LOOKUP_METHOD_SUFFIX:
        if (symbol_table != NULL)
            list = hanja_table_match_suffix (symbol_table, key);

        if (list == NULL)
            list = hanja_table_match_suffix (hanja_table, key);

        break;
    }

    return list;
}

static void
ibus_hangul_engine_update_hanja_list (IBusHangulEngine *hangul)
{
    gchar* hanja_key;
    gchar* preedit_utf8;
    const ucschar* hic_preedit;
    UString* preedit;
    int lookup_method;
    IBusText* ibus_text = NULL;
    guint cursor_pos = 0;
    guint anchor_pos = 0;

    if (hangul->hanja_list != NULL) {
        hanja_list_delete (hangul->hanja_list);
        hangul->hanja_list = NULL;
    }

    hic_preedit = hangul_ic_get_preedit_string (hangul->context);

    hanja_key = NULL;
    lookup_method = LOOKUP_METHOD_PREFIX;

    preedit = ustring_dup (hangul->preedit);
    ustring_append_ucs4 (preedit, hic_preedit, -1);

    if (ustring_length(preedit) > 0) {
        preedit_utf8 = ustring_to_utf8 (preedit, -1);
        if (word_commit || hangul->hanja_mode) {
            hanja_key = preedit_utf8;
            lookup_method = LOOKUP_METHOD_PREFIX;
        } else {
            gchar* substr;
            ibus_engine_get_surrounding_text ((IBusEngine *)hangul, &ibus_text,
                    &cursor_pos, &anchor_pos);

            substr = h_ibus_text_get_substring (ibus_text,
                    cursor_pos - 64, cursor_pos);

            if (substr != NULL) {
                hanja_key = g_strconcat (substr, preedit_utf8, NULL);
                g_free (preedit_utf8);
            } else {
                hanja_key = preedit_utf8;
            }
            lookup_method = LOOKUP_METHOD_SUFFIX;
        }
    } else {
        ibus_engine_get_surrounding_text ((IBusEngine *)hangul, &ibus_text,
                &cursor_pos, &anchor_pos);
        if (cursor_pos != anchor_pos) {
            // If we have selection in surrounding text, we use that.
            hanja_key = h_ibus_text_get_substring (ibus_text,
                    cursor_pos, anchor_pos);
            lookup_method = LOOKUP_METHOD_EXACT;
        } else {
            hanja_key = h_ibus_text_get_substring (ibus_text,
                    cursor_pos - 64, cursor_pos);
            lookup_method = LOOKUP_METHOD_SUFFIX;
        }
    }

    if (hanja_key != NULL) {
        hangul->hanja_list = ibus_hangul_engine_lookup_hanja_table (hanja_key,
                lookup_method);
        hangul->last_lookup_method = lookup_method;
        g_free (hanja_key);
    }

    ustring_delete (preedit);

    if (ibus_text != NULL)
        g_object_unref (ibus_text);
}

static void
ibus_hangul_engine_apply_hanja_list (IBusHangulEngine *hangul)
{
    HanjaList* list = hangul->hanja_list;
    if (list != NULL) {
        int i, n;
        n = hanja_list_get_size (list);

        ibus_lookup_table_clear (hangul->table);
        for (i = 0; i < n; i++) {
            const char* value = hanja_list_get_nth_value (list, i);
            IBusText* text = ibus_text_new_from_string (value);
            ibus_lookup_table_append_candidate (hangul->table, text);
        }

        ibus_lookup_table_set_cursor_pos (hangul->table, 0);
        ibus_hangul_engine_update_lookup_table_ui (hangul);
        lookup_table_set_visible (hangul->table, TRUE);
    }
}

static void
ibus_hangul_engine_hide_lookup_table (IBusHangulEngine *hangul)
{
    gboolean is_visible;
    is_visible = lookup_table_is_visible (hangul->table);

    // Sending hide lookup table message when the lookup table
    // is not visible results wrong behavior. So I have to check
    // whether the table is visible or not before to hide.
    if (is_visible) {
        ibus_engine_hide_lookup_table ((IBusEngine *)hangul);
        ibus_engine_hide_auxiliary_text ((IBusEngine *)hangul);
        lookup_table_set_visible (hangul->table, FALSE);
    }

    if (hangul->hanja_list != NULL) {
        hanja_list_delete (hangul->hanja_list);
        hangul->hanja_list = NULL;
    }
}

static void
ibus_hangul_engine_update_lookup_table (IBusHangulEngine *hangul)
{
    ibus_hangul_engine_update_hanja_list (hangul);

    if (hangul->hanja_list != NULL) {
	// We should redraw preedit text with IBUS_ENGINE_PREEDIT_CLEAR option
	// here to prevent committing it on focus out event incidentally.
	ibus_hangul_engine_update_preedit_text (hangul);
        ibus_hangul_engine_apply_hanja_list (hangul);
    } else {
        ibus_hangul_engine_hide_lookup_table (hangul);
    }
}

static gboolean
ibus_hangul_engine_process_candidate_key_event (IBusHangulEngine    *hangul,
                                                guint                keyval,
                                                guint                modifiers)
{
    if (keyval == IBUS_Escape) {
        ibus_hangul_engine_hide_lookup_table (hangul);
	// When the lookup table is poped up, preedit string is 
	// updated with IBUS_ENGINE_PREEDIT_CLEAR option.
	// So, when focus is out, the preedit text will not be committed.
	// To prevent this problem, we have to update preedit text here
	// with IBUS_ENGINE_PREEDIT_COMMIT option.
	ibus_hangul_engine_update_preedit_text (hangul);
        return TRUE;
    } else if (keyval == IBUS_Return) {
        ibus_hangul_engine_commit_current_candidate (hangul);

        if (hangul->hanja_mode && ibus_hangul_engine_has_preedit (hangul)) {
            ibus_hangul_engine_update_lookup_table (hangul);
        } else {
            ibus_hangul_engine_hide_lookup_table (hangul);
        }
        return TRUE;
    } else if (keyval >= IBUS_1 && keyval <= IBUS_9) {
        guint page_no;
        guint page_size;
        guint cursor_pos;

        page_size = ibus_lookup_table_get_page_size (hangul->table);
        cursor_pos = ibus_lookup_table_get_cursor_pos (hangul->table);
        page_no = cursor_pos / page_size;

        cursor_pos = page_no * page_size + (keyval - IBUS_1);
        ibus_lookup_table_set_cursor_pos (hangul->table, cursor_pos);

        ibus_hangul_engine_commit_current_candidate (hangul);

        if (hangul->hanja_mode && ibus_hangul_engine_has_preedit (hangul)) {
            ibus_hangul_engine_update_lookup_table (hangul);
        } else {
            ibus_hangul_engine_hide_lookup_table (hangul);
        }
        return TRUE;
    } else if (keyval == IBUS_Page_Up) {
        ibus_lookup_table_page_up (hangul->table);
        ibus_hangul_engine_update_lookup_table_ui (hangul);
        return TRUE;
    } else if (keyval == IBUS_Page_Down) {
        ibus_lookup_table_page_down (hangul->table);
        ibus_hangul_engine_update_lookup_table_ui (hangul);
        return TRUE;
    } else {
        if (lookup_table_orientation == 0) {
            // horizontal
            if (keyval == IBUS_Left) {
                ibus_lookup_table_cursor_up (hangul->table);
                ibus_hangul_engine_update_lookup_table_ui (hangul);
                return TRUE;
            } else if (keyval == IBUS_Right) {
                ibus_lookup_table_cursor_down (hangul->table);
                ibus_hangul_engine_update_lookup_table_ui (hangul);
                return TRUE;
            } else if (keyval == IBUS_Up) {
                ibus_lookup_table_page_up (hangul->table);
                ibus_hangul_engine_update_lookup_table_ui (hangul);
                return TRUE;
            } else if (keyval == IBUS_Down) {
                ibus_lookup_table_page_down (hangul->table);
                ibus_hangul_engine_update_lookup_table_ui (hangul);
                return TRUE;
            }
        } else {
            // vertical
            if (keyval == IBUS_Left) {
                ibus_lookup_table_page_up (hangul->table);
                ibus_hangul_engine_update_lookup_table_ui (hangul);
                return TRUE;
            } else if (keyval == IBUS_Right) {
                ibus_lookup_table_page_down (hangul->table);
                ibus_hangul_engine_update_lookup_table_ui (hangul);
                return TRUE;
            } else if (keyval == IBUS_Up) {
                ibus_lookup_table_cursor_up (hangul->table);
                ibus_hangul_engine_update_lookup_table_ui (hangul);
                return TRUE;
            } else if (keyval == IBUS_Down) {
                ibus_lookup_table_cursor_down (hangul->table);
                ibus_hangul_engine_update_lookup_table_ui (hangul);
                return TRUE;
            }
        }
    }

    if (!hangul->hanja_mode) {
        if (lookup_table_orientation == 0) {
            // horizontal
            if (keyval == IBUS_h) {
                ibus_lookup_table_cursor_up (hangul->table);
                ibus_hangul_engine_update_lookup_table_ui (hangul);
                return TRUE;
            } else if (keyval == IBUS_l) {
                ibus_lookup_table_cursor_down (hangul->table);
                ibus_hangul_engine_update_lookup_table_ui (hangul);
                return TRUE;
            } else if (keyval == IBUS_k) {
                ibus_lookup_table_page_up (hangul->table);
                ibus_hangul_engine_update_lookup_table_ui (hangul);
                return TRUE;
            } else if (keyval == IBUS_j) {
                ibus_lookup_table_page_down (hangul->table);
                ibus_hangul_engine_update_lookup_table_ui (hangul);
                return TRUE;
            }
        } else {
            // vertical
            if (keyval == IBUS_h) {
                ibus_lookup_table_page_up (hangul->table);
                ibus_hangul_engine_update_lookup_table_ui (hangul);
                return TRUE;
            } else if (keyval == IBUS_l) {
                ibus_lookup_table_page_down (hangul->table);
                ibus_hangul_engine_update_lookup_table_ui (hangul);
                return TRUE;
            } else if (keyval == IBUS_k) {
                ibus_lookup_table_cursor_up (hangul->table);
                ibus_hangul_engine_update_lookup_table_ui (hangul);
                return TRUE;
            } else if (keyval == IBUS_j) {
                ibus_lookup_table_cursor_down (hangul->table);
                ibus_hangul_engine_update_lookup_table_ui (hangul);
                return TRUE;
            }
        }
    }

    return FALSE;
}

static gboolean
ibus_hangul_engine_process_key_event (IBusEngine     *engine,
                                      guint           keyval,
                                      guint           keycode,
                                      guint           modifiers)
{
    IBusHangulEngine *hangul = (IBusHangulEngine *) engine;

    guint mask;
    gboolean retval;
    const ucschar *str;

    if (modifiers & IBUS_RELEASE_MASK)
        return FALSE;

    // if we don't ignore shift keys, shift key will make flush the preedit 
    // string. So you cannot input shift+key.
    // Let's think about these examples:
    //   dlTek (2 set)
    //   qhRdmaqkq (2 set)
    if (keyval == IBUS_Shift_L || keyval == IBUS_Shift_R)
        return FALSE;

    // If a hotkey has any modifiers, we ignore that modifier
    // keyval, or we cannot make the hanja key work.
    // Because when we get the modifier key alone, we commit the
    // current preedit string. So after that, even if we get the
    // right hanja key event, we don't have preedit string to be changed
    // to hanja word.
    // See this bug: http://code.google.com/p/ibus/issues/detail?id=1036
    if (hotkey_list_has_modifier(&switch_keys, keyval))
        return FALSE;

    if (hotkey_list_match(&switch_keys, keyval, modifiers)) {
        ibus_hangul_engine_toggle_input_mode (hangul);
        return TRUE;
    }

    if (hotkey_list_match (&on_keys, keyval, modifiers)) {
        ibus_hangul_engine_set_input_mode (hangul, INPUT_MODE_HANGUL);
        return FALSE;
    }

    if (hangul->input_mode == INPUT_MODE_DIRECT)
        return FALSE;

    /* This feature is for vi* users.
     * On Esc, the input mode is changed to latin */
    if (hotkey_list_match (&off_keys, keyval, modifiers)) {
        ibus_hangul_engine_set_input_mode (hangul, INPUT_MODE_DIRECT);
        /* If we return TRUE, then vi will not receive "ESC" key event. */
        return FALSE;
    }

    if (hotkey_list_has_modifier(&hanja_keys, keyval))
	return FALSE; 

    if (hotkey_list_match(&hanja_keys, keyval, modifiers)) {
        if (hangul->hanja_list == NULL) {
            ibus_hangul_engine_update_lookup_table (hangul);
        } else {
            ibus_hangul_engine_hide_lookup_table (hangul);
        }
        return TRUE;
    }

    if (hangul->hanja_list != NULL) {
        retval = ibus_hangul_engine_process_candidate_key_event (hangul,
                     keyval, modifiers);
        if (hangul->hanja_mode) {
            if (retval)
                return TRUE;
        } else {
            return TRUE;
        }
    }

    // If we've got a key event with some modifiers, commit current
    // preedit string and ignore this key event.
    // So, if you want to add some key event handler, put it 
    // before this code.
    // Ignore key event with control, alt, super or mod5
    mask = IBUS_CONTROL_MASK |
	    IBUS_MOD1_MASK | IBUS_MOD3_MASK | IBUS_MOD4_MASK | IBUS_MOD5_MASK;
    if (modifiers & mask) {
        ibus_hangul_engine_flush (hangul);
        return FALSE;
    }

    if (keyval == IBUS_BackSpace) {
        retval = hangul_ic_backspace (hangul->context);
        if (!retval) {
            guint preedit_len = ustring_length (hangul->preedit);
            if (preedit_len > 0) {
                ustring_erase (hangul->preedit, preedit_len - 1, 1);
                retval = TRUE;
            }
        }

        ibus_hangul_engine_update_preedit_text (hangul);

        if (hangul->hanja_mode) {
            if (ibus_hangul_engine_has_preedit (hangul)) {
                ibus_hangul_engine_update_lookup_table (hangul);
            } else {
                ibus_hangul_engine_hide_lookup_table (hangul);
            }
        }
    } else {
	// We need to normalize the keyval to US qwerty keylayout,
	// because the korean input method is depend on the position of
	// each key, not the character. We make the keyval from keycode
	// as if the keyboard is US qwerty layout. Then we can assume the
	// keyval represent the position of the each key.
	// But if the hic is in transliteration mode, then we should not
	// normalize the keyval.
	bool is_transliteration_mode =
		 hangul_ic_is_transliteration(hangul->context);
	if (!is_transliteration_mode) {
	    if (keymap != NULL)
		keyval = ibus_keymap_lookup_keysym(keymap, keycode, modifiers);
	}

        // ignore capslock
        if (modifiers & IBUS_LOCK_MASK) {
            if (keyval >= 'A' && keyval <= 'z') {
                if (isupper(keyval))
                    keyval = tolower(keyval);
                else
                    keyval = toupper(keyval);
            }
        }
        retval = hangul_ic_process (hangul->context, keyval);

        str = hangul_ic_get_commit_string (hangul->context);
        if (word_commit || hangul->hanja_mode) {
            const ucschar* hic_preedit;

            hic_preedit = hangul_ic_get_preedit_string (hangul->context);
            if (hic_preedit != NULL && hic_preedit[0] != 0) {
                ustring_append_ucs4 (hangul->preedit, str, -1);
            } else {
                IBusText *text;
                const ucschar* preedit;

                ustring_append_ucs4 (hangul->preedit, str, -1);
                if (ustring_length (hangul->preedit) > 0) {
                    /* clear preedit text before commit */
                    ibus_hangul_engine_clear_preedit_text (hangul);

                    preedit = ustring_begin (hangul->preedit);
                    text = ibus_text_new_from_ucs4 ((gunichar*)preedit);
                    ibus_engine_commit_text (engine, text);
                }
                ustring_clear (hangul->preedit);
            }
        } else {
            if (str != NULL && str[0] != 0) {
                IBusText *text;

                /* clear preedit text before commit */
                ibus_hangul_engine_clear_preedit_text (hangul);

                text = ibus_text_new_from_ucs4 (str);
                ibus_engine_commit_text (engine, text);
            }
        }

        ibus_hangul_engine_update_preedit_text (hangul);

        if (hangul->hanja_mode) {
            ibus_hangul_engine_update_lookup_table (hangul);
        }

        if (!retval)
            ibus_hangul_engine_flush (hangul);
    }

    return retval;
}

static void
ibus_hangul_engine_flush (IBusHangulEngine *hangul)
{
    const gunichar *str;
    IBusText *text;

    ibus_hangul_engine_hide_lookup_table (hangul);

    str = hangul_ic_flush (hangul->context);

    ustring_append_ucs4 (hangul->preedit, str, -1);

    if (ustring_length (hangul->preedit) != 0) {
        /* clear preedit text before commit */
        ibus_hangul_engine_clear_preedit_text (hangul);

	str = ustring_begin (hangul->preedit);
	text = ibus_text_new_from_ucs4 (str);

	ibus_engine_commit_text ((IBusEngine *) hangul, text);

	ustring_clear(hangul->preedit);
    }

    ibus_hangul_engine_update_preedit_text (hangul);
}

static void
ibus_hangul_engine_focus_in (IBusEngine *engine)
{
    IBusHangulEngine *hangul = (IBusHangulEngine *) engine;

    if (hangul->input_mode == INPUT_MODE_HANGUL) {
        ibus_property_set_state (hangul->prop_hangul_mode, PROP_STATE_CHECKED);
    } else {
        ibus_property_set_state (hangul->prop_hangul_mode, PROP_STATE_UNCHECKED);
    }

    if (hangul->hanja_mode) {
        ibus_property_set_state (hangul->prop_hanja_mode, PROP_STATE_CHECKED);
    } else {
        ibus_property_set_state (hangul->prop_hanja_mode, PROP_STATE_UNCHECKED);
    }

    ibus_engine_register_properties (engine, hangul->prop_list);

    ibus_hangul_engine_update_preedit_text (hangul);

    if (hangul->hanja_list != NULL) {
        ibus_hangul_engine_update_lookup_table_ui (hangul);
    }

    parent_class->focus_in (engine);
}

static void
ibus_hangul_engine_focus_out (IBusEngine *engine)
{
    IBusHangulEngine *hangul = (IBusHangulEngine *) engine;

    if (hangul->hanja_list == NULL) {
	// ibus-hangul uses
	// ibus_engine_update_preedit_text_with_mode() function which makes
	// the preedit string committed automatically when the focus is out.
	// So we don't need to commit the preedit here.
	hangul_ic_reset (hangul->context);
    } else {
        ibus_engine_hide_lookup_table (engine);
        ibus_engine_hide_auxiliary_text (engine);
    }

    parent_class->focus_out ((IBusEngine *) hangul);
}

static void
ibus_hangul_engine_reset (IBusEngine *engine)
{
    IBusHangulEngine *hangul = (IBusHangulEngine *) engine;

    if (_does_ibus_commit_preedit_text_before_reset) {
        ibus_hangul_engine_hide_lookup_table (hangul);
        hangul_ic_reset (hangul->context);

        if (ustring_length (hangul->preedit) != 0)
            ustring_clear(hangul->preedit);

        ibus_hangul_engine_update_preedit_text (hangul);
    } else {
        ibus_hangul_engine_flush (hangul);
    }

    parent_class->reset (engine);
}

static void
ibus_hangul_engine_enable (IBusEngine *engine)
{
    parent_class->enable (engine);

    ibus_engine_get_surrounding_text (engine, NULL, NULL, NULL);
}

static void
ibus_hangul_engine_disable (IBusEngine *engine)
{
    ibus_hangul_engine_focus_out (engine);
    parent_class->disable (engine);
}

static void
ibus_hangul_engine_page_up (IBusEngine *engine)
{
    parent_class->page_up (engine);
}

static void
ibus_hangul_engine_page_down (IBusEngine *engine)
{
    parent_class->page_down (engine);
}

static void
ibus_hangul_engine_cursor_up (IBusEngine *engine)
{
    IBusHangulEngine *hangul = (IBusHangulEngine *) engine;

    if (hangul->hanja_list != NULL) {
        ibus_lookup_table_cursor_up (hangul->table);
        ibus_hangul_engine_update_lookup_table_ui (hangul);
    }

    parent_class->cursor_up (engine);
}

static void
ibus_hangul_engine_cursor_down (IBusEngine *engine)
{
    IBusHangulEngine *hangul = (IBusHangulEngine *) engine;

    if (hangul->hanja_list != NULL) {
        ibus_lookup_table_cursor_down (hangul->table);
        ibus_hangul_engine_update_lookup_table_ui (hangul);
    }

    parent_class->cursor_down (engine);
}

static void
ibus_hangul_engine_property_activate (IBusEngine    *engine,
                                      const gchar   *prop_name,
                                      guint          prop_state)
{
    if (strcmp(prop_name, "setup") == 0) {
        GError *error = NULL;
        gchar *argv[2] = { NULL, };

        argv[0] = "ibus-setup-hangul";
        argv[1] = NULL;
        g_spawn_async (NULL, argv, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, NULL, &error);
    } else if (strcmp(prop_name, "InputMode") == 0) {
        IBusHangulEngine *hangul = (IBusHangulEngine *) engine;

        ibus_hangul_engine_toggle_input_mode (hangul);
    } else if (strcmp(prop_name, "hanja_mode") == 0) {
        IBusHangulEngine *hangul = (IBusHangulEngine *) engine;

        hangul->hanja_mode = !hangul->hanja_mode;
        if (hangul->hanja_mode) {
            ibus_property_set_state (hangul->prop_hanja_mode,
                    PROP_STATE_CHECKED);
        } else {
            ibus_property_set_state (hangul->prop_hanja_mode,
                    PROP_STATE_UNCHECKED);
        }

        ibus_engine_update_property (engine, hangul->prop_hanja_mode);
        ibus_hangul_engine_flush (hangul);
    }
}

static gboolean
ibus_hangul_engine_has_preedit (IBusHangulEngine *hangul)
{
    guint preedit_len;
    const ucschar *hic_preedit;

    hic_preedit = hangul_ic_get_preedit_string (hangul->context);
    if (hic_preedit[0] != 0)
        return TRUE;

    preedit_len = ustring_length (hangul->preedit);
    if (preedit_len > 0)
        return TRUE;

    return FALSE;
}

static void
ibus_hangul_engine_toggle_input_mode (IBusHangulEngine *hangul)
{
    int input_mode;

    if (hangul->input_mode == INPUT_MODE_HANGUL)
        input_mode = INPUT_MODE_DIRECT;
    else
        input_mode = INPUT_MODE_HANGUL;

    ibus_hangul_engine_set_input_mode (hangul, input_mode);
}

static IBusText *
ibus_hangul_engine_get_input_mode_symbol (IBusHangulEngine *hangul,
                                          int input_mode)
{
    IBusText **symbols = hangul->input_mode_symbols;

    if (symbols[0] == NULL) {
        symbols[INPUT_MODE_DIRECT] = ibus_text_new_from_string ("A");
        g_object_ref_sink(symbols[INPUT_MODE_DIRECT]);
        symbols[INPUT_MODE_HANGUL] = ibus_text_new_from_string ("한");
        g_object_ref_sink(symbols[INPUT_MODE_HANGUL]);
    }

    if (input_mode >= INPUT_MODE_COUNT)
        return symbols[INPUT_MODE_HANGUL];

    return symbols[input_mode];
}

static void
ibus_hangul_engine_set_input_mode (IBusHangulEngine *hangul, int input_mode)
{
    IBusText* symbol;
    IBusProperty* prop;

    ibus_hangul_engine_flush (hangul);

    prop = hangul->prop_hangul_mode;

    hangul->input_mode = input_mode;

    symbol = ibus_hangul_engine_get_input_mode_symbol (hangul, input_mode);
    ibus_property_set_symbol(prop, symbol);

    if (hangul->input_mode == INPUT_MODE_HANGUL) {
        ibus_property_set_state (prop, PROP_STATE_CHECKED);
    } else {
        ibus_property_set_state (prop, PROP_STATE_UNCHECKED);
    }

    ibus_engine_update_property (IBUS_ENGINE (hangul), prop);
}

static bool
ibus_hangul_engine_on_transition (HangulInputContext     *hic,
                                  ucschar                 c,
                                  const ucschar          *preedit,
                                  void                   *data)
{
    if (!auto_reorder) {
        if (hangul_is_choseong (c)) {
            if (hangul_ic_has_jungseong (hic) || hangul_ic_has_jongseong (hic))
                return false;
        }

        if (hangul_is_jungseong (c)) {
            if (hangul_ic_has_jongseong (hic))
                return false;
        }
    }

    return true;
}

static void
ibus_config_value_changed (IBusConfig   *config,
                           const gchar  *section,
                           const gchar  *name,
                           GVariant     *value,
                           gpointer      user_data)
{
    IBusHangulEngine *hangul = (IBusHangulEngine *) user_data;

    if (strcmp(section, "engine/hangul") == 0) {
        if (strcmp(name, "hangul-keyboard") == 0) {
            const gchar *str = g_variant_get_string(value, NULL);
            g_string_assign (hangul_keyboard, str);
            hangul_ic_select_keyboard (hangul->context, hangul_keyboard->str);
        } else if (strcmp(name, "hanja-keys") == 0) {
            const gchar* str = g_variant_get_string(value, NULL);
	    hotkey_list_set_from_string(&hanja_keys, str);
        } else if (strcmp(name, "word-commit") == 0) {
            word_commit = g_variant_get_boolean (value);
        } else if (strcmp (name, "auto-reorder") == 0) {
            auto_reorder = g_variant_get_boolean (value);
        } else if (strcmp (name, "switch-keys") == 0) {
            const gchar* str = g_variant_get_string(value, NULL);
	    hotkey_list_set_from_string(&switch_keys, str);
        } else if (strcmp (name, "on-keys") == 0) {
            const gchar* str = g_variant_get_string(value, NULL);
	    hotkey_list_set_from_string(&on_keys, str);
        } else if (strcmp (name, "off-keys") == 0) {
            const gchar* str = g_variant_get_string(value, NULL);
	    hotkey_list_set_from_string(&off_keys, str);
        }
    } else if (strcmp(section, "panel") == 0) {
        if (strcmp(name, "lookup-table-orientation") == 0) {
            lookup_table_orientation = g_variant_get_int32(value);
        }
    }
}

static void
lookup_table_set_visible (IBusLookupTable *table, gboolean flag)
{
    g_object_set_data (G_OBJECT(table), "visible", GUINT_TO_POINTER(flag));
}

static gboolean
lookup_table_is_visible (IBusLookupTable *table)
{
    gpointer res = g_object_get_data (G_OBJECT(table), "visible");
    return GPOINTER_TO_UINT(res);
}

static void
key_event_list_append(GArray* list, guint keyval, guint modifiers)
{
    struct KeyEvent ev = { keyval, modifiers};
    g_array_append_val(list, ev);
}

static gboolean
key_event_list_match(GArray* list, guint keyval, guint modifiers)
{
    guint i;
    guint mask;

    /* ignore capslock and numlock */
    mask = IBUS_SHIFT_MASK |
           IBUS_CONTROL_MASK |
           IBUS_MOD1_MASK |
           IBUS_MOD3_MASK |
           IBUS_MOD4_MASK |
           IBUS_MOD5_MASK;

    modifiers &= mask;
    for (i = 0; i < list->len; ++i) {
        struct KeyEvent* ev = &g_array_index(list, struct KeyEvent, i);
        if (ev->keyval == keyval && ev->modifiers == modifiers) {
            return TRUE;
        }
    }

    return FALSE;
}

static void
ibus_hangul_engine_candidate_clicked (IBusEngine     *engine,
                                      guint           index,
                                      guint           button,
                                      guint           state)
{
    IBusHangulEngine *hangul = (IBusHangulEngine *) engine;
    if (hangul == NULL)
	return;

    if (hangul->table == NULL)
	return;

    ibus_lookup_table_set_cursor_pos (hangul->table, index);
    ibus_hangul_engine_commit_current_candidate (hangul);

    if (hangul->hanja_mode) {
	ibus_hangul_engine_update_lookup_table (hangul);
    } else {
	ibus_hangul_engine_hide_lookup_table (hangul);
    }
}

static void
hotkey_list_init(HotkeyList* list)
{
    list->all_modifiers = 0;
    list->keys = g_array_sized_new(FALSE, TRUE, sizeof(struct KeyEvent), 4);
}

static void
hotkey_list_fini(HotkeyList* list)
{
    g_array_free(list->keys, TRUE);
    list->keys = NULL;
}

static void
hotkey_list_append_from_string(HotkeyList *list, const char* str)
{
    guint keyval = 0;
    guint modifiers = 0;
    gboolean res;

    res = ibus_key_event_from_string(str, &keyval, &modifiers);
    if (res) {
	hotkey_list_append(list, keyval, modifiers);
    }
}

static void
hotkey_list_append(HotkeyList *list, guint keyval, guint modifiers)
{
    list->all_modifiers |= modifiers;
    key_event_list_append(list->keys, keyval, modifiers);
}

static void
hotkey_list_set_from_string(HotkeyList *list, const char* str)
{
    gchar** items = g_strsplit(str, ",", 0);

    list->all_modifiers = 0;
    g_array_set_size(list->keys, 0);

    if (items != NULL) {
        int i;
        for (i = 0; items[i] != NULL; ++i) {
	    hotkey_list_append_from_string(list, items[i]);
        }
        g_strfreev(items);
    }
}

static gboolean
hotkey_list_match(HotkeyList* list, guint keyval, guint modifiers)
{
    return key_event_list_match(list->keys, keyval, modifiers);
}

static gboolean
hotkey_list_has_modifier(HotkeyList* list, guint keyval)
{
    if (list->all_modifiers & IBUS_CONTROL_MASK) {
	if (keyval == IBUS_Control_L || keyval == IBUS_Control_R)
	    return TRUE;
    }

    if (list->all_modifiers & IBUS_MOD1_MASK) {
	if (keyval == IBUS_Alt_L || keyval == IBUS_Alt_R)
	    return TRUE;
    }

    if (list->all_modifiers & IBUS_SUPER_MASK) {
	if (keyval == IBUS_Super_L || keyval == IBUS_Super_R)
	    return TRUE;
    }

    if (list->all_modifiers & IBUS_HYPER_MASK) {
	if (keyval == IBUS_Hyper_L || keyval == IBUS_Hyper_R)
	    return TRUE;
    }

    if (list->all_modifiers & IBUS_META_MASK) {
	if (keyval == IBUS_Meta_L || keyval == IBUS_Meta_R)
	    return TRUE;
    }

    return FALSE;
}
