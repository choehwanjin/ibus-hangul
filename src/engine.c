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

#include <stdio.h>
#include <ibus.h>
#include <gio/gio.h>
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
    INPUT_MODE_HANGUL,
    INPUT_MODE_LATIN,
    INPUT_MODE_COUNT,
};

/**
 * ibus-hangul supports 3 preedit modes.
 */
typedef enum {
    /**
     * @brief zero length preedit mode
     * An option to use zero length preedit text.
     * ibus-hangul will utilize surrounding text feature to draw "composing text".
     * So the "composing text" will not be shown in preedit style but normal text.
     */
    PREEDIT_MODE_NONE,
    /**
     * @brief syllable preedit mode
     * An option to use syllable length preedit text.
     * This option utilizes preedit text feature.
     */
    PREEDIT_MODE_SYLLABLE,
    /**
     * @brief word preedit mode
     * An option to use word length preedit text.
     */
    PREEDIT_MODE_WORD,
} IBusHangulPreeditMode;

struct _IBusHangulEngine {
    IBusEngineSimple parent;

    /* members */
    /* unique context id */
    guint id;

    HangulInputContext *context;
    UString* preedit;
    /* Every instance's preedit_mode may be different from global settings.
     * So we need a value for each instance. */
    IBusHangulPreeditMode preedit_mode;
    int input_mode;
    unsigned int input_purpose;
    gboolean hanja_mode;
    HanjaList* hanja_list;
    int last_lookup_method;

    guint caps;
    IBusLookupTable *table;

    IBusProperty    *prop_hangul_mode;
    IBusProperty    *prop_hanja_mode;
    IBusPropList    *prop_list;

    IBusText        *input_mode_symbols[INPUT_MODE_COUNT];
};

