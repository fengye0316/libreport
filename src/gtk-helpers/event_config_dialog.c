/*
    Copyright (C) 2011  ABRT Team
    Copyright (C) 2011  RedHat inc.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/
#include <gtk/gtk.h>
#include "internal_libreport_gtk.h"

static GtkWindow *g_event_list_window;
static GList *option_widget_list;
static bool has_password_option;

enum
{
    COLUMN_EVENT_UINAME,
    COLUMN_EVENT_NAME,
    NUM_COLUMNS
};

typedef struct
{
    event_option_t *option;
    GtkWidget *widget;
} option_widget_t;

int show_event_config_dialog(const char *event_name, GtkWindow *parent);

static GtkWidget *gtk_label_new_justify_left(const gchar *label_str)
{
    GtkWidget *label = gtk_label_new(label_str);
    gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);
    gtk_misc_set_alignment(GTK_MISC(label), /*xalign:*/ 0, /*yalign:*/ 0.5);
    /* Make some space between label and input field to the right of it: */
    gtk_misc_set_padding(GTK_MISC(label), /*xpad:*/ 5, /*ypad:*/ 0);
    return label;
}

static void add_option_widget(GtkWidget *widget, event_option_t *option)
{
    option_widget_t *ow = (option_widget_t *)xmalloc(sizeof(option_widget_t));
    ow->widget = widget;
    ow->option = option;
    option_widget_list = g_list_prepend(option_widget_list, ow);
}

static void on_show_pass_cb(GtkToggleButton *tb, gpointer user_data)
{
    GtkEntry *entry = (GtkEntry *)user_data;
    gtk_entry_set_visibility(entry, gtk_toggle_button_get_active(tb));
}

static void on_show_pass_store_cb(GtkToggleButton *tb, gpointer user_data)
{
    set_user_setting("store_passwords", gtk_toggle_button_get_active(tb) ? "no" : "yes");
}

static unsigned add_one_row_to_grid(GtkGrid *table)
{
    gulong rows = (gulong)g_object_get_data(G_OBJECT(table), "n-rows");
    gtk_grid_insert_row(table, rows);
    g_object_set_data(G_OBJECT(table), "n-rows", (gpointer)(rows + 1));
    return rows;
}

static void add_option_to_table(gpointer data, gpointer user_data)
{
    event_option_t *option = data;
    GtkGrid *option_table = user_data;
    if (option->is_advanced)
        option_table = GTK_GRID(g_object_get_data(G_OBJECT(option_table), "advanced-options"));

    GtkWidget *label;
    GtkWidget *option_input;
    unsigned last_row;

    char *option_label;
    if (option->eo_label != NULL)
        option_label = xstrdup(option->eo_label);
    else
    {
        option_label = xstrdup(option->eo_name ? option->eo_name : "");
        /* Replace '_' with ' ' */
        char *p = option_label - 1;
        while (*++p)
            if (*p == '_')
                *p = ' ';
    }

    switch (option->eo_type)
    {
        case OPTION_TYPE_TEXT:
        case OPTION_TYPE_NUMBER:
        case OPTION_TYPE_PASSWORD:
            last_row = add_one_row_to_grid(option_table);
            label = gtk_label_new_justify_left(option_label);
            gtk_grid_attach(option_table, label,
                             /*left,top:*/ 0, last_row,
                             /*width,height:*/ 1, 1);
            option_input = gtk_entry_new();
            gtk_widget_set_hexpand(option_input, TRUE);
            if (option->eo_value != NULL)
                gtk_entry_set_text(GTK_ENTRY(option_input), option->eo_value);
            gtk_grid_attach(option_table, option_input,
                             /*left,top:*/ 1, last_row,
                             /*width,height:*/ 1, 1);
            add_option_widget(option_input, option);
            if (option->eo_type == OPTION_TYPE_PASSWORD)
            {
                gtk_entry_set_visibility(GTK_ENTRY(option_input), 0);
                last_row = add_one_row_to_grid(option_table);
                GtkWidget *pass_cb = gtk_check_button_new_with_label(_("Show password"));
                gtk_grid_attach(option_table, pass_cb,
                             /*left,top:*/ 1, last_row,
                             /*width,height:*/ 1, 1);
                g_signal_connect(pass_cb, "toggled", G_CALLBACK(on_show_pass_cb), option_input);
                has_password_option = true;
            }
            break;

        case OPTION_TYPE_HINT_HTML:
            label = gtk_label_new(option_label);
            gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
            gtk_misc_set_alignment(GTK_MISC(label), /*x,yalign:*/ 0.0, 0.0);
            make_label_autowrap_on_resize(GTK_LABEL(label));

            last_row = add_one_row_to_grid(option_table);
            gtk_grid_attach(option_table, label,
                             /*left,top:*/ 0, last_row,
                             /*width,height:*/ 2, 1);
            break;

        case OPTION_TYPE_BOOL:
            last_row = add_one_row_to_grid(option_table);
            option_input = gtk_check_button_new_with_label(option_label);
            gtk_grid_attach(option_table, option_input,
                             /*left,top:*/ 0, last_row,
                             /*width,height:*/ 2, 1);
            if (option->eo_value != NULL)
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(option_input),
                                    string_to_bool(option->eo_value));
            add_option_widget(option_input, option);
            break;

        default:
            //option_input = gtk_label_new_justify_left("WTF?");
            log("unsupported option type");
            free(option_label);
            return;
    }

    if (option->eo_note_html)
    {
        label = gtk_label_new(option->eo_note_html);
        gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
        gtk_misc_set_alignment(GTK_MISC(label), /*x,yalign:*/ 0.0, 0.0);
        make_label_autowrap_on_resize(GTK_LABEL(label));

        last_row = add_one_row_to_grid(option_table);
        gtk_grid_attach(option_table, label,
                             /*left,top:*/ 1, last_row,
                             /*top,heigh:*/ 1, 1);
    }

    free(option_label);
}

