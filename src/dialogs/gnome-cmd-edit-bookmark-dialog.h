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

#ifndef __GNOME_CMD_EDIT_BOOKMARK_DIALOG_H__
#define __GNOME_CMD_EDIT_BOOKMARK_DIALOG_H__

#include "gnome-cmd-data.h"

gboolean gnome_cmd_edit_bookmark_dialog (GtkWindow *parent, const gchar *title, gchar *&name, gchar *&path);

#endif // __GNOME_CMD_EDIT_BOOKMARK_DIALOG_H__