struct _IBusHangulEngineClass {
    IBusEngineSimpleClass parent;
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
#endif
static void ibus_hangul_engine_set_capabilities
                                            (IBusEngine             *engine,
                                             guint                   caps);
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
static void ibus_hangul_engine_set_content_type
                                            (IBusEngine             *engine,
                                             guint                   purpose,
                                             guint                   hints);

static void ibus_hangul_engine_flush        (IBusHangulEngine       *hangul);
static void ibus_hangul_engine_clear_preedit_text
                                            (IBusHangulEngine       *hangul);
static void ibus_hangul_engine_update_preedit_text
                                            (IBusHangulEngine       *hangul);
static void ibus_hangul_engine_process_commit_and_edit
                                            (IBusHangulEngine       *hangul);

static void ibus_hangul_engine_update_lookup_table
                                            (IBusHangulEngine       *hangul);
static gboolean ibus_hangul_engine_has_preedit
                                            (IBusHangulEngine       *hangul);
static void ibus_hangul_engine_switch_input_mode
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

static void        settings_changed         (GSettings              *settings,
                                             const gchar            *key,
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

static gint ibus_version[3] = { IBUS_MAJOR_VERSION, IBUS_MINOR_VERSION, IBUS_MICRO_VERSION };

static IBusEngineSimpleClass *parent_class = NULL;
static guint last_context_id = 0;
static HanjaTable *hanja_table = NULL;
static HanjaTable *symbol_table = NULL;
static GSettings *settings_hangul = NULL;
static GSettings *settings_panel = NULL;
static GString    *hangul_keyboard = NULL;
static HotkeyList hanja_keys;
static HotkeyList switch_keys;
static HotkeyList on_keys;
static HotkeyList off_keys;
static int lookup_table_orientation = 0;
static IBusKeymap *keymap = NULL;
static gboolean word_commit = FALSE;
static gboolean auto_reorder = TRUE;
static gboolean disable_latin_mode = FALSE;
static int initial_input_mode = INPUT_MODE_LATIN;
/**
 * whether to use event forwarding workaround
 * See: https://github.com/libhangul/ibus-hangul/issues/42
 */
static gboolean use_event_forwarding = TRUE;
/**
 * whether to use client commit
 * See: https://github.com/libhangul/ibus-hangul/pull/68
 */
static gboolean use_client_commit = FALSE;

/**
 * global preedit mode
 * This option may have a value, one of the IBusHangulPreeditMode.
 * See: https://github.com/libhangul/ibus-hangul/issues/69
 */
static IBusHangulPreeditMode global_preedit_mode = PREEDIT_MODE_SYLLABLE;


static glong
ucschar_strlen (const ucschar* str)
{
    const ucschar* p = str;
    while (*p != 0)
        p++;
    return p - str;
}

static void
check_ibus_version ()
{
    gboolean retval;
    gchar* standard_output = NULL;
    const gchar* version_str;
    gchar** version_str_array;
    gint version[3];

    retval = g_spawn_command_line_sync ("ibus version", &standard_output,
                                        NULL, NULL, NULL);
    if (!retval)
        goto fail;

    version_str = strpbrk (standard_output, " \t");
    if (version_str == NULL) {
        g_free (standard_output);
        goto fail;
    }

    while (*version_str == ' ' || *version_str == '\t')
        version_str++;

    version_str_array = g_strsplit (version_str, ".", 3);
    g_free (standard_output);
    if (version_str_array == NULL) {
        goto fail;
    }

    version[0] = 0;
    version[1] = 0;
    version[2] = 0;
    for (gint i = 0; version_str_array[i] != NULL; ++i) {
        version[i] = g_ascii_strtoll (version_str_array[i], NULL, 10);
    }
    g_strfreev (version_str_array);

    if (version[0] == 0 && version[1] == 0 && version[2] == 0) {
        goto fail;
    }

    ibus_version[0] = version[0];
    ibus_version[1] = version[1];
    ibus_version[2] = version[2];
    g_debug ("ibus version detected: %d.%d.%d",
            ibus_version[0], ibus_version[1], ibus_version[2]);
    exit(0);
    return;

fail:
    g_debug ("ibus version detection failed: use default value: %d.%d.%d",
            ibus_version[0], ibus_version[1], ibus_version[2]);
    exit(-1);
}

static gboolean
ibus_hangul_check_ibus_version (gint required_major,
                                gint required_minor,
                                gint required_micro)
{
    gint major = ibus_version[0];
    gint minor = ibus_version[1];
    gint micro = ibus_version[2];

    return major > required_major ||
        (major == required_major && minor > required_minor) ||
        (major == required_major && minor == required_minor &&
         micro >= required_micro);
}

static gboolean
check_client_commit ()
{
    gboolean client_commit = FALSE;

    client_commit = ibus_hangul_check_ibus_version (1, 5, 20);

    g_debug ("client_commit: %d", client_commit);

    return client_commit;
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
            type = g_type_register_static (IBUS_TYPE_ENGINE_SIMPLE,
                                           "IBusHangulEngine",
                                           &type_info,
                                           (GTypeFlags) 0);
    }

    return type;
}

void
ibus_hangul_init (IBusBus *bus)
{
    GVariant* value = NULL;

    last_context_id = 0;

    hanja_table = hanja_table_load (NULL);

    symbol_table = hanja_table_load (IBUSHANGUL_DATADIR "/data/symbol.txt");

    check_ibus_version ();

    settings_hangul = g_settings_new ("org.freedesktop.ibus.engine.hangul");
    settings_panel = g_settings_new ("org.freedesktop.ibus.panel");

    hangul_keyboard = g_string_new_len (NULL, 8);
    value = g_settings_get_value (settings_hangul, "hangul-keyboard");
    if (value != NULL) {
        const gchar* str = g_variant_get_string (value, NULL);
        g_string_assign (hangul_keyboard, str);
        g_clear_pointer (&value, g_variant_unref);
    }

    hotkey_list_init(&switch_keys);

    value = g_settings_get_value (settings_hangul, "switch-keys");
    if (value != NULL) {
        const gchar* str = g_variant_get_string (value, NULL);
        hotkey_list_set_from_string(&switch_keys, str);
        g_clear_pointer (&value, g_variant_unref);
    } else {
        hotkey_list_append(&switch_keys, IBUS_Hangul, 0);
        hotkey_list_append(&switch_keys, IBUS_space, IBUS_SHIFT_MASK);
    }

    hotkey_list_init(&hanja_keys);

    value = g_settings_get_value (settings_hangul, "hanja-keys");
    if (value != NULL) {
        const gchar* str = g_variant_get_string (value, NULL);
        hotkey_list_set_from_string(&hanja_keys, str);
        g_clear_pointer (&value, g_variant_unref);
    } else {
        hotkey_list_append(&hanja_keys, IBUS_Hangul_Hanja, 0);
        hotkey_list_append(&hanja_keys, IBUS_F9, 0);
    }

    hotkey_list_init (&on_keys);
    value = g_settings_get_value (settings_hangul, "on-keys");
    if (value != NULL) {
        const gchar* str = g_variant_get_string (value, NULL);
        hotkey_list_set_from_string (&on_keys, str);
        g_clear_pointer (&value, g_variant_unref);
    }

    hotkey_list_init (&off_keys);
    value = g_settings_get_value (settings_hangul, "off-keys");
    if (value != NULL) {
        const gchar* str = g_variant_get_string (value, NULL);
        hotkey_list_set_from_string (&off_keys, str);
        g_clear_pointer (&value, g_variant_unref);
    }

    value = g_settings_get_value (settings_hangul, "word-commit");
    if (value != NULL) {
        word_commit = g_variant_get_boolean (value);
        g_clear_pointer (&value, g_variant_unref);
    }

    value = g_settings_get_value (settings_hangul, "auto-reorder");
    if (value != NULL) {
        auto_reorder = g_variant_get_boolean (value);
        g_clear_pointer (&value, g_variant_unref);
    }

    value = g_settings_get_value (settings_hangul, "disable-latin-mode");
    if (value != NULL) {
        disable_latin_mode = g_variant_get_boolean (value);
        g_clear_pointer (&value, g_variant_unref);
    }

    value = g_settings_get_value (settings_hangul, "initial-input-mode");
    if (value != NULL) {
        const gchar* str = g_variant_get_string (value, NULL);
        if (strcmp(str, "latin") == 0) {
            initial_input_mode = INPUT_MODE_LATIN;
        } else if (strcmp(str, "hangul") == 0) {
            initial_input_mode = INPUT_MODE_HANGUL;
        }
        g_clear_pointer (&value, g_variant_unref);
    }

    value = g_settings_get_value (settings_hangul, "use-event-forwarding");
    if (value != NULL) {
        use_event_forwarding = g_variant_get_boolean (value);
        g_clear_pointer (&value, g_variant_unref);
    }

    value = g_settings_get_value (settings_hangul, "preedit-mode");
    if (value != NULL) {
        const gchar* str = g_variant_get_string (value, NULL);
        if (strcmp(str, "none") == 0) {
            global_preedit_mode = PREEDIT_MODE_NONE;
        } else if (strcmp(str, "word") == 0) {
            global_preedit_mode = PREEDIT_MODE_WORD;
        } else {
            global_preedit_mode = PREEDIT_MODE_SYLLABLE;
        }
        g_clear_pointer (&value, g_variant_unref);
    }

    value = g_settings_get_value (settings_panel, "lookup-table-orientation");
    if (value != NULL) {
        lookup_table_orientation = g_variant_get_int32(value);
        g_clear_pointer (&value, g_variant_unref);
    }

    keymap = ibus_keymap_get("us");
    use_client_commit = check_client_commit ();

    g_debug ("init");
}

void
ibus_hangul_exit (void)
{
    g_debug ("exit");

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

    g_clear_object (&settings_hangul);
    g_clear_object (&settings_panel);

    g_string_free (hangul_keyboard, TRUE);
    hangul_keyboard = NULL;
}

/*
static void
ibus_hangul_engine_set_surrounding_text (IBusEngine     *engine,
                                         IBusText       *text,
                                         guint           cursor_index,
                                         guint           anchor_pos)

{
    g_debug ("set_surrounding_text: %s %d %d",
            ibus_text_get_text (text), cursor_index, anchor_pos);
}
*/

static void
ibus_hangul_engine_class_init (IBusHangulEngineClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    IBusObjectClass *ibus_object_class = IBUS_OBJECT_CLASS (klass);
    IBusEngineClass *engine_class = IBUS_ENGINE_CLASS (klass);

    parent_class = (IBusEngineSimpleClass *) g_type_class_peek_parent (klass);

    object_class->constructor = ibus_hangul_engine_constructor;
    ibus_object_class->destroy = (IBusObjectDestroyFunc) ibus_hangul_engine_destroy;

    engine_class->process_key_event = ibus_hangul_engine_process_key_event;

    engine_class->reset = ibus_hangul_engine_reset;
    engine_class->enable = ibus_hangul_engine_enable;
    engine_class->disable = ibus_hangul_engine_disable;

    engine_class->focus_in = ibus_hangul_engine_focus_in;
    engine_class->focus_out = ibus_hangul_engine_focus_out;

    engine_class->set_capabilities = ibus_hangul_engine_set_capabilities;
    engine_class->page_up = ibus_hangul_engine_page_up;
    engine_class->page_down = ibus_hangul_engine_page_down;

    engine_class->cursor_up = ibus_hangul_engine_cursor_up;
    engine_class->cursor_down = ibus_hangul_engine_cursor_down;

    engine_class->property_activate = ibus_hangul_engine_property_activate;

    engine_class->candidate_clicked = ibus_hangul_engine_candidate_clicked;
    //engine_class->set_surrounding_text = ibus_hangul_engine_set_surrounding_text;
    engine_class->set_content_type = ibus_hangul_engine_set_content_type;
}

static void
ibus_hangul_engine_init (IBusHangulEngine *hangul)
{
    IBusProperty* prop;
    IBusText* label;
    IBusText* tooltip;
    IBusText* symbol;

    hangul->id = last_context_id;
    ++last_context_id;

    hangul->context = hangul_ic_new (hangul_keyboard->str);
    hangul_ic_connect_callback (hangul->context, "transition",
                                ibus_hangul_engine_on_transition, hangul);

    hangul->preedit = ustring_new();
    hangul->preedit_mode = global_preedit_mode;
    hangul->hanja_list = NULL;
    hangul->input_mode = initial_input_mode;
    hangul->input_purpose = IBUS_INPUT_PURPOSE_FREE_FORM;
    hangul->hanja_mode = FALSE;
    hangul->last_lookup_method = LOOKUP_METHOD_PREFIX;
    hangul->caps = 0;

    if (disable_latin_mode) {
        hangul->input_mode = INPUT_MODE_HANGUL;
    }

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
                                                       hangul->input_mode);
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