static void on_close_event_list_cb(GtkWidget *button, gpointer user_data)
{
    GtkWidget *window = (GtkWidget *)user_data;
    gtk_widget_destroy(window);
}

static char *get_event_name_from_row(GtkTreeView *treeview)
{
    GtkTreeSelection *selection = gtk_tree_view_get_selection(treeview);
    char *event_name = NULL;
    if (selection)
    {
        GtkTreeIter iter;
        GtkTreeModel *store = gtk_tree_view_get_model(treeview);
        if (gtk_tree_selection_get_selected(selection, &store, &iter) == TRUE)
        {
            GValue value = { 0 };
            gtk_tree_model_get_value(store, &iter, COLUMN_EVENT_NAME, &value);
            event_name = (char *)g_value_get_string(&value);
        }
    }
    return event_name;
}

static void on_configure_event_cb(GtkWidget *button, gpointer user_data)
{
    GtkTreeView *events_tv = (GtkTreeView *)user_data;
    char *event_name = get_event_name_from_row(events_tv);
    if (event_name != NULL)
        show_event_config_dialog(event_name, NULL);
    //else
    //    error_msg(_("Please select a plugin from the list to edit its options."));
}

static void on_event_row_activated_cb(GtkTreeView *treeview, GtkTreePath *path, GtkTreeViewColumn *column, gpointer user_data)
{
    char *event_name = get_event_name_from_row(treeview);
    event_config_t *ec = get_event_config(event_name);
    if (ec->options != NULL) //We need to have some options to show
        show_event_config_dialog(event_name, NULL);
}

static void on_event_row_changed_cb(GtkTreeView *treeview, gpointer user_data)
{
    const char *event_name = get_event_name_from_row(treeview);

    if (event_name)
    {
        event_config_t *ec = get_event_config(event_name);
        gtk_widget_set_sensitive(GTK_WIDGET(user_data), ec->options != NULL);
    }
}

static void add_event_to_liststore(gpointer key, gpointer value, gpointer user_data)
{
    GtkListStore *events_list_store = (GtkListStore *)user_data;
    event_config_t *ec = (event_config_t *)value;

    char *event_label;
    if (ec->screen_name != NULL && ec->description != NULL)
        event_label = xasprintf("<b>%s</b>\n%s", ec->screen_name, ec->description);
    else
        //if event has no xml description
        event_label = xasprintf("<b>%s</b>\nNo description available", key);

    GtkTreeIter iter;
    gtk_list_store_append(events_list_store, &iter);
    gtk_list_store_set(events_list_store, &iter,
                      COLUMN_EVENT_UINAME, event_label,
                      COLUMN_EVENT_NAME, key,
                      -1);
    free(event_label);
}

static void save_value_from_widget(gpointer data, gpointer user_data)
{
    option_widget_t *ow = (option_widget_t *)data;

    const char *val = NULL;
    switch (ow->option->eo_type)
    {
        case OPTION_TYPE_TEXT:
        case OPTION_TYPE_NUMBER:
        case OPTION_TYPE_PASSWORD:
            val = (char *)gtk_entry_get_text(GTK_ENTRY(ow->widget));
            break;
        case OPTION_TYPE_BOOL:
            val = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ow->widget)) ? "yes" : "no";
            break;
        default:
            log("unsupported option type");
    }
    if (val)
    {
        free(ow->option->eo_value);
        ow->option->eo_value = xstrdup(val);
        VERB1 log("saved: %s:%s", ow->option->eo_name, ow->option->eo_value);
    }
}

