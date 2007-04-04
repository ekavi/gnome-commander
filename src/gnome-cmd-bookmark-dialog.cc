/*
    GNOME Commander - A GNOME based file manager
    Copyright (C) 2001-2006 Marcus Bjurman

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#include <config.h>
#include <sys/types.h>
#include <regex.h>
#include <unistd.h>
#include <errno.h>
#include "gnome-cmd-includes.h"
#include "gnome-cmd-bookmark-dialog.h"
#include "gnome-cmd-main-win.h"
#include "gnome-cmd-file-selector.h"
#include "gnome-cmd-file.h"
#include "gnome-cmd-clist.h"
#include "gnome-cmd-dir.h"
#include "gnome-cmd-data.h"
#include "gnome-cmd-combo.h"
#include "widget-factory.h"

using namespace std;


struct _GnomeCmdBookmarkDialogPrivate
{
    GList *groups;
    GnomeCmdBookmark *sel_bookmark;
    GnomeCmdBookmarkGroup *sel_group;

    GtkWidget *combo;
    GtkWidget *dir_list;
    GtkWidget *remove_btn;
    GtkWidget *edit_btn;
    GtkWidget *goto_btn;
    GtkWidget *move_up_btn;
    GtkWidget *move_down_btn;
};


static GnomeCmdDialogClass *parent_class = NULL;

guint bookmark_dialog_default_column_width[BOOKMARK_DIALOG_NUM_COLUMNS] = {
    16,
    100,
    300,
};


static void
show_bookmark_dialog (const gchar *name, const gchar *path,
                      const gchar *title,
                      GnomeCmdStringDialogCallback on_ok,
                      GnomeCmdBookmarkDialog *dialog)
{
    const gchar *labels[] = {
        _("Bookmark name:"),
        _("Bookmark target:"),
    };

    GtkWidget *dlg = gnome_cmd_string_dialog_new (title, labels, 2, (GnomeCmdStringDialogCallback)on_ok, dialog);

    gnome_cmd_string_dialog_set_value (GNOME_CMD_STRING_DIALOG (dlg), 0, name);
    gnome_cmd_string_dialog_set_value (GNOME_CMD_STRING_DIALOG (dlg), 1, path);

    gtk_widget_show (dlg);
}


static void
do_add_bookmark (GnomeCmdBookmarkDialog *dialog, GnomeCmdBookmark *bookmark)
{
    gint row;
    gchar *text[4];

    g_return_if_fail (bookmark != NULL);

    text[0] = NULL;
    text[1] = bookmark->name;
    text[2] = bookmark->path;
    text[3] = NULL;

    row = gtk_clist_append (GTK_CLIST (dialog->priv->dir_list), text);
    gtk_clist_set_row_data (GTK_CLIST (dialog->priv->dir_list), row, bookmark);

    gtk_clist_set_pixmap (GTK_CLIST (dialog->priv->dir_list), row, 0,
                          IMAGE_get_pixmap (PIXMAP_BOOKMARK),
                          IMAGE_get_mask (PIXMAP_BOOKMARK));
}


static void
set_selected_group (GnomeCmdBookmarkDialog *dialog, GnomeCmdBookmarkGroup *group)
{
    g_return_if_fail (group != NULL);

    dialog->priv->sel_group = group;

    gtk_entry_set_text (GTK_ENTRY (GNOME_CMD_COMBO (dialog->priv->combo)->entry), gnome_cmd_con_get_alias (group->con));

    gtk_clist_clear (GTK_CLIST (dialog->priv->dir_list));

    for (GList *bookmarks = group->bookmarks; bookmarks; bookmarks = bookmarks->next)
    {
        GnomeCmdBookmark *bookmark = (GnomeCmdBookmark *) bookmarks->data;
        do_add_bookmark (dialog, bookmark);
    }
}


static void on_dir_goto (GtkButton *button, GnomeCmdBookmarkDialog *dialog)
{
    GtkCList *dir_list = GTK_CLIST (dialog->priv->dir_list);
    GnomeCmdBookmark *bookmark = (GnomeCmdBookmark *) gtk_clist_get_row_data (dir_list, dir_list->focus_row);

    gnome_cmd_bookmark_goto (bookmark);

    gtk_widget_destroy (GTK_WIDGET (dialog));
}


static void on_dir_remove (GtkButton *button, GnomeCmdBookmarkDialog *dialog)
{
    GtkCList *dir_list = GTK_CLIST (dialog->priv->dir_list);
    GnomeCmdBookmark *bookmark = (GnomeCmdBookmark *) gtk_clist_get_row_data (dir_list, dir_list->focus_row);
    gtk_clist_remove (dir_list, dir_list->focus_row);

    bookmark->group->bookmarks = g_list_remove (bookmark->group->bookmarks, bookmark);
    g_free (bookmark->name);
    g_free (bookmark->path);
    g_free (bookmark);
}


static gboolean on_edit_ok (GnomeCmdStringDialog *string_dialog, const gchar **values, GnomeCmdBookmarkDialog *dialog)
{
    const gchar *name = values[0];
    const gchar *path = values[1];
    GtkCList *dir_list = GTK_CLIST (dialog->priv->dir_list);

    g_return_val_if_fail (dialog->priv->sel_bookmark != NULL, TRUE);

    if (!name)
    {
        gnome_cmd_string_dialog_set_error_desc (string_dialog, g_strdup (_("Bookmark name is missing")));
        return FALSE;
    }

    if (!path)
    {
        gnome_cmd_string_dialog_set_error_desc (string_dialog, g_strdup (_("Bookmark target is missing")));
        return FALSE;
    }

    g_free (dialog->priv->sel_bookmark->name);
    dialog->priv->sel_bookmark->name = g_strdup (name);
    g_free (dialog->priv->sel_bookmark->path);
    dialog->priv->sel_bookmark->path = g_strdup (path);

    gtk_clist_set_text (dir_list, dir_list->focus_row, 1, name);
    gtk_clist_set_text (dir_list, dir_list->focus_row, 2, path);

    return TRUE;
}


static void on_dir_edit (GtkButton *button, GnomeCmdBookmarkDialog *dialog)
{
    show_bookmark_dialog (dialog->priv->sel_bookmark->name,
                          dialog->priv->sel_bookmark->path,
                          _("Edit Bookmark"),
                          (GnomeCmdStringDialogCallback)on_edit_ok,
                          dialog);
}


static void update_move_buttons (GnomeCmdBookmarkDialog *dialog, int row)
{
    GtkCList *dir_list = GTK_CLIST (dialog->priv->dir_list);

    if (row == 0)
    {
        gtk_widget_set_sensitive (dialog->priv->move_up_btn, FALSE);
        gtk_widget_set_sensitive (dialog->priv->move_down_btn, dir_list->rows > 1);
    }
    else
        if (row == dir_list->rows - 1)
        {
            gtk_widget_set_sensitive (dialog->priv->move_down_btn, FALSE);
            gtk_widget_set_sensitive (dialog->priv->move_up_btn, dir_list->rows > 1);
        }
        else
        {
            gtk_widget_set_sensitive (dialog->priv->move_up_btn, TRUE);
            gtk_widget_set_sensitive (dialog->priv->move_down_btn, TRUE);
        }
}


static void on_dir_move_up (GtkButton *button, GnomeCmdBookmarkDialog *dialog)
{
    GtkCList *dir_list = GTK_CLIST (dialog->priv->dir_list);

    if (dir_list->focus_row >= 1) {
        gtk_clist_row_move (dir_list, dir_list->focus_row, dir_list->focus_row-1);
        update_move_buttons (dialog, dir_list->focus_row);
    }
}


static void on_dir_move_down (GtkButton *button, GnomeCmdBookmarkDialog *dialog)
{
    GtkCList *dir_list = GTK_CLIST (dialog->priv->dir_list);

    if (dir_list->focus_row >= 0) {
        gtk_clist_row_move (dir_list, dir_list->focus_row, dir_list->focus_row+1);
        update_move_buttons (dialog, dir_list->focus_row);
    }
}


static void
on_dir_moved (GtkCList *clist, gint arg1, gint arg2, GnomeCmdBookmarkDialog *dialog)
{
    GList *bookmarks = dialog->priv->sel_group->bookmarks;
    gpointer data;

    if (!bookmarks
        || MAX (arg1, arg2) >= g_list_length (bookmarks)
        || MIN (arg1, arg2) < 0
        || arg1 == arg2)
        return;

    data = g_list_nth_data (bookmarks, arg1);
    bookmarks = g_list_remove (bookmarks, data);
    bookmarks = g_list_insert (bookmarks, data, arg2);

    dialog->priv->sel_group->bookmarks = bookmarks;
}


static void on_close (GtkButton *button, GnomeCmdBookmarkDialog *dialog)
{
    gtk_widget_destroy (GTK_WIDGET (dialog));
}


static gboolean
on_dialog_keypress (GnomeCmdBookmarkDialog *dialog, GdkEventKey *event)
{
    if (event->keyval == GDK_Escape) {
        gtk_widget_destroy (GTK_WIDGET (dialog));
        return TRUE;
    }
    else if (event->keyval == GDK_Return) {
        on_dir_goto (NULL, dialog);
        return TRUE;
    }

    return FALSE;
}


static void on_dir_selected (GtkCList *list, gint row, gint column,
                              GdkEventButton *event, GnomeCmdBookmarkDialog *dialog)
{
    if (event && event->type == GDK_2BUTTON_PRESS)
        on_dir_goto (NULL, dialog);
    else
    {
        dialog->priv->sel_bookmark = (GnomeCmdBookmark *) gtk_clist_get_row_data (list, row);
        gtk_widget_set_sensitive (dialog->priv->remove_btn, TRUE);
        gtk_widget_set_sensitive (dialog->priv->edit_btn, TRUE);
        gtk_widget_set_sensitive (dialog->priv->goto_btn, TRUE);
    }

    update_move_buttons (dialog, row);
}


static void on_dir_unselected (GtkCList *list, gint row, gint column, GdkEventButton *event, GnomeCmdBookmarkDialog *dialog)
{
    gtk_widget_set_sensitive (dialog->priv->remove_btn, FALSE);
    gtk_widget_set_sensitive (dialog->priv->edit_btn, FALSE);
    gtk_widget_set_sensitive (dialog->priv->goto_btn, FALSE);
    gtk_widget_set_sensitive (dialog->priv->move_up_btn, FALSE);
    gtk_widget_set_sensitive (dialog->priv->move_down_btn, FALSE);
}


static void
on_scroll_vertical (GtkCList *clist, GtkScrollType scroll_type, gfloat position, gpointer data)
{
    gtk_clist_select_row (clist, clist->focus_row, 0);
}


static void
add_groups (GnomeCmdBookmarkDialog *dialog)
{
    gchar *text[3];

    for (GList *groups = dialog->priv->groups; groups; groups = groups->next)
    {
        GnomeCmdBookmarkGroup *group = (GnomeCmdBookmarkGroup *) groups->data;
        GnomeCmdPixmap *pixmap = gnome_cmd_con_get_open_pixmap (group->con);

        text[0] = NULL;
        text[1] = (gchar *) gnome_cmd_con_get_alias (group->con);
        text[2] = NULL;

        gint row = gnome_cmd_combo_append (GNOME_CMD_COMBO (dialog->priv->combo), text, group);
        gnome_cmd_combo_set_pixmap (GNOME_CMD_COMBO (dialog->priv->combo), row, 0, pixmap);
    }
}


static void
add_bookmarks (GnomeCmdBookmarkDialog *dialog)
{
    GnomeCmdCon *current_con = gnome_cmd_file_selector_get_connection (gnome_cmd_main_win_get_fs (main_win, ACTIVE));
    GnomeCmdBookmarkGroup *group, *current_group = NULL;

    g_return_if_fail (current_con != NULL);

    // Then add bookmarks for all connections
    for (GList *all_cons = gnome_cmd_con_list_get_all (gnome_cmd_data_get_con_list ()); all_cons; all_cons = all_cons->next)
    {
        GnomeCmdCon *con = (GnomeCmdCon *) all_cons->data;
        group = gnome_cmd_con_get_bookmarks (con);

        if (group->bookmarks)
        {
            group->data = (gpointer *) dialog;
            dialog->priv->groups = g_list_append (dialog->priv->groups, group);

            if (con == current_con)
                current_group = group;
        }
    }

    add_groups (dialog);
    set_selected_group (dialog, current_group);
}


static void
on_group_combo_item_selected (GnomeCmdCombo *group_combo, GnomeCmdBookmarkGroup *group, GnomeCmdBookmarkDialog *dialog)
{
    g_return_if_fail (group != NULL);

    set_selected_group (dialog, group);
}


static void
on_column_resize (GtkCList *clist, gint column, gint width, GnomeCmdBookmarkDialog *dialog)
{
    gnome_cmd_data_set_bookmark_dialog_col_width (column, width);
}


/*******************************
 * Gtk class implementation
 *******************************/

