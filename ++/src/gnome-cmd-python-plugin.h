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

#ifndef __GNOME_CMD_PYTHON_PLUGIN_H__
#define __GNOME_CMD_PYTHON_PLUGIN_H__

#include "gnome-cmd-main-win.h"

G_BEGIN_DECLS

typedef struct
{
    gchar *name;        // plugin name
    gchar *path;        // full path to plugin (including file name, but without ext: .py or .pyc)
    gchar *fname;       // file name (without ext: .py or .pyc)
} PythonPluginData;


void python_plugin_manager_init ();
void python_plugin_manager_shutdown ();

GList *gnome_cmd_python_plugin_get_list();
gboolean gnome_cmd_python_plugin_execute(const PythonPluginData *plugin, GnomeCmdMainWin *mw);

G_END_DECLS

#endif // __GNOME_CMD_PYTHON_PLUGIN_H__