static void dehydrate_config_dialog()
{
    if (option_widget_list != NULL)
        g_list_foreach(option_widget_list, &save_value_from_widget, NULL);
}

/*
GtkWidget *create_event_config_dialog(event_name)
{

    return
}
*/

int show_event_config_dialog(const char *event_name, GtkWindow *parent)
{
    if (option_widget_list != NULL)
    {
        g_list_free(option_widget_list);
        option_widget_list = NULL;
    }

    event_config_t *event = get_event_config(event_name);

    GtkWindow *parent_window = parent ? parent : g_event_list_window;

    GtkWidget *dialog = gtk_dialog_new_with_buttons(
                        /*title:*/ event->screen_name ? event->screen_name : event_name,
                        parent_window,
                        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                        GTK_STOCK_CANCEL,
                        GTK_RESPONSE_CANCEL,
                        GTK_STOCK_OK,
                        GTK_RESPONSE_APPLY,
                        NULL);

    /* Allow resize?
     * W/o resize, e.g. upload configuration hint looks awfully
     * line wrapped.
     * With resize, there are some somewhat not nice effects:
     * for one, opening an expander will enlarge window,
     * but won't contract it back when expander is closed.
     */
    gtk_window_set_resizable(GTK_WINDOW(dialog), true);
    gtk_window_set_default_size(GTK_WINDOW(dialog), 450, -1);

    if (parent_window != NULL)
    {
        gtk_window_set_icon_name(GTK_WINDOW(dialog),
                gtk_window_get_icon_name(parent_window));
    }

    GtkWidget *option_table = gtk_grid_new();
    gtk_grid_set_row_homogeneous(GTK_GRID(option_table), FALSE);
    gtk_grid_set_column_homogeneous(GTK_GRID(option_table), FALSE);
    gtk_grid_set_row_spacing(GTK_GRID(option_table), 2);
    g_object_set_data(G_OBJECT(option_table), "n-rows", (gpointer)-1);

    /* table to hold advanced options
     * hidden in expander which is visible only if there's at least
     * one advanced option
    */

    GtkWidget *adv_option_table = gtk_grid_new();
    gtk_grid_set_row_homogeneous(GTK_GRID(adv_option_table), FALSE);
    gtk_grid_set_column_homogeneous(GTK_GRID(adv_option_table), FALSE);
    gtk_grid_set_row_spacing(GTK_GRID(adv_option_table), 2);
    g_object_set_data(G_OBJECT(adv_option_table), "n-rows", (gpointer)-1);

    GtkWidget *adv_expander = gtk_expander_new(_("Advanced"));
    gtk_container_add(GTK_CONTAINER(adv_expander), adv_option_table);
    g_object_set_data(G_OBJECT(option_table), "advanced-options", adv_option_table);

    has_password_option = false;
    g_list_foreach(event->options, &add_option_to_table, option_table);

    /* if there is at least one password option, add checkbox to disable storing passwords */
    if (has_password_option)
    {
        unsigned last_row = add_one_row_to_grid(GTK_GRID(option_table));
        GtkWidget *pass_store_cb = gtk_check_button_new_with_label(_("Don't store passwords"));
        gtk_grid_attach(GTK_GRID(option_table), pass_store_cb,
                /*left,top:*/ 0, last_row,
                /*width,height:*/ 1, 1);
        const char *store_passwords = get_user_setting("store_passwords");
        if (store_passwords && !strcmp(store_passwords, "no"))
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pass_store_cb), 1);
        g_signal_connect(pass_store_cb, "toggled", G_CALLBACK(on_show_pass_store_cb), NULL);
    }

    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_box_pack_start(GTK_BOX(content), option_table, false, false, 20);

    /* add the adv_option_table to the dialog only if there is some adv option */
    if (g_list_length(gtk_container_get_children(GTK_CONTAINER(adv_option_table))) > 0)
        gtk_box_pack_start(GTK_BOX(content), adv_expander, false, false, 0);

    /* add warning if secrets service is not available showing the nagging dialog
     * is considered "too heavy UI" be designers
     */
    if (!is_event_config_user_storage_available())
    {
        GtkWidget *keyring_warn_lbl =
        gtk_label_new(
          _("Secret Service is not available, your settings won't be saved!"));
        static const GdkColor red = { .red = 0xffff };
        gtk_widget_modify_fg(keyring_warn_lbl, GTK_STATE_NORMAL, &red);
        gtk_box_pack_start(GTK_BOX(content), keyring_warn_lbl, false, false, 0);
    }

    gtk_widget_show_all(content);

    int result = gtk_dialog_run(GTK_DIALOG(dialog));
    if (result == GTK_RESPONSE_APPLY)
    {
        dehydrate_config_dialog();
        const char *const store_passwords_s = get_user_setting("store_passwords");
        save_event_config_data_to_user_storage(event_name,
                                               get_event_config(event_name),
                                               !(store_passwords_s && !strcmp(store_passwords_s, "no")));
    }
    //else if (result == GTK_RESPONSE_CANCEL)
    //    log("log");
    gtk_widget_destroy(dialog);
    return result;
}