    g_signal_connect (settings_hangul, "changed",
                      G_CALLBACK (settings_changed), hangul);
    g_signal_connect (settings_panel, "changed",
                      G_CALLBACK (settings_changed), hangul);

    g_debug ("context new:%u", hangul->id);
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

    g_debug ("context delete:%u", hangul->id);

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

    if (hangul->preedit != NULL) {
        ustring_delete (hangul->preedit);
        hangul->preedit = NULL;
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

/**
 * @brief a function to check whether the caret has moved
 *
 * An ibus client should inform to the engine that the caret has moved
 * by calling reset method. But many implementations don't follow this rule.
 * So we need to check whether the caret pos is changed.
 * If so, we will reset the context.
 *
 * Usually we don't need this function. Only when the preedit mode is
 * PREEDIT_MODE_NONE, it is critical to have same surrounding text and
 * internal cached preedit text.
 */
static void
ibus_hangul_engine_check_caret_pos_sanity (IBusHangulEngine *hangul)
{
    size_t preedit_len = ustring_length (hangul->preedit);
    if (preedit_len == 0)
        return;

    IBusText* ibus_text = NULL;
    guint cursor_pos = 0;
    guint anchor_pos = 0;
    ibus_engine_get_surrounding_text ((IBusEngine *)hangul,
            &ibus_text, &cursor_pos, &anchor_pos);

    const gchar* text = ibus_text_get_text (ibus_text);
    if (text == NULL || cursor_pos == 0)
        return;

    const gchar* text_on_cursor = g_utf8_offset_to_pointer (text, cursor_pos - 1);
    gchar* preedit_utf8 = ustring_to_utf8 (hangul->preedit, -1);
    if (preedit_utf8 == NULL) {
        g_object_unref (ibus_text);
        return;
    }

    size_t n = strlen(preedit_utf8);
    // Ok, Just comparing text value is not perfect. But any other idea?
    if (strncmp(preedit_utf8, text_on_cursor, n) != 0) {
        // If the text_on_cursor is different from preedit cache, there's a possibility
        // that the cursor was moved by the user. Then we need to reset the context.
        hangul_ic_reset (hangul->context);
        ustring_clear (hangul->preedit);
    }
    g_free (preedit_utf8);

    g_object_unref (ibus_text);
}

static void
ibus_hangul_engine_update_preedit_mode (IBusHangulEngine *hangul)
{
    if (global_preedit_mode == PREEDIT_MODE_NONE &&
        !(hangul->caps & IBUS_CAP_SURROUNDING_TEXT)) {
        // If an instance doesn't support surrounding text, ibus-hangul will change
        // preedit mode of this instance to PREEDIT_MODE_SYLLABLE.
        // Because without surrounding text feature, users cannot see "composing text".
        // This is pretty inconvenient for korean users.
        hangul->preedit_mode = PREEDIT_MODE_SYLLABLE;
    } else {
        hangul->preedit_mode = global_preedit_mode;
    }
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

    if (hangul->preedit_mode == PREEDIT_MODE_NONE) {
        return;
    }

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
ibus_hangul_engine_process_commit_and_edit (IBusHangulEngine *hangul)
{
    // commit current commit_text + preedit_text
    const ucschar *hic_commit_text = hangul_ic_get_commit_string (hangul->context);
    const ucschar *hic_preedit_text = hangul_ic_get_preedit_string (hangul->context);

    UString *commit_text = ustring_new ();
    ustring_append_ucs4 (commit_text, hic_commit_text, -1);
    ustring_append_ucs4 (commit_text, hic_preedit_text, -1);

    // commit only when the final result is different from preedit text cache
    if (ustring_compare (commit_text, hangul->preedit) != 0) {
        IBusEngine *engine = (IBusEngine *)hangul;

        // remove composing text
        guint preedit_text_len = ustring_length (hangul->preedit);
        ibus_engine_delete_surrounding_text (engine, -preedit_text_len, preedit_text_len);

        const ucschar *s = ustring_begin (commit_text);
        IBusText *text = ibus_text_new_from_ucs4 (s);
        ibus_engine_commit_text (engine, text);
    }

    ustring_delete (commit_text);

    // update preedit_text cache
    ustring_clear (hangul->preedit);
    ustring_append_ucs4 (hangul->preedit, hic_preedit_text, -1);
}

static void
ibus_hangul_engine_process_edit_and_commit (IBusHangulEngine *hangul)
{
    IBusEngine *engine = (IBusEngine *)hangul;

    const ucschar *hic_commit_text = hangul_ic_get_commit_string (hangul->context);
    const ucschar *hic_preedit_text = hangul_ic_get_preedit_string (hangul->context);

    if (hangul->preedit_mode == PREEDIT_MODE_WORD || hangul->hanja_mode) {
        ustring_append_ucs4 (hangul->preedit, hic_commit_text, -1);

        if (hic_preedit_text == NULL || hic_preedit_text[0] == 0) {
            if (ustring_length (hangul->preedit) > 0) {
                IBusText *text;
                const ucschar* preedit_text;

                /* clear preedit text before commit */
                ibus_hangul_engine_clear_preedit_text (hangul);

                preedit_text = ustring_begin (hangul->preedit);
                text = ibus_text_new_from_ucs4 ((gunichar*)preedit_text);
                ibus_engine_commit_text (engine, text);
                ustring_clear (hangul->preedit);
            }
        }
    } else {
        if (hic_commit_text != NULL && hic_commit_text[0] != 0) {
            IBusText *text;

            /* clear preedit text before commit */
            ibus_hangul_engine_clear_preedit_text (hangul);

            text = ibus_text_new_from_ucs4 (hic_commit_text);
            ibus_engine_commit_text (engine, text);
        }
    }

    ibus_hangul_engine_update_preedit_text (hangul);
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
            if (hangul->preedit_mode == PREEDIT_MODE_NONE) {
                if (preedit_len > hic_preedit_len) {
                    guint pos = preedit_len - hic_preedit_len;
                    ustring_erase (hangul->preedit, pos, hic_preedit_len);
                } else {
                    ustring_clear (hangul->preedit);
                }
                preedit_len = ustring_length(hangul->preedit);
            } else {
                key_len -= hic_preedit_len;
            }
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
    HanjaList* list = NULL;

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

    g_debug("lookup hanja table: %s", key);
    return list;
}

static void
ibus_hangul_engine_update_hanja_list (IBusHangulEngine *hangul)
{
    gchar* hanja_key;
    gchar* preedit_utf8;
    const ucschar* hic_preedit;
    UString* preedit = NULL;
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

    if (hangul->preedit_mode != PREEDIT_MODE_NONE) {
        preedit = ustring_dup (hangul->preedit);
        ustring_append_ucs4 (preedit, hic_preedit, -1);
    }

    if (preedit != NULL && ustring_length(preedit) > 0) {
        preedit_utf8 = ustring_to_utf8 (preedit, -1);
        if (hangul->preedit_mode == PREEDIT_MODE_WORD || hangul->hanja_mode) {
            hanja_key = preedit_utf8;
            lookup_method = LOOKUP_METHOD_PREFIX;
        } else {
            gchar* substr;
            ibus_engine_get_surrounding_text ((IBusEngine *)hangul, &ibus_text,
                    &cursor_pos, &anchor_pos);

            substr = h_ibus_text_get_substring (ibus_text,
                    (glong)cursor_pos - 32, cursor_pos);

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
                    (glong)cursor_pos - 32, cursor_pos);
            lookup_method = LOOKUP_METHOD_SUFFIX;
        }
    }

    if (hanja_key != NULL) {
        hangul->hanja_list = ibus_hangul_engine_lookup_hanja_table (hanja_key,
                lookup_method);
        hangul->last_lookup_method = lookup_method;
        g_free (hanja_key);
    }

    if (preedit != NULL)
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

    if (modifiers & IBUS_RELEASE_MASK)
        return FALSE;

    // if we don't ignore shift keys, shift key will make flush the preedit 
    // string. So you cannot input shift+key.
    // Let's think about these examples:
    //   dlTek (2 set)
    //   qhRdmaqkq (2 set)
    if (keyval == IBUS_Shift_L || keyval == IBUS_Shift_R)
        return FALSE;

    // On password mode, we ignore hotkeys
    if (hangul->input_purpose == IBUS_INPUT_PURPOSE_PASSWORD)
        return IBUS_ENGINE_CLASS (parent_class)->process_key_event (engine, keyval, keycode, modifiers);

    /* Process candidate key event before hot keys,
     * or lookup table can't receive important events.
     * For example, if Esc key is pressed, this key event should be used for
     * closing lookup table, not for turning to latin mode. */
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
        ibus_hangul_engine_switch_input_mode (hangul);
        return TRUE;
    }

    if (hotkey_list_match (&on_keys, keyval, modifiers)) {
        ibus_hangul_engine_set_input_mode (hangul, INPUT_MODE_HANGUL);
        return FALSE;
    }

    if (hangul->input_mode == INPUT_MODE_LATIN)
        return IBUS_ENGINE_CLASS (parent_class)->process_key_event (engine, keyval, keycode, modifiers);

    /* This feature is for vi* users.
     * On Esc, the input mode is changed to latin */
    if (hotkey_list_match (&off_keys, keyval, modifiers)) {
        ibus_hangul_engine_set_input_mode (hangul, INPUT_MODE_LATIN);
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

    if (hangul->preedit_mode == PREEDIT_MODE_NONE) {
        ibus_hangul_engine_check_caret_pos_sanity (hangul);
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

        if (hangul->preedit_mode == PREEDIT_MODE_NONE) {
            ibus_hangul_engine_process_commit_and_edit (hangul);
        } else {
            ibus_hangul_engine_update_preedit_text (hangul);
        }

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

        if (hangul->preedit_mode == PREEDIT_MODE_NONE) {
            ibus_hangul_engine_process_commit_and_edit (hangul);
        } else {
            ibus_hangul_engine_process_edit_and_commit (hangul);
        }

        if (hangul->hanja_mode) {
            ibus_hangul_engine_update_lookup_table (hangul);
        }

        if (!retval)
            ibus_hangul_engine_flush (hangul);
    }

    /* We always return TRUE here even if we didn't use this event.
     * Instead, we forward the event to clients.
     *
     * Because IBus has a problem with sync mode.
     * I think it's limitations of IBus implementation.
     * We call several engine functions(updating preedit text and committing
     * text) inside this function.
     * But clients cannot receive the results of other calls until this
     * function ends. Clients only get one result from a remote call at a time
     * because clients may run on event loop.
     * Clients may process this event first and then get the results which
     * may change the preedit text or commit text.
     * So the event order is broken.
     * Call order:
     *      engine                          client
     *                                      call process_key_event
     *      begin process_key_event
     *        call commit_text
     *        call update_preedit_text
     *      return the event as unused
     *                                      receive the result of process_key_event
     *                                      receive the result of commit_text
     *                                      receive the result of update_preedit_text
     *
     * To solve this problem, we return TRUE as if we consumed this event.
     * After that, we forward this event to clients.
     * Then clients may get the events in correct order.
     * This approach is a kind of async processing.
     * Call order:
     *      engine                          client
     *                                      call process_key_event
     *      begin process_key_event
     *        call commit_text
     *        call update_preedit_text
     *        call forward_key_event
     *      return the event as used
     *                                      receive the result of process_key_event
     *                                      receive the result of commit_text
     *                                      receive the result of update_preedit_text
     *                                      receive the forwarded key event
     *
     * See: https://github.com/choehwanjin/ibus-hangul/issues/40
     */
    if (use_event_forwarding) {
        if (!retval) {
            ibus_engine_forward_key_event (engine, keyval, keycode, modifiers);
        }

        return TRUE;
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

        g_debug ("flush:%u: %s", hangul->id, text->text);
	ibus_engine_commit_text ((IBusEngine *) hangul, text);

	ustring_clear(hangul->preedit);
    }

    ibus_hangul_engine_update_preedit_text (hangul);
}

static void
ibus_hangul_engine_focus_in (IBusEngine *engine)
{
    IBusHangulEngine *hangul = (IBusHangulEngine *) engine;

    //g_debug ("focus_in: %u", hangul->id);

    ibus_hangul_engine_update_preedit_mode (hangul);

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

    IBUS_ENGINE_CLASS (parent_class)->focus_in (engine);
}

static void
ibus_hangul_engine_focus_out (IBusEngine *engine)
{
    IBusHangulEngine *hangul = (IBusHangulEngine *) engine;

    //g_debug ("focus_out: %u", hangul->id);

    if (hangul->hanja_list == NULL) {
	// ibus-hangul uses
	// ibus_engine_update_preedit_text_with_mode() function which makes
	// the preedit string committed automatically when the focus is out.
	// So we don't need to commit the preedit here.
	hangul_ic_reset (hangul->context);
        ustring_clear (hangul->preedit);
    } else {
        ibus_engine_hide_lookup_table (engine);
        ibus_engine_hide_auxiliary_text (engine);
    }

    IBUS_ENGINE_CLASS (parent_class)->focus_out ((IBusEngine *) hangul);
}

static void
ibus_hangul_engine_reset (IBusEngine *engine)
{
    IBusHangulEngine *hangul = (IBusHangulEngine *) engine;

    g_debug ("reset:%u", hangul->id);

    if (hangul->preedit_mode == PREEDIT_MODE_NONE) {
        hangul_ic_reset (hangul->context);
        ustring_clear (hangul->preedit);
    }

    if (use_client_commit) {
        // ibus-hangul uses
        // ibus_engine_update_preedit_text_with_mode() function which makes
        // the preedit string committed automatically when the reset is received
        // So we don't need to commit the preedit here.
        hangul_ic_reset (hangul->context);
        ustring_clear (hangul->preedit);
    }

    ibus_hangul_engine_flush (hangul);

    IBUS_ENGINE_CLASS (parent_class)->reset (engine);
}

static void
ibus_hangul_engine_enable (IBusEngine *engine)
{
    IBUS_ENGINE_CLASS (parent_class)->enable (engine);

    g_debug ("enable:%u", ((IBusHangulEngine*) engine)->id);

    ibus_engine_get_surrounding_text (engine, NULL, NULL, NULL);
}

static void
ibus_hangul_engine_disable (IBusEngine *engine)
{
    g_debug ("disable:%u", ((IBusHangulEngine*) engine)->id);

    ibus_hangul_engine_focus_out (engine);
    IBUS_ENGINE_CLASS (parent_class)->disable (engine);
}

static void
ibus_hangul_engine_set_capabilities (IBusEngine *engine, guint caps)
{
    IBusHangulEngine *hangul = (IBusHangulEngine *) engine;

    hangul->caps = caps;

    ibus_hangul_engine_update_preedit_mode (hangul);

    g_debug ("set_capabilities:%u: %x", hangul->id, caps);
}

static void
ibus_hangul_engine_page_up (IBusEngine *engine)
{
    IBUS_ENGINE_CLASS (parent_class)->page_up (engine);
}

static void
ibus_hangul_engine_page_down (IBusEngine *engine)
{
    IBUS_ENGINE_CLASS (parent_class)->page_down (engine);
}

static void
ibus_hangul_engine_cursor_up (IBusEngine *engine)
{
    IBusHangulEngine *hangul = (IBusHangulEngine *) engine;

    if (hangul->hanja_list != NULL) {
        ibus_lookup_table_cursor_up (hangul->table);
        ibus_hangul_engine_update_lookup_table_ui (hangul);
    }

    IBUS_ENGINE_CLASS (parent_class)->cursor_up (engine);
}

static void
ibus_hangul_engine_cursor_down (IBusEngine *engine)
{
    IBusHangulEngine *hangul = (IBusHangulEngine *) engine;

    if (hangul->hanja_list != NULL) {
        ibus_lookup_table_cursor_down (hangul->table);
        ibus_hangul_engine_update_lookup_table_ui (hangul);
    }

    IBUS_ENGINE_CLASS (parent_class)->cursor_down (engine);
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

        ibus_hangul_engine_switch_input_mode (hangul);
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
ibus_hangul_engine_switch_input_mode (IBusHangulEngine *hangul)
{
    int input_mode = hangul->input_mode + 1;

    if (input_mode >= INPUT_MODE_COUNT) {
        input_mode = INPUT_MODE_HANGUL;
    }

    ibus_hangul_engine_set_input_mode (hangul, input_mode);
}

static IBusText *
ibus_hangul_engine_get_input_mode_symbol (IBusHangulEngine *hangul,
                                          int input_mode)
{
    IBusText **symbols = hangul->input_mode_symbols;

    if (symbols[0] == NULL) {
        symbols[INPUT_MODE_HANGUL] = ibus_text_new_from_string ("한");
        g_object_ref_sink(symbols[INPUT_MODE_HANGUL]);
        symbols[INPUT_MODE_LATIN] = ibus_text_new_from_string ("EN");
        g_object_ref_sink(symbols[INPUT_MODE_LATIN]);
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

    if (disable_latin_mode) {
        return;
    }

    prop = hangul->prop_hangul_mode;

    hangul->input_mode = input_mode;
    g_debug("input_mode:%u: %s", hangul->id,
            (input_mode == INPUT_MODE_HANGUL) ? "hangul" : "latin");

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
print_changed_settings (const gchar *schema_id, const gchar *key, GVariant *value)
{
    gchar *variant_printable = g_variant_print (value, FALSE);
    g_debug ("settings_changed: %s/%s: %s", schema_id, key, variant_printable);
    g_free (variant_printable);
}

static void
settings_changed (GSettings    *settings,
                  const gchar  *key,
                  gpointer      user_data)
{
    IBusHangulEngine *hangul = (IBusHangulEngine *) user_data;
    GValue schema_value = G_VALUE_INIT;
    const gchar *schema_id;
    GVariant *value;

    g_return_if_fail (G_IS_SETTINGS (settings));

    g_value_init (&schema_value, G_TYPE_STRING);
    g_object_get_property (G_OBJECT (settings), "schema-id", &schema_value);
    schema_id = g_value_get_string (&schema_value);
    value = g_settings_get_value (settings, key);
    if (strcmp (schema_id, "org.freedesktop.ibus.engine.hangul") == 0) {
        if (strcmp(key, "hangul-keyboard") == 0) {
            const gchar *str = g_variant_get_string(value, NULL);
            g_string_assign (hangul_keyboard, str);
            hangul_ic_select_keyboard (hangul->context, hangul_keyboard->str);
            print_changed_settings (schema_id, key, value);
        } else if (strcmp (key, "hanja-keys") == 0) {
            const gchar* str = g_variant_get_string(value, NULL);
	    hotkey_list_set_from_string(&hanja_keys, str);
            print_changed_settings (schema_id, key, value);
        } else if (strcmp (key, "word-commit") == 0) {
            word_commit = g_variant_get_boolean (value);
            print_changed_settings (schema_id, key, value);
        } else if (strcmp (key, "auto-reorder") == 0) {
            auto_reorder = g_variant_get_boolean (value);
            print_changed_settings (schema_id, key, value);
        } else if (strcmp (key, "switch-keys") == 0) {
            const gchar* str = g_variant_get_string(value, NULL);
	    hotkey_list_set_from_string(&switch_keys, str);
            print_changed_settings (schema_id, key, value);
        } else if (strcmp (key, "on-keys") == 0) {
            const gchar* str = g_variant_get_string(value, NULL);
	    hotkey_list_set_from_string(&on_keys, str);
            print_changed_settings (schema_id, key, value);
        } else if (strcmp (key, "off-keys") == 0) {
            const gchar* str = g_variant_get_string(value, NULL);
	    hotkey_list_set_from_string(&off_keys, str);
            print_changed_settings (schema_id, key, value);
        } else if (strcmp (key, "initial-input-mode") == 0) {
            const gchar* str = g_variant_get_string (value, NULL);
            if (strcmp(str, "latin") == 0) {
                initial_input_mode = INPUT_MODE_LATIN;
            } else if (strcmp(str, "hangul") == 0) {
                initial_input_mode = INPUT_MODE_HANGUL;
            }
            print_changed_settings (schema_id, key, value);
        } else if (strcmp (key, "use-event-forwarding") == 0) {
            use_event_forwarding = g_variant_get_boolean (value);
            print_changed_settings (schema_id, key, value);
        } else if (strcmp (key, "preedit-mode") == 0) {
            const gchar* str = g_variant_get_string (value, NULL);
            if (strcmp(str, "none") == 0) {
                global_preedit_mode = PREEDIT_MODE_NONE;
            } else if (strcmp(str, "word") == 0) {
                global_preedit_mode = PREEDIT_MODE_WORD;
            } else {
                global_preedit_mode = PREEDIT_MODE_SYLLABLE;
            }
            print_changed_settings (schema_id, key, value);
        }
    } else if (strcmp (schema_id, "org.freedesktop.ibus.panel") == 0) {
        if (strcmp (key, "lookup-table-orientation") == 0) {
            lookup_table_orientation = g_variant_get_int32(value);
            print_changed_settings (schema_id, key, value);
        }
    }
    g_variant_unref (value);
    g_value_unset (&schema_value);
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
ibus_hangul_engine_set_content_type (IBusEngine     *engine,
                                     guint           purpose,
                                     guint           hints)
{
    IBusHangulEngine *hangul = (IBusHangulEngine *) engine;
    if (hangul == NULL)
        return;

    hangul->input_purpose = purpose;
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
