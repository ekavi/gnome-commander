/*
    GNOME Commander - A GNOME based file manager
    Copyright (C) 2001-2006 Marcus Bjurman
    Copyright (C) 2007-2012 Piotr Eljasiak
    Copyright (C) 2013-2016 Uwe Scholz

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
#ifndef __GNOME_CMD_CHMOD_DIALOG_H__
#define __GNOME_CMD_CHMOD_DIALOG_H__

#define GNOME_CMD_TYPE_CHMOD_DIALOG              (gnome_cmd_chmod_dialog_get_type ())
#define GNOME_CMD_CHMOD_DIALOG(obj)              (G_TYPE_CHECK_INSTANCE_CAST((obj), GNOME_CMD_TYPE_CHMOD_DIALOG, GnomeCmdChmodDialog))
#define GNOME_CMD_CHMOD_DIALOG_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST((klass), GNOME_CMD_TYPE_CHMOD_DIALOG, GnomeCmdChmodDialogClass))
#define GNOME_CMD_IS_CHMOD_DIALOG(obj)           (G_TYPE_CHECK_INSTANCE_TYPE((obj), GNOME_CMD_TYPE_CHMOD_DIALOG))
#define GNOME_CMD_IS_CHMOD_DIALOG_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GNOME_CMD_TYPE_CHMOD_DIALOG))
#define GNOME_CMD_CHMOD_DIALOG_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS((obj), GNOME_CMD_TYPE_CHMOD_DIALOG, GnomeCmdChmodDialogClass))


GType gnome_cmd_chmod_dialog_get_type ();


struct GnomeCmdChmodDialogPrivate;


struct GnomeCmdChmodDialog
{
    GnomeCmdDialog parent;
    GnomeCmdChmodDialogPrivate *priv;
};


struct GnomeCmdChmodDialogClass
{
    GnomeCmdDialogClass parent_class;
};


GtkWidget *gnome_cmd_chmod_dialog_new (GList *files);

#endif // __GNOME_CMD_CHMOD_DIALOG_H__