GtkWidget *create_event_list_config_dialog()
{
    GtkWidget *main_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    GtkWidget *events_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(events_scroll),
                                    GTK_POLICY_NEVER,
                                    GTK_POLICY_AUTOMATIC);
    /* event list treeview */
    GtkWidget *events_tv = gtk_tree_view_new();
    /* column with event name and description */
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;

    /* add column to tree view */
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(_("Event"),
                                                 renderer,
                                                 "markup",
                                                 COLUMN_EVENT_UINAME,
                                                 NULL);
    gtk_tree_view_column_set_resizable(column, TRUE);
    g_object_set(G_OBJECT(renderer), "wrap-mode", PANGO_WRAP_WORD, NULL);
    g_object_set(G_OBJECT(renderer), "wrap-width", 440, NULL);
    gtk_tree_view_column_set_sort_column_id(column, COLUMN_EVENT_NAME);
    gtk_tree_view_append_column(GTK_TREE_VIEW(events_tv), column);
    /* "Please draw rows in alternating colors": */
    gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(events_tv), TRUE);
    // TODO: gtk_tree_view_set_headers_visible(FALSE)? We have only one column anyway...

    /* Create data store for the list and attach it
     * COLUMN_EVENT_UINAME -> name+description
     * COLUMN_EVENT_NAME -> event name so we can retrieve it from the row
     */
    GtkListStore *events_list_store = gtk_list_store_new(NUM_COLUMNS,
                                                G_TYPE_STRING, /* Event name + description */
                                                G_TYPE_STRING  /* event name */
    );
    gtk_tree_view_set_model(GTK_TREE_VIEW(events_tv), GTK_TREE_MODEL(events_list_store));

    g_hash_table_foreach(g_event_config_list,
                        &add_event_to_liststore,
                        events_list_store);
//TODO: can unref events_list_store? treeview holds one ref.

    /* Double click/Enter handler */
    g_signal_connect(events_tv, "row-activated", G_CALLBACK(on_event_row_activated_cb), NULL);

    gtk_container_add(GTK_CONTAINER(events_scroll), events_tv);

    GtkWidget *configure_event_btn = gtk_button_new_with_mnemonic(_("Configure E_vent"));
    gtk_widget_set_sensitive(configure_event_btn, false);
    g_signal_connect(configure_event_btn, "clicked", G_CALLBACK(on_configure_event_cb), events_tv);
    g_signal_connect(events_tv, "cursor-changed", G_CALLBACK(on_event_row_changed_cb), configure_event_btn);

    GtkWidget *close_btn = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
    g_signal_connect(close_btn, "clicked", G_CALLBACK(on_close_event_list_cb), g_event_list_window);

    GtkWidget *btnbox = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_end(GTK_BOX(btnbox), close_btn, false, false, 0);
    gtk_box_pack_end(GTK_BOX(btnbox), configure_event_btn, false, false, 0);

    gtk_box_pack_start(GTK_BOX(main_vbox), events_scroll, true, true, 10);
    gtk_box_pack_start(GTK_BOX(main_vbox), btnbox, false, false, 0);

    return main_vbox;
}

void show_events_list_dialog(GtkWindow *parent)
{
    /*remove this line if we want to reload the config
     *everytime we show the config dialog
     */
    if (g_event_config_list == NULL)
    {
        load_event_config_data();
        load_event_config_data_from_user_storage(g_event_config_list);
    }

    GtkWidget *event_list_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    g_event_list_window = (GtkWindow*)event_list_window;
    gtk_window_set_title(g_event_list_window, _("Event Configuration"));
    gtk_window_set_default_size(g_event_list_window, 450, 400);
    gtk_window_set_position(g_event_list_window, parent ? GTK_WIN_POS_CENTER_ON_PARENT : GTK_WIN_POS_CENTER);
    if (parent != NULL)
    {
        gtk_window_set_transient_for(g_event_list_window, parent);
        // modal = parent window can't steal focus
        gtk_window_set_modal(g_event_list_window, true);
        gtk_window_set_icon_name(g_event_list_window,
            gtk_window_get_icon_name(parent));
    }

    GtkWidget *main_vbox = create_event_list_config_dialog();

    gtk_container_add(GTK_CONTAINER(event_list_window), main_vbox);

    gtk_widget_show_all(event_list_window);
}