static void
destroy (GtkObject *object)
{
    GnomeCmdBookmarkDialog *dialog = GNOME_CMD_BOOKMARK_DIALOG (object);

    g_list_free (dialog->priv->groups);
    g_free (dialog->priv);

    gnome_cmd_main_win_update_bookmarks (main_win);

    if (GTK_OBJECT_CLASS (parent_class)->destroy)
        (*GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}


static void
map (GtkWidget *widget)
{
    if (GTK_WIDGET_CLASS (parent_class)->map != NULL)
        GTK_WIDGET_CLASS (parent_class)->map (widget);
}


static void
class_init (GnomeCmdBookmarkDialogClass *klass)
{
    GtkObjectClass *object_class = GTK_OBJECT_CLASS (klass);
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

    parent_class = (GnomeCmdDialogClass *) gtk_type_class (gnome_cmd_dialog_get_type ());
    object_class->destroy = destroy;
    widget_class->map = ::map;
}


static void
init (GnomeCmdBookmarkDialog *in_dialog)
{
    GtkWidget *dialog;
    GtkWidget *cat;
    GtkWidget *vbox;
    GtkWidget *hbox;
    GtkWidget *bbox;
    GtkWidget *dir_list;
    GtkAccelGroup *accel_group = gtk_accel_group_new ();

    in_dialog->priv = g_new (GnomeCmdBookmarkDialogPrivate, 1);
    in_dialog->priv->groups = NULL;

    dialog = GTK_WIDGET (in_dialog);
    gtk_object_set_data (GTK_OBJECT (dialog), "dialog", dialog);
    gtk_widget_set_size_request (GTK_WIDGET (dialog), 400, 400);
    gtk_window_set_default_size (GTK_WINDOW (dialog), 500, 400);
    gtk_window_set_title (GTK_WINDOW (dialog), _("Bookmarks"));

    vbox = create_vbox (dialog, FALSE, 12);
    cat = create_category (dialog, vbox, _("Bookmark Groups"));
    gnome_cmd_dialog_add_category (GNOME_CMD_DIALOG (dialog), cat);

    in_dialog->priv->combo = create_clist_combo (dialog, 2, 1, NULL);
    gtk_clist_set_column_width (GTK_CLIST (GNOME_CMD_COMBO (in_dialog->priv->combo)->list), 0, 20);
    gtk_clist_set_column_width (GTK_CLIST (GNOME_CMD_COMBO (in_dialog->priv->combo)->list), 1, 60);
    gtk_box_pack_start (GTK_BOX (vbox), in_dialog->priv->combo, TRUE, TRUE, 0);

    hbox = create_hbox (dialog, FALSE, 12);
    cat = create_category (dialog, hbox, _("Bookmarks"));
    gnome_cmd_dialog_add_expanding_category (GNOME_CMD_DIALOG (dialog), cat);

    dir_list = create_clist (dialog, "dir_list", 3, 16,
                             GTK_SIGNAL_FUNC (on_dir_selected), GTK_SIGNAL_FUNC (on_dir_moved));
    gtk_box_pack_start (GTK_BOX (hbox), dir_list, TRUE, TRUE, 0);
    create_clist_column (dir_list, 0, gnome_cmd_data_get_bookmark_dialog_col_width (0), "");
    create_clist_column (dir_list, 1, gnome_cmd_data_get_bookmark_dialog_col_width (1), _("name"));
    create_clist_column (dir_list, 2, gnome_cmd_data_get_bookmark_dialog_col_width (2), _("path"));

    in_dialog->priv->dir_list = lookup_widget (GTK_WIDGET (dialog), "dir_list");

    bbox = create_vbox (dialog, FALSE, 12);
    gtk_box_pack_start (GTK_BOX (hbox), bbox, FALSE, FALSE, 0);

    in_dialog->priv->goto_btn = create_named_button (dialog, _("_Goto"), "goto_button", GTK_SIGNAL_FUNC (on_dir_goto));
    GTK_WIDGET_SET_FLAGS (in_dialog->priv->goto_btn, GTK_CAN_DEFAULT);
    gtk_box_pack_start (GTK_BOX (bbox), in_dialog->priv->goto_btn, FALSE, TRUE, 0);
    gtk_widget_set_sensitive (GTK_WIDGET (in_dialog->priv->goto_btn), FALSE);

    in_dialog->priv->edit_btn = create_named_button (dialog, _("_Edit"), "edit_button", GTK_SIGNAL_FUNC (on_dir_edit));
    GTK_WIDGET_SET_FLAGS (in_dialog->priv->edit_btn, GTK_CAN_DEFAULT);
    gtk_box_pack_start (GTK_BOX (bbox), in_dialog->priv->edit_btn, FALSE, TRUE, 0);
    gtk_widget_set_sensitive (GTK_WIDGET (in_dialog->priv->edit_btn), FALSE);

    in_dialog->priv->remove_btn = create_named_button (dialog, _("_Remove"), "remove_button", GTK_SIGNAL_FUNC (on_dir_remove));
    GTK_WIDGET_SET_FLAGS (in_dialog->priv->remove_btn, GTK_CAN_DEFAULT);
    gtk_box_pack_start (GTK_BOX (bbox), in_dialog->priv->remove_btn, FALSE, TRUE, 0);
    gtk_widget_set_sensitive (GTK_WIDGET (in_dialog->priv->remove_btn), FALSE);

    in_dialog->priv->move_up_btn = create_named_stock_button (
        dialog, GNOME_STOCK_BUTTON_UP, "move_up_button",
        GTK_SIGNAL_FUNC (on_dir_move_up));
    GTK_WIDGET_SET_FLAGS (in_dialog->priv->move_up_btn, GTK_CAN_DEFAULT);
    gtk_box_pack_start (GTK_BOX (bbox), in_dialog->priv->move_up_btn, FALSE, TRUE, 0);
    gtk_widget_set_sensitive (GTK_WIDGET (in_dialog->priv->move_up_btn), FALSE);

    in_dialog->priv->move_down_btn = create_named_stock_button (
        dialog, GNOME_STOCK_BUTTON_DOWN, "move_down_button",
        GTK_SIGNAL_FUNC (on_dir_move_down));
    GTK_WIDGET_SET_FLAGS (in_dialog->priv->move_down_btn, GTK_CAN_DEFAULT);
    gtk_box_pack_start (GTK_BOX (bbox), in_dialog->priv->move_down_btn, FALSE, TRUE, 0);
    gtk_widget_set_sensitive (GTK_WIDGET (in_dialog->priv->move_down_btn), FALSE);


    gnome_cmd_dialog_add_button (GNOME_CMD_DIALOG (dialog), GNOME_STOCK_BUTTON_CLOSE,
                                 GTK_SIGNAL_FUNC (on_close), dialog);

    gtk_signal_connect (GTK_OBJECT (dialog), "key-press-event",
                        GTK_SIGNAL_FUNC (on_dialog_keypress), dialog);
    gtk_signal_connect_after (GTK_OBJECT (in_dialog->priv->dir_list), "scroll_vertical",
                              GTK_SIGNAL_FUNC (on_scroll_vertical), NULL);
    gtk_signal_connect (GTK_OBJECT (in_dialog->priv->dir_list), "unselect-row",
                        GTK_SIGNAL_FUNC (on_dir_unselected), dialog);
    gtk_signal_connect (GTK_OBJECT (in_dialog->priv->dir_list), "resize-column",
                        GTK_SIGNAL_FUNC (on_column_resize), dialog);
    gtk_signal_connect (GTK_OBJECT (in_dialog->priv->combo), "item-selected",
                        GTK_SIGNAL_FUNC (on_group_combo_item_selected), dialog);

    gtk_widget_grab_focus (in_dialog->priv->dir_list);
}


/***********************************
 * Public functions
 ***********************************/

GtkType
gnome_cmd_bookmark_dialog_get_type (void)
{
    static GtkType dlg_type = 0;

    if (dlg_type == 0)
    {
        GtkTypeInfo dlg_info =
        {
            "GnomeCmdBookmarkDialog",
            sizeof (GnomeCmdBookmarkDialog),
            sizeof (GnomeCmdBookmarkDialogClass),
            (GtkClassInitFunc) class_init,
            (GtkObjectInitFunc) init,
            /* reserved_1 */ NULL,
            /* reserved_2 */ NULL,
            (GtkClassInitFunc) NULL
        };

        dlg_type = gtk_type_unique (gnome_cmd_dialog_get_type (), &dlg_info);
    }
    return dlg_type;
}


GtkWidget *
gnome_cmd_bookmark_dialog_new (void)
{
    GnomeCmdBookmarkDialog *dialog = (GnomeCmdBookmarkDialog *) gtk_type_new (gnome_cmd_bookmark_dialog_get_type ());

    add_bookmarks (dialog);

    GTK_CLIST (dialog->priv->dir_list)->focus_row = 0;
    gtk_clist_select_row (GTK_CLIST (dialog->priv->dir_list), 0, 0);

    return GTK_WIDGET (dialog);
}


static gboolean
on_new_bookmark_ok (GnomeCmdStringDialog *string_dialog, const gchar **values, gpointer data)
{
    GnomeCmdBookmark *bookmark = g_new (GnomeCmdBookmark, 1);
    GnomeCmdCon *con = gnome_cmd_file_selector_get_connection (gnome_cmd_main_win_get_fs (main_win, ACTIVE));
    GnomeCmdBookmarkGroup *group = gnome_cmd_con_get_bookmarks (con);

    bookmark->name = g_strdup (values[0]);
    bookmark->path = g_strdup (values[1]);
    bookmark->group = group;

    group->bookmarks = g_list_append (group->bookmarks, bookmark);

    gnome_cmd_main_win_update_bookmarks (main_win);

    return TRUE;
}


void
gnome_cmd_bookmark_add_current (void)
{
    GnomeCmdDir *cwd = gnome_cmd_file_selector_get_directory (gnome_cmd_main_win_get_fs (main_win, ACTIVE));
    gchar *path = gnome_cmd_file_get_path (GNOME_CMD_FILE (cwd));

    if (!g_utf8_validate (path, -1, NULL)) {
        create_error_dialog (_("To bookmark a directory the whole search path to the directory must be in valid UTF-8 encoding\n"));
        g_free (path);
        return;
    }

    show_bookmark_dialog (g_basename (path),
                          path,
                          _("New Bookmark"),
                          (GnomeCmdStringDialogCallback)on_new_bookmark_ok,
                          NULL);
    g_free (path);
}


void
gnome_cmd_bookmark_goto (GnomeCmdBookmark *bookmark)
{
    GnomeCmdCon *current_con;
    GnomeCmdFileSelector *fs = gnome_cmd_main_win_get_fs (main_win, ACTIVE);

    g_return_if_fail (GNOME_CMD_IS_FILE_SELECTOR (fs));

    current_con = gnome_cmd_file_selector_get_connection (fs);
    g_return_if_fail (bookmark->group->con != NULL);

    if (current_con == bookmark->group->con)
        gnome_cmd_file_selector_goto_directory (fs, bookmark->path);
    else {
        GnomeCmdCon *con = bookmark->group->con;

        if (con->state == CON_STATE_OPEN) {
            GnomeCmdDir *dir = gnome_cmd_dir_new (con, gnome_cmd_con_create_path (con, bookmark->path));
            gnome_cmd_file_selector_set_connection (fs, con, dir);

        }
        else {
            if (con->base_path)
                gtk_object_unref (GTK_OBJECT (con->base_path));

            con->base_path = gnome_cmd_con_create_path (con, bookmark->path);
            gtk_object_ref (GTK_OBJECT (con->base_path));
            gnome_cmd_file_selector_set_connection (fs, con, NULL);
        }
    }
}
