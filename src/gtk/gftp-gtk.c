/*****************************************************************************/
/*  gftp-gtk.c - GTK+ 1.2 port of gftp                                       */
/*  Copyright (C) 1998-2007 Brian Masney <masneyb@gftp.org>                  */
/*                                                                           */
/*  This program is free software; you can redistribute it and/or modify     */
/*  it under the terms of the GNU General Public License as published by     */
/*  the Free Software Foundation; either version 2 of the License, or        */
/*  (at your option) any later version.                                      */
/*                                                                           */
/*  This program is distributed in the hope that it will be useful,          */
/*  but WITHOUT ANY WARRANTY; without even the implied warranty of           */
/*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            */
/*  GNU General Public License for more details.                             */
/*                                                                           */
/*  You should have received a copy of the GNU General Public License        */
/*  along with this program; if not, write to the Free Software              */
/*  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111 USA      */
/*****************************************************************************/

#include "gftp-gtk.h"

static GtkWidget * local_frame, * remote_frame, * log_table, * transfer_scroll,
                 * gftpui_command_toolbar;

gftp_window_data window1, window2, *other_wdata, *current_wdata;
GtkWidget * stop_btn, * hostedit, * useredit, * passedit, * portedit, * logwdw,
          * dlwdw, * optionmenu, * gftpui_command_widget, * download_left_arrow,
          * upload_right_arrow, * openurl_btn;
GtkAdjustment * logwdw_vadj;
GtkTextMark * logwdw_textmark;
int local_start, remote_start, trans_start;
GHashTable * graphic_hash_table = NULL;
GtkUIManager * ui_manager;
GList * viewedit_processes = NULL;

static int
get_column (GtkTreeView * listbox, int column)
{
  GtkTreeViewColumn * col = gtk_tree_view_get_column(listbox, column);
  if (gtk_tree_view_column_get_expand(col))
    return 0;
  else if (! gtk_tree_view_column_get_visible(col))
    return -1;
  else
    return gtk_tree_view_column_get_width(col);
}


static void
_gftp_exit (GtkWidget * widget, gpointer data)
{
  intptr_t remember_last_directory;
  const char *tempstr;
  const char * tempwid;
  intptr_t ret;

#if GTK_MAJOR_VERSION == 3
  ret = gtk_widget_get_allocated_width(GTK_WIDGET (local_frame));
  gftp_set_global_option ("listbox_local_width", GINT_TO_POINTER (ret));
  ret = gtk_widget_get_allocated_width(GTK_WIDGET (remote_frame));
  gftp_set_global_option ("listbox_remote_width", GINT_TO_POINTER (ret));
  ret = gtk_widget_get_allocated_height(GTK_WIDGET (remote_frame));
  gftp_set_global_option ("listbox_file_height", GINT_TO_POINTER (ret));
  ret = gtk_widget_get_allocated_height(GTK_WIDGET (log_table));
  gftp_set_global_option ("log_height", GINT_TO_POINTER (ret));
  ret = gtk_widget_get_allocated_height(GTK_WIDGET (transfer_scroll));
  gftp_set_global_option ("transfer_height", GINT_TO_POINTER (ret));
#endif

  ret = get_column (GTK_TREE_VIEW (dlwdw), 0);
  gftp_set_global_option ("file_trans_column", GINT_TO_POINTER (ret));

  ret = get_column (GTK_TREE_VIEW (window1.listbox), 1);
  gftp_set_global_option ("local_file_width", GINT_TO_POINTER (ret));
  ret = get_column (GTK_TREE_VIEW (window1.listbox), 2);
  gftp_set_global_option ("local_size_width", GINT_TO_POINTER (ret));
  ret = get_column (GTK_TREE_VIEW (window1.listbox), 3);
  gftp_set_global_option ("local_user_width", GINT_TO_POINTER (ret));
  ret = get_column (GTK_TREE_VIEW (window1.listbox), 4);
  gftp_set_global_option ("local_group_width", GINT_TO_POINTER (ret));
  ret = get_column (GTK_TREE_VIEW (window1.listbox), 5);
  gftp_set_global_option ("local_date_width", GINT_TO_POINTER (ret));
  ret = get_column (GTK_TREE_VIEW (window1.listbox), 6);
  gftp_set_global_option ("local_attribs_width", GINT_TO_POINTER (ret));

  ret = get_column (GTK_TREE_VIEW (window2.listbox), 1);
  gftp_set_global_option ("remote_file_width", GINT_TO_POINTER (ret));
  ret = get_column (GTK_TREE_VIEW (window2.listbox), 2);
  gftp_set_global_option ("remote_size_width", GINT_TO_POINTER (ret));
  ret = get_column (GTK_TREE_VIEW (window2.listbox), 3);
  gftp_set_global_option ("remote_user_width", GINT_TO_POINTER (ret));
  ret = get_column (GTK_TREE_VIEW (window2.listbox), 4);
  gftp_set_global_option ("remote_group_width", GINT_TO_POINTER (ret));
  ret = get_column (GTK_TREE_VIEW (window2.listbox), 5);
  gftp_set_global_option ("remote_date_width", GINT_TO_POINTER (ret));
  ret = get_column (GTK_TREE_VIEW (window2.listbox), 6);
  gftp_set_global_option ("remote_attribs_width", GINT_TO_POINTER (ret));

  tempstr = gtk_entry_get_text (GTK_ENTRY (gtk_bin_get_child (GTK_BIN (GTK_COMBO_BOX_TEXT (hostedit)))));
  gftp_set_global_option ("host_value", tempstr);

  tempstr = gtk_entry_get_text (GTK_ENTRY (gtk_bin_get_child (GTK_BIN (GTK_COMBO_BOX_TEXT (portedit)))));
  gftp_set_global_option ("port_value", tempstr);

  tempstr = gtk_entry_get_text (GTK_ENTRY (gtk_bin_get_child (GTK_BIN (GTK_COMBO_BOX_TEXT (useredit)))));
  gftp_set_global_option ("user_value", tempstr);

  gftp_lookup_global_option ("remember_last_directory",
                             &remember_last_directory);
  if (remember_last_directory)
    {
      tempstr = gtk_entry_get_text (GTK_ENTRY (gtk_bin_get_child (GTK_BIN (GTK_COMBO_BOX_TEXT (window1.combo)))));
      gftp_set_global_option ("local_startup_directory", tempstr);

      tempstr = gtk_entry_get_text (GTK_ENTRY (gtk_bin_get_child (GTK_BIN (GTK_COMBO_BOX_TEXT (window2.combo)))));
      gftp_set_global_option ("remote_startup_directory", tempstr);
    }

  tempwid = gtk_combo_box_text_get_active_text (GTK_COMBO_BOX_TEXT (optionmenu));
  gftp_set_global_option ("default_protocol", tempwid);

  gftp_shutdown ();
  exit (0);
}

static void
ui_disconnect (GtkAction * a, void * uidata)
{
   gftpui_disconnect (uidata);
}

static gint
_gftp_try_close (GtkWidget * widget, void * a, gpointer data)
{
  if (gftp_file_transfers == NULL)
    {
      _gftp_exit (NULL, NULL);
      return (0);
    }
  else
    {
      MakeYesNoDialog (_("Exit"), _("There are file transfers in progress.\nAre you sure you want to exit?"), _gftp_exit, NULL, NULL, NULL);
      return (1);
    }
}


static void
_gftp_force_close (GtkWidget * widget, gpointer data)
{
  exit (0);
}


static void
_gftp_menu_exit (GtkAction * a, gpointer data)
{
  if (!_gftp_try_close (NULL, NULL, NULL))
    _gftp_exit (NULL, NULL);
}


static void
change_setting (gftp_window_data * wdata, int menuitem, GtkWidget * checkmenu)
{
  switch (menuitem)
    {
    case GFTP_MENU_ITEM_ASCII:
      gftp_set_global_option ("ascii_transfers", GINT_TO_POINTER(1));
      break;
    case GFTP_MENU_ITEM_BINARY:
      gftp_set_global_option ("ascii_transfers", GINT_TO_POINTER(0));
      break;
    case GFTP_MENU_ITEM_WIN1:
      current_wdata = &window1;
      other_wdata = &window2;

      if (wdata->request != NULL)
        update_window_info ();

      break;
    case GFTP_MENU_ITEM_WIN2:
      current_wdata = &window2;
      other_wdata = &window1;

      if (wdata->request != NULL)
        update_window_info ();

      break;
    }
}


static void
_gftpui_gtk_do_openurl (gftp_window_data * wdata, gftp_dialog_data * ddata)
{
  const char *tempstr;
  char *buf;

  tempstr = gtk_entry_get_text (GTK_ENTRY (ddata->edit));
  if (tempstr != NULL && *tempstr != '\0')
    {
      buf = g_strdup (tempstr);
      destroy_dialog (ddata);
      gftpui_common_cmd_open (wdata, wdata->request, NULL, NULL, buf);
      g_free (buf);
    }
}


static void
openurl_dialog (GtkAction * a, gpointer data)
{
  gftp_window_data * wdata;

  wdata = data;
  MakeEditDialog (_("Open Location"), _("Enter a URL to connect to"),
                  NULL, 1, NULL, gftp_dialog_button_connect,
                  _gftpui_gtk_do_openurl, wdata,
                  NULL, NULL);
}


static void
tb_openurl_dialog (GtkToolButton *toolbutton, gpointer data)
{
  const char *edttxt;

  if (current_wdata->request->stopable)
    {
      ftp_log (gftp_logging_error, NULL,
               _("%s: Please hit the stop button first to do anything else\n"),
               _("Open Location"));
      return;
    }

  edttxt = gtk_entry_get_text (GTK_ENTRY (gtk_bin_get_child (GTK_BIN (GTK_COMBO_BOX_TEXT (hostedit)))));

  if (GFTP_IS_CONNECTED (current_wdata->request))
    gftpui_disconnect (current_wdata);
  else if (edttxt != NULL && *edttxt != '\0')
    toolbar_hostedit (NULL, NULL);
  else
    openurl_dialog (NULL, current_wdata);
}

static void
gftp_gtk_refresh (GtkAction * a, gftp_window_data * wdata)
{
  gftpui_refresh (wdata, 1);
}

static GtkWidget *
CreateMenus (GtkWidget * window)
{
  GtkAccelGroup * accel_group;
  intptr_t ascii_transfers;

GtkActionGroup *action_group;
GError *error;

static const GtkRadioActionEntry radio_entries[] = {
  { "Window _1", NULL, N_("Window _1"), "<control>1", NULL, GFTP_MENU_ITEM_WIN1 },
  { "Window _2", NULL, N_("Window _2"), "<control>2", NULL, GFTP_MENU_ITEM_WIN2 },
};


static const GtkRadioActionEntry radio_entriess[] = {
  { "Ascii", NULL, N_("_Ascii"), NULL, NULL, GFTP_MENU_ITEM_ASCII },
  { "Binary", NULL, N_("_Binary"), NULL, NULL, GFTP_MENU_ITEM_BINARY },
};

  static const GtkActionEntry entries1[] = {
     { "PLocal", NULL, N_("_Local")},
     { "Local", NULL, N_("_Local")},
     { "local_Open Location...", GTK_STOCK_OPEN, N_("_Open Location..."), "<control><shift>O", NULL, G_CALLBACK(openurl_dialog)},
     { "local_D_isconnect", GTK_STOCK_CLOSE, N_("D_isconnect"), "<control><shift>I", NULL, G_CALLBACK(ui_disconnect)},

     { "local_Change _Filespec...", NULL, N_("Change _Filespec..."), "<control><shift>F", NULL, G_CALLBACK(change_filespec)},
     { "local_Show selected", NULL, N_("_Show selected"), NULL, NULL, G_CALLBACK(show_selected)},
     { "local_Select _All", NULL, N_("Select _All"), "<control><shift>A", NULL, G_CALLBACK(selectall)},
     { "local_Select All Files", NULL, N_("Select All Files"), NULL, NULL, G_CALLBACK(selectallfiles)},
     { "local_Deselect All", NULL, N_("Deselect All"), NULL, NULL, G_CALLBACK(deselectall)},

     { "local_Save Directory Listing...", NULL, N_("Save Directory Listing..."), NULL, NULL, G_CALLBACK(save_directory_listing)},
     { "local_Send SITE Command...", NULL, N_("Send SITE Command..."), NULL, NULL, G_CALLBACK(gftpui_site_dialog)},
     { "local_Change Directory", NULL, N_("_Change Directory"), NULL, NULL, G_CALLBACK(gftpui_chdir_dialog)},
     { "local_Permissions...", NULL, N_("_Permissions..."), "<control><shift>P", NULL, G_CALLBACK(chmod_dialog)},
     { "local_New Folder...", NULL, N_("_New Folder..."), "<control><shift>N", NULL, G_CALLBACK(gftpui_mkdir_dialog)},
     { "local_Rena_me...", NULL, N_("Rena_me..."), "<control><shift>M", NULL, G_CALLBACK(gftpui_rename_dialog)},
     { "local_Delete...", NULL, N_("_Delete..."), "<control><shift>D", NULL, G_CALLBACK(delete_dialog)},
     { "local_Edit...", NULL, N_("_Edit..."), "<control><shift>E", NULL, G_CALLBACK(edit_dialog)},
     { "local_View...", NULL, N_("_View..."), "<control><shift>V", NULL, G_CALLBACK(view_dialog)},
     { "local_Refresh", GTK_STOCK_REFRESH, N_("_Refresh"), "<control><shift>R", NULL, G_CALLBACK(gftp_gtk_refresh)},
  };

  static const GtkActionEntry entries2[] = {
     { "Remote", NULL, N_("_Remote")},
     { "PRemote", NULL, N_("_Remote")},
     { "remote_Open Location...", GTK_STOCK_OPEN, N_("_Open Location..."), "<control>O", NULL, G_CALLBACK(openurl_dialog)},
     { "remote_D_isconnect", GTK_STOCK_CLOSE, N_("D_isconnect"), "<control>D", NULL, G_CALLBACK(ui_disconnect)},

     { "remote_Change _Filespec...", NULL, N_("Change _Filespec..."), "<control>F", NULL, G_CALLBACK(change_filespec)},
     { "remote_Show selected", NULL, N_("_Show selected"), NULL, NULL, G_CALLBACK(show_selected)},
     { "remote_Select _All", NULL, N_("Select _All"), "<control>A", NULL, G_CALLBACK(selectall)},
     { "remote_Select All Files", NULL, N_("Select All Files"), NULL, NULL, G_CALLBACK(selectallfiles)},
     { "remote_Deselect All", NULL, N_("Deselect All"), NULL, NULL, G_CALLBACK(deselectall)},

     { "remote_Save Directory Listing...", NULL, N_("Save Directory Listing..."), NULL, NULL, G_CALLBACK(save_directory_listing)},
     { "remote_Send SITE Command...", NULL, N_("Send SITE Command..."), NULL, NULL, G_CALLBACK(gftpui_site_dialog)},
     { "remote_Change Directory", NULL, N_("_Change Directory"), NULL, NULL, G_CALLBACK(gftpui_chdir_dialog)},
     { "remote_Permissions...", NULL, N_("_Permissions..."), "<control>P", NULL, G_CALLBACK(chmod_dialog)},
     { "remote_New Folder...", NULL, N_("_New Folder..."), "<control>N", NULL, G_CALLBACK(gftpui_mkdir_dialog)},
     { "remote_Rena_me...", NULL, N_("Rena_me..."), "<control>M", NULL, G_CALLBACK(gftpui_rename_dialog)},
     { "remote_Delete...", NULL, N_("_Delete..."), "<control>D", NULL, G_CALLBACK(delete_dialog)},
     { "remote_Edit...", NULL, N_("_Edit..."), "<control>E", NULL, G_CALLBACK(edit_dialog)},
     { "remote_View...", NULL, N_("_View..."), "<control>V", NULL, G_CALLBACK(view_dialog)},
     { "remote_Refresh", GTK_STOCK_REFRESH, N_("_Refresh"), "<control>R", NULL, G_CALLBACK(gftp_gtk_refresh)},

     { "Bookmarks", NULL, N_("_Bookmarks")},
     { "Add _Bookmark", GTK_STOCK_ADD, N_("Add _Bookmark"), "<control>B", NULL, G_CALLBACK(add_bookmark)},
     { "Edit Bookmarks", NULL, N_("Edit Bookmarks"), NULL, NULL, G_CALLBACK(edit_bookmarks)},
  };

  static const GtkActionEntry entries[] = {
     { "FTP", NULL, N_("_FTP")},
     { "Preferences...", GTK_STOCK_PREFERENCES, N_("_Preferences..."), NULL, NULL, G_CALLBACK(options_dialog)},
     { "Quit", GTK_STOCK_QUIT, N_("_Quit"), "<control>Q", NULL, G_CALLBACK(_gftp_menu_exit)},

     { "PTransfer", NULL, N_("_Transfer")},
     { "Transfer", NULL, N_("_Transfer")},
     { "t_Start", NULL, N_("_Start"), NULL, NULL, G_CALLBACK(start_transfer)},
     { "t_St_op", GTK_STOCK_STOP, N_("St_op"), NULL, NULL, G_CALLBACK(stop_transfer)},

     { "t_Skip _Current File", NULL, N_("Skip _Current File"), NULL, NULL, G_CALLBACK(skip_transfer)},
     { "t_Remove File", GTK_STOCK_DELETE, N_("_Remove File"), NULL, NULL, G_CALLBACK(remove_file_transfer)},
     { "t_Move File _Up", GTK_STOCK_GO_UP, N_("Move File _Up"), NULL, NULL, G_CALLBACK(move_transfer_up)},
     { "t_Move File _Down", GTK_STOCK_GO_DOWN, N_("Move File _Down"), NULL, NULL, G_CALLBACK(move_transfer_down)},

     { "t_Retrieve Files", NULL, N_("_Retrieve Files"), "<control>R", NULL, G_CALLBACK(get_files)},
     { "t_Put Files", NULL, N_("_Put Files"), "<control>P", NULL, G_CALLBACK(put_files)},

     { "Log", NULL, N_("L_og")},
     { "PLog", NULL, N_("L_og")},
     { "Log_Clear", GTK_STOCK_CLEAR, N_("Log/_Clear"), NULL, NULL, G_CALLBACK(clearlog)},
     { "Log_View", NULL, N_("_View"), NULL, NULL, G_CALLBACK(viewlog)},
     { "Log_Save...", GTK_STOCK_SAVE, N_("_Save..."), NULL, NULL, G_CALLBACK(savelog)},
     { "Tools", NULL, N_("Tool_s")},
     { "C_ompare Windows", NULL, N_("C_ompare Windows"), NULL, NULL, G_CALLBACK(compare_windows)},
     { "Clear Cache", GTK_STOCK_CLEAR, N_("_Clear Cache"), NULL, NULL, G_CALLBACK(clear_cache)},
     { "Help", NULL, N_("Help")},
     { "About", GTK_STOCK_ABOUT, N_("_About"), NULL, NULL, G_CALLBACK(about_dialog) }
  };

static const char *ui_description =
"<ui>"
"  <menubar name='MainMenu'>"
"    <menu action='FTP'>"
"      <menuitem action='Window _1'/>"
"      <menuitem action='Window _2'/>"
"      <separator/>"
"      <menuitem action='Ascii'/>"
"      <menuitem action='Binary'/>"
"      <separator/>"
"      <menuitem action='Preferences...'/>"
"      <separator/>"
"      <menuitem action='Quit'/>"
"    </menu>"
"    <menu action='Local'>"
"      <menuitem action='local_Open Location...'/>"
"      <menuitem action='local_D_isconnect'/>"
"      <separator/>"
"      <menuitem action='local_Change _Filespec...'/>"
"      <menuitem action='local_Show selected'/>"
"      <menuitem action='local_Select _All'/>"
"      <menuitem action='local_Select All Files'/>"
"      <menuitem action='local_Deselect All'/>"
"      <separator/>"
"      <menuitem action='local_Save Directory Listing...'/>"
"      <menuitem action='local_Send SITE Command...'/>"
"      <menuitem action='local_Change Directory'/>"
"      <menuitem action='local_Permissions...'/>"
"      <menuitem action='local_New Folder...'/>"
"      <menuitem action='local_Rena_me...'/>"
"      <menuitem action='local_Delete...'/>"
"      <menuitem action='local_Edit...'/>"
"      <menuitem action='local_View...'/>"
"      <menuitem action='local_Refresh'/>"
"    </menu>"
"    <menu action='Remote'>"
"      <menuitem action='remote_Open Location...'/>"
"      <menuitem action='remote_D_isconnect'/>"
"      <separator/>"
"      <menuitem action='remote_Change _Filespec...'/>"
"      <menuitem action='remote_Show selected'/>"
"      <menuitem action='remote_Select _All'/>"
"      <menuitem action='remote_Select All Files'/>"
"      <menuitem action='remote_Deselect All'/>"
"      <separator/>"
"      <menuitem action='remote_Save Directory Listing...'/>"
"      <menuitem action='remote_Send SITE Command...'/>"
"      <menuitem action='remote_Change Directory'/>"
"      <menuitem action='remote_Permissions...'/>"
"      <menuitem action='remote_New Folder...'/>"
"      <menuitem action='remote_Rena_me...'/>"
"      <menuitem action='remote_Delete...'/>"
"      <menuitem action='remote_Edit...'/>"
"      <menuitem action='remote_View...'/>"
"      <menuitem action='remote_Refresh'/>"
"    </menu>"
"    <menu action='Bookmarks'>"
"      <menuitem action='Add _Bookmark'/>"
"      <menuitem action='Edit Bookmarks'/>"
"      <separator/>"
"      <placeholder name='Bookmarks Placeholder'/>"
"    </menu>"
"    <menu action='Transfer'>"
"      <menuitem action='t_Start'/>"
"      <menuitem action='t_St_op'/>"
"      <separator/>"
"      <menuitem action='t_Skip _Current File'/>"
"      <menuitem action='t_Remove File'/>"
"      <menuitem action='t_Move File _Up'/>"
"      <menuitem action='t_Move File _Down'/>"
"      <separator/>"
"      <menuitem action='t_Retrieve Files'/>"
"      <menuitem action='t_Put Files'/>"
"    </menu>"
"    <menu action='Log'>"
"      <menuitem action='Log_Clear'/>"
"      <menuitem action='Log_View'/>"
"      <menuitem action='Log_Save...'/>"
"    </menu>"
"    <menu action='Tools'>"
"      <menuitem action='C_ompare Windows'/>"
"      <menuitem action='Clear Cache'/>"
"    </menu>"
"    <menu action='Help'>"
"      <menuitem action='About'/>"
"    </menu>"
"  </menubar>"

"    <popup action='PLocal'>"
"      <menuitem action='local_Open Location...'/>"
"      <menuitem action='local_D_isconnect'/>"
"      <separator/>"
"      <menuitem action='local_Change _Filespec...'/>"
"      <menuitem action='local_Show selected'/>"
"      <menuitem action='local_Select _All'/>"
"      <menuitem action='local_Select All Files'/>"
"      <menuitem action='local_Deselect All'/>"
"      <separator/>"
"      <menuitem action='local_Save Directory Listing...'/>"
"      <menuitem action='local_Send SITE Command...'/>"
"      <menuitem action='local_Change Directory'/>"
"      <menuitem action='local_Permissions...'/>"
"      <menuitem action='local_New Folder...'/>"
"      <menuitem action='local_Rena_me...'/>"
"      <menuitem action='local_Delete...'/>"
"      <menuitem action='local_Edit...'/>"
"      <menuitem action='local_View...'/>"
"      <menuitem action='local_Refresh'/>"
"    </popup>"
"    <popup action='PRemote'>"
"      <menuitem action='remote_Open Location...'/>"
"      <menuitem action='remote_D_isconnect'/>"
"      <separator/>"
"      <menuitem action='remote_Change _Filespec...'/>"
"      <menuitem action='remote_Show selected'/>"
"      <menuitem action='remote_Select _All'/>"
"      <menuitem action='remote_Select All Files'/>"
"      <menuitem action='remote_Deselect All'/>"
"      <separator/>"
"      <menuitem action='remote_Save Directory Listing...'/>"
"      <menuitem action='remote_Send SITE Command...'/>"
"      <menuitem action='remote_Change Directory'/>"
"      <menuitem action='remote_Permissions...'/>"
"      <menuitem action='remote_New Folder...'/>"
"      <menuitem action='remote_Rena_me...'/>"
"      <menuitem action='remote_Delete...'/>"
"      <menuitem action='remote_Edit...'/>"
"      <menuitem action='remote_View...'/>"
"      <menuitem action='remote_Refresh'/>"
"    </popup>"

"    <popup action='PTransfer'>"
"      <menuitem action='t_Start'/>"
"      <menuitem action='t_St_op'/>"
"      <separator/>"
"      <menuitem action='t_Skip _Current File'/>"
"      <menuitem action='t_Remove File'/>"
"      <menuitem action='t_Move File _Up'/>"
"      <menuitem action='t_Move File _Down'/>"
"      <separator/>"
"      <menuitem action='t_Retrieve Files'/>"
"      <menuitem action='t_Put Files'/>"
"    </popup>"
"    <popup action='PLog'>"
"      <menuitem action='Log_Clear'/>"
"      <menuitem action='Log_View'/>"
"      <menuitem action='Log_Save...'/>"
"    </popup>"

"</ui>";

  action_group = gtk_action_group_new ("MenuActions");
  gtk_action_group_add_actions (action_group, entries, G_N_ELEMENTS (entries), NULL);
  gtk_action_group_add_actions (action_group, entries1, G_N_ELEMENTS (entries1), &window1);
  gtk_action_group_add_actions (action_group, entries2, G_N_ELEMENTS (entries2), &window2);
  gtk_action_group_add_radio_actions (action_group, radio_entries, G_N_ELEMENTS (radio_entries), 1, G_CALLBACK(change_setting), window);
  gftp_lookup_global_option ("ascii_transfers", &ascii_transfers);
  gtk_action_group_add_radio_actions (action_group, radio_entriess, G_N_ELEMENTS (radio_entriess), ascii_transfers, G_CALLBACK(change_setting), window);

  ui_manager = gtk_ui_manager_new ();
  gtk_ui_manager_insert_action_group (ui_manager, action_group, 0);

  accel_group = gtk_ui_manager_get_accel_group (ui_manager);

  error = NULL;
  if (!gtk_ui_manager_add_ui_from_string (ui_manager, ui_description, -1, &error))
  {
    g_message ("building menus failed: %s", error->message);
    g_error_free (error);
    exit (EXIT_FAILURE);
  }

  build_bookmarks_menu ();

  gtk_window_add_accel_group (GTK_WINDOW (window), accel_group);

  return gtk_ui_manager_get_widget (ui_manager, "/MainMenu");
}


static GtkWidget *
CreateConnectToolbar (GtkWidget * parent)
{
  const GtkTargetEntry possible_types[] = {
    {"STRING", 0, 0},
    {"text/plain", 0, 0},
    {"application/x-rootwin-drop", 0, 1}
  };
  GtkWidget *box, *tempwid;
  gftp_config_list_vars * tmplistvar;
  char *default_protocol, *tempstr;
  int i, j, num;

  box = gtk_toolbar_new ();

  openurl_btn = gtk_tool_button_new_from_stock (GTK_STOCK_NETWORK);
  g_signal_connect (G_OBJECT (openurl_btn), "clicked", G_CALLBACK (tb_openurl_dialog), NULL);
  g_signal_connect (G_OBJECT (openurl_btn), "drag_data_received", G_CALLBACK (openurl_get_drag_data), NULL);
  gtk_drag_dest_set (GTK_WIDGET(openurl_btn), GTK_DEST_DEFAULT_ALL, possible_types, 2, GDK_ACTION_COPY | GDK_ACTION_MOVE);
  gtk_toolbar_insert (GTK_TOOLBAR(box), GTK_TOOL_ITEM(openurl_btn), -1);

  GtkToolItem * item = gtk_tool_item_new ();
  tempwid = gtk_label_new_with_mnemonic (_("_Host: "));
  gtk_container_add (GTK_CONTAINER(item), tempwid);
  gtk_toolbar_insert (GTK_TOOLBAR(box), item, -1);

  item = gtk_tool_item_new ();
  gtk_tool_item_set_expand (item, TRUE);
  hostedit = gtk_combo_box_text_new_with_entry ();
  gtk_widget_set_size_request (hostedit, 130, -1);
  g_signal_connect (G_OBJECT (gtk_bin_get_child (GTK_BIN (hostedit))), "activate", G_CALLBACK (toolbar_hostedit), NULL);
  gftp_lookup_global_option ("hosthistory", &tmplistvar);
  if (tmplistvar->list)
    gtk_combo_box_set_popdown_strings (GTK_COMBO_BOX_TEXT (hostedit), tmplistvar->list);

  gftp_lookup_global_option ("host_value", &tempstr);
  gtk_entry_set_text (GTK_ENTRY (gtk_bin_get_child (GTK_BIN (hostedit))), tempstr);
  gtk_label_set_mnemonic_widget (GTK_LABEL (tempwid), GTK_WIDGET (hostedit));
  gtk_container_add (GTK_CONTAINER(item), hostedit);
  gtk_toolbar_insert (GTK_TOOLBAR(box), item, -1);

  item = gtk_tool_item_new ();
  tempwid = gtk_label_new (_("Port: "));
  gtk_container_add (GTK_CONTAINER(item), tempwid);
  gtk_toolbar_insert (GTK_TOOLBAR(box), item, -1);

  item = gtk_tool_item_new ();
  portedit = gtk_combo_box_text_new_with_entry ();
  gtk_widget_set_size_request (portedit, 50, -1);
  g_signal_connect (G_OBJECT (gtk_bin_get_child (GTK_BIN (portedit))), "activate", G_CALLBACK (toolbar_hostedit), NULL);
  gftp_lookup_global_option ("porthistory", &tmplistvar);
  if (tmplistvar->list)
    gtk_combo_box_set_popdown_strings (GTK_COMBO_BOX_TEXT (portedit), tmplistvar->list);

  gftp_lookup_global_option ("port_value", &tempstr);
  gtk_entry_set_text (GTK_ENTRY (gtk_bin_get_child (GTK_BIN (portedit))), tempstr);
  gtk_container_add (GTK_CONTAINER(item), portedit);
  gtk_toolbar_insert (GTK_TOOLBAR(box), item, -1);

  item = gtk_tool_item_new ();
  tempwid = gtk_label_new_with_mnemonic (_("_User: "));
  gtk_container_add (GTK_CONTAINER(item), tempwid);
  gtk_toolbar_insert (GTK_TOOLBAR(box), item, -1);

  item = gtk_tool_item_new ();
  gtk_tool_item_set_expand (item, TRUE);
  useredit = gtk_combo_box_text_new_with_entry ();
  gtk_widget_set_size_request (useredit, 75, -1);
  g_signal_connect (G_OBJECT (gtk_bin_get_child (GTK_BIN (useredit))), "activate", G_CALLBACK (toolbar_hostedit), NULL);
  gftp_lookup_global_option ("userhistory", &tmplistvar);
  if (tmplistvar->list)
    gtk_combo_box_set_popdown_strings (GTK_COMBO_BOX_TEXT (useredit), tmplistvar->list);

  gftp_lookup_global_option ("user_value", &tempstr);
  gtk_entry_set_text (GTK_ENTRY (gtk_bin_get_child (GTK_BIN (useredit))), tempstr);
  gtk_label_set_mnemonic_widget (GTK_LABEL (tempwid), gtk_bin_get_child (GTK_BIN (useredit)));
  gtk_container_add (GTK_CONTAINER(item), useredit);
  gtk_toolbar_insert (GTK_TOOLBAR(box), item, -1);

  item = gtk_tool_item_new ();
  tempwid = gtk_label_new (_("Pass: "));
  gtk_container_add (GTK_CONTAINER(item), tempwid);
  gtk_toolbar_insert (GTK_TOOLBAR(box), item, -1);

  item = gtk_tool_item_new ();
  gtk_tool_item_set_expand (item, TRUE);
  passedit = gtk_entry_new ();
  gtk_widget_set_size_request (passedit, 55, -1);
  gtk_entry_set_visibility (GTK_ENTRY (passedit), FALSE);
  g_signal_connect (G_OBJECT (passedit), "activate", G_CALLBACK (toolbar_hostedit), NULL);
  gtk_container_add (GTK_CONTAINER(item), passedit);
  gtk_toolbar_insert (GTK_TOOLBAR(box), item, -1);

  item = gtk_tool_item_new ();
  num = 0;
  gftp_lookup_global_option ("default_protocol", &default_protocol);
  optionmenu = gtk_combo_box_text_new ();
  for (i = 0, j = 0; gftp_protocols[i].name; i++)
    {
      if (!gftp_protocols[i].shown)
        continue;

      if (default_protocol != NULL &&
          strcmp (gftp_protocols[i].name, default_protocol) == 0)
        num = j;

      j++;

      gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT(optionmenu), gftp_protocols[i].name);
    }
  gtk_combo_box_set_active (GTK_COMBO_BOX (optionmenu), num);
  gtk_container_add (GTK_CONTAINER(item), optionmenu);
  gtk_toolbar_insert (GTK_TOOLBAR(box), item, -1);

  stop_btn = gtk_tool_button_new_from_stock (GTK_STOCK_STOP);
  gtk_widget_set_sensitive (GTK_WIDGET(stop_btn), 0);
  g_signal_connect_swapped (G_OBJECT (stop_btn), "clicked",
                 G_CALLBACK (stop_button), NULL);
  gtk_toolbar_insert (GTK_TOOLBAR(box), GTK_TOOL_ITEM(stop_btn), -1);

  gtk_widget_grab_focus (gtk_bin_get_child (GTK_BIN (hostedit)));

  return box;
}


static GtkWidget *
CreateCommandToolbar (void)
{
  GtkWidget * box, * tempwid;

  gftpui_command_toolbar = gtk_handle_box_new ();

  box = gtk_hbox_new (FALSE, 4);
  gtk_container_add (GTK_CONTAINER (gftpui_command_toolbar), box);
  gtk_container_set_border_width (GTK_CONTAINER (box), 5);

  tempwid = gtk_label_new (_("Command: "));
  gtk_box_pack_start (GTK_BOX (box), tempwid, FALSE, FALSE, 0);

  gftpui_command_widget = gtk_entry_new ();
  gtk_box_pack_start (GTK_BOX (box), gftpui_command_widget, TRUE, TRUE, 0);
  g_signal_connect (G_OBJECT (gftpui_command_widget), "activate",
              G_CALLBACK (gftpui_run_command), NULL);

  return (gftpui_command_toolbar);
}


static void
setup_column (GtkTreeViewColumn * c, int width)
{
  if (width == 0)
    gtk_tree_view_column_set_expand (c, TRUE);
  else if (width == -1)
    gtk_tree_view_column_set_visible (c, FALSE);
  else
    gtk_tree_view_column_set_fixed_width (c, width);
}


static void
list_doaction (GtkTreeView * view,
    GtkTreePath * path, GtkTreeViewColumn *column, gpointer data)
{
  intptr_t list_dblclk_action;
  gftp_file *tempfle;
  int success;
  char *directory;
  GtkTreeIter iter;
  GtkTreeModel * model;
  gftp_window_data * wdata;

  wdata = data;
  gftp_lookup_request_option (wdata->request, "list_dblclk_action", &list_dblclk_action);
  model = gtk_tree_view_get_model(GTK_TREE_VIEW (wdata->listbox));
  if (gtk_tree_model_get_iter (model, &iter, path))
  {
    gtk_tree_model_get(model, &iter, 0, &tempfle, -1);

    if (check_reconnect (wdata) < 0)
      return;
    if (S_ISLNK (tempfle->st_mode) || S_ISDIR (tempfle->st_mode))
    {
      directory = gftp_build_path (wdata->request, wdata->request->directory,
                                   tempfle->file, NULL);
      success = gftpui_run_chdir (wdata, directory);
      g_free (directory);
    }
    else
      success = 0;
    if (!S_ISDIR (tempfle->st_mode) && !success)
    {
      switch (list_dblclk_action)
        {
          case 0:
            view_dialog (NULL, wdata);
            break;
          case 1:
            edit_dialog (NULL, wdata);
            break;
          case 2:
            if (wdata == &window2)
              get_files (NULL, wdata);
            else
              put_files (NULL, wdata);
            break;
        }
    }
  }
}


static gint
list_enter (GtkWidget * widget, GdkEventKey * event, gpointer data)
{
  gftp_window_data * wdata;

  wdata = data;
  if (GFTP_IS_CONNECTED (wdata->request))
  {
    if (event->type == GDK_KEY_PRESS &&
      (event->keyval == GDK_KEY_KP_Delete || event->keyval == GDK_KEY_Delete))
    {
      delete_dialog (NULL, wdata);
      return TRUE;
    }
  }
  return FALSE;
}


static gint
list_dblclick (GtkWidget * widget, GdkEventButton * event, gpointer data)
{
  GtkWidget * menu;
  gftp_window_data * wdata;

  wdata = data;
  if (event->button == 3)
  {
    if (wdata == &window1)
    {
      menu = gtk_ui_manager_get_widget (ui_manager, "/PLocal");
    }
    else
    {
      menu = gtk_ui_manager_get_widget (ui_manager, "/PRemote");
    }
    gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, event->button, event->time);
    return TRUE;
  }
  return FALSE;
}

void
gftp_gtk_init_request (gftp_window_data * wdata)
{
  wdata->request->logging_function = ftp_log;
  wdata->filespec = g_malloc0 (2);
  *wdata->filespec = '*';
}

static void data_col_pb (GtkTreeViewColumn *tree_column, GtkCellRenderer *cell,
    GtkTreeModel * model, GtkTreeIter *iter, gpointer data)
{
  gftp_file * fle;
  GdkPixbuf * pb;
  size_t stlen;
  gftp_config_list_vars * tmplistvar;
  gftp_file_extensions * tempext;
  GList * templist;

  gtk_tree_model_get(model, iter, 0, &fle, -1);
  pb = NULL;
  if (strcmp (fle->file, "..") == 0)
    pb = gftp_get_pixmap ("dotdot.xpm");
  else if (S_ISLNK (fle->st_mode) && S_ISDIR (fle->st_mode))
    pb = gftp_get_pixmap ("linkdir.xpm");
  else if (S_ISLNK (fle->st_mode))
    pb = gftp_get_pixmap ("linkfile.xpm");
  else if (S_ISDIR (fle->st_mode))
    pb = gftp_get_pixmap ("dir.xpm");
  else if ((fle->st_mode & S_IXUSR) ||
           (fle->st_mode & S_IXGRP) ||
           (fle->st_mode & S_IXOTH))
    pb = gftp_get_pixmap ("exe.xpm");
  else
    {
      stlen = strlen (fle->file);
      gftp_lookup_global_option ("ext", &tmplistvar);
      templist = tmplistvar->list;
      while (templist != NULL)
        {
          tempext = templist->data;
          if (stlen >= tempext->stlen &&
              strcmp (&fle->file[stlen - tempext->stlen], tempext->ext) == 0)
            {
              pb = gftp_get_pixmap (tempext->filename);
              break;
            }
          templist = templist->next;
        }
    }

  if (pb == NULL)
    gftp_get_pixmap ("doc.xpm");

  g_object_set(cell, "pixbuf", pb, NULL);
}

static void data_col_1 (GtkTreeViewColumn *tree_column, GtkCellRenderer *cell,
    GtkTreeModel * model, GtkTreeIter *iter, gpointer data)
{
  gftp_file * fle;

  gtk_tree_model_get(model, iter, 0, &fle, -1);
  if (fle->file != NULL && fle->filename_utf8_encoded)
    g_object_set(cell, "text", fle->file, NULL);
}

static void data_col_2 (GtkTreeViewColumn *tree_column, GtkCellRenderer *cell,
    GtkTreeModel * model, GtkTreeIter *iter, gpointer data)
{
  gftp_file * fle;
  char * tempstr;

  gtk_tree_model_get(model, iter, 0, &fle, -1);
  if (GFTP_IS_SPECIAL_DEVICE (fle->st_mode))
    tempstr = g_strdup_printf ("%d, %d", major (fle->size),
                               minor (fle->size));
  else
    tempstr = insert_commas (fle->size, NULL, 0);

  g_object_set(cell, "text", tempstr, NULL);
  g_free (tempstr);
}

static void data_col_3 (GtkTreeViewColumn *tree_column, GtkCellRenderer *cell,
    GtkTreeModel * model, GtkTreeIter *iter, gpointer data)
{
  gftp_file * fle;

  gtk_tree_model_get(model, iter, 0, &fle, -1);

  if (fle->user)
    g_object_set(cell, "text", fle->user, NULL);
}

static void data_col_4 (GtkTreeViewColumn *tree_column, GtkCellRenderer *cell,
    GtkTreeModel * model, GtkTreeIter *iter, gpointer data)
{
  gftp_file * fle;

  gtk_tree_model_get(model, iter, 0, &fle, -1);
  if (fle->group)
    g_object_set(cell, "text", fle->group, NULL);
}

void data_col_5 (GtkTreeViewColumn *tree_column, GtkCellRenderer *cell,
    GtkTreeModel * model, GtkTreeIter *iter, gpointer data)
{
  gftp_file * fle;
  char * str, * pos;

  gtk_tree_model_get(model, iter, 0, &fle, -1);
  if ((str = ctime (&fle->datetime)))
    {
      if ((pos = strchr (str, '\n')) != NULL)
        *pos = '\0';
      g_object_set(cell, "text", str, NULL);
    }
}

void data_col_6 (GtkTreeViewColumn *tree_column, GtkCellRenderer *cell,
    GtkTreeModel * model, GtkTreeIter *iter, gpointer data)
{
  gftp_file * fle;
  char * attribs;

  gtk_tree_model_get(model, iter, 0, &fle, -1);
  attribs = gftp_convert_attributes_from_mode_t (fle->st_mode);
  g_object_set(cell, "text", attribs, NULL);
  g_free (attribs);
}

static GtkWidget *
CreateFTPWindow (gftp_window_data * wdata)
{
  const GtkTargetEntry possible_types[] = {
    {"STRING", 0, 0},
    {"text/plain", 0, 0},
    {"application/x-rootwin-drop", 0, 1}
  };
  char tempstr[50], *startup_directory;
  GtkWidget *box, *scroll_list, *parent;
  intptr_t listbox_file_height, colwidth;
  GtkCellRenderer * cell;
  GtkTreeViewColumn * c;

  wdata->request = gftp_request_new ();
  gftp_gtk_init_request (wdata);

  parent = gtk_frame_new (NULL);

  gftp_lookup_global_option ("listbox_file_height", &listbox_file_height);
  g_snprintf (tempstr, sizeof (tempstr), "listbox_%s_width", wdata->prefix_col_str);
  gftp_lookup_global_option (tempstr, &colwidth);
  gtk_widget_set_size_request (parent, colwidth, listbox_file_height);

  gtk_container_set_border_width (GTK_CONTAINER (parent), 5);

  box = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (box), 5);
  gtk_container_add (GTK_CONTAINER (parent), box);

  wdata->combo = gtk_combo_box_text_new_with_entry ();
  gtk_box_pack_start (GTK_BOX (box), wdata->combo, FALSE, FALSE, 0);
  g_signal_connect (G_OBJECT (gtk_bin_get_child (GTK_BIN (wdata->combo))),
              "activate", G_CALLBACK (chdir_edit),
              (gpointer) wdata);
  if (*wdata->history)
    gtk_combo_box_set_popdown_strings (GTK_COMBO_BOX_TEXT (wdata->combo), *wdata->history);

  g_snprintf (tempstr, sizeof (tempstr), "%s_startup_directory",
              wdata->prefix_col_str);
  gftp_lookup_global_option (tempstr, &startup_directory);
  gtk_entry_set_text (GTK_ENTRY (gtk_bin_get_child (GTK_BIN (wdata->combo))),
                      startup_directory);

  wdata->hoststxt = gtk_label_new (NULL);
  gtk_misc_set_alignment (GTK_MISC (wdata->hoststxt), 0, 0);
  gtk_box_pack_start (GTK_BOX (box), wdata->hoststxt, FALSE, FALSE, 0);

  scroll_list = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll_list),
                  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

  GtkListStore * l = gtk_list_store_new (1, G_TYPE_POINTER);
  wdata->listbox = gtk_tree_view_new_with_model(GTK_TREE_MODEL(l));

  gtk_container_add (GTK_CONTAINER (scroll_list), wdata->listbox);
  gtk_drag_source_set (wdata->listbox, GDK_BUTTON1_MASK, possible_types, 3,
               GDK_ACTION_COPY | GDK_ACTION_MOVE);
  gtk_drag_dest_set (wdata->listbox, GTK_DEST_DEFAULT_ALL, possible_types, 2,
             GDK_ACTION_COPY | GDK_ACTION_MOVE);

  GtkTreeSelection *select;
  select = gtk_tree_view_get_selection (GTK_TREE_VIEW (wdata->listbox));
  gtk_tree_selection_set_mode (select, GTK_SELECTION_MULTIPLE);

  cell = gtk_cell_renderer_pixbuf_new ();
  gtk_tree_view_insert_column_with_data_func(GTK_TREE_VIEW(wdata->listbox), -1, "", cell, data_col_pb, wdata, NULL);
  c = gtk_tree_view_get_column(GTK_TREE_VIEW(wdata->listbox), 0);
  g_object_set_data(G_OBJECT(c), "col", (gpointer) -1);
  gtk_tree_view_column_set_fixed_width (c, 16);

  cell = gtk_cell_renderer_text_new();
  gtk_tree_view_insert_column_with_data_func(GTK_TREE_VIEW(wdata->listbox), -1, _("Filename"), cell, data_col_1, wdata, NULL);
  g_snprintf (tempstr, sizeof (tempstr), "%s_file_width", wdata->prefix_col_str);
  gftp_lookup_global_option (tempstr, &colwidth);
  c = gtk_tree_view_get_column(GTK_TREE_VIEW(wdata->listbox), 1);
  g_object_set_data(G_OBJECT(c), "col", (gpointer)1);
  gtk_tree_view_column_set_resizable(c, TRUE);
  gtk_tree_view_column_set_clickable(c, TRUE);
  setup_column (c, colwidth);
  g_signal_connect (G_OBJECT (c), "clicked", G_CALLBACK (sortrows), (gpointer) wdata);

  cell = gtk_cell_renderer_text_new();
  gtk_tree_view_insert_column_with_data_func(GTK_TREE_VIEW(wdata->listbox), -1, _("Size"), cell, data_col_2, wdata, NULL);
  g_snprintf (tempstr, sizeof (tempstr), "%s_size_width", wdata->prefix_col_str);
  gftp_lookup_global_option (tempstr, &colwidth);
  c = gtk_tree_view_get_column(GTK_TREE_VIEW(wdata->listbox), 2);
  g_object_set_data(G_OBJECT(c), "col", (gpointer)2);
  gtk_tree_view_column_set_resizable(c, TRUE);
  gtk_tree_view_column_set_clickable(c, TRUE);
  setup_column (c, colwidth);
  g_signal_connect (G_OBJECT (c), "clicked", G_CALLBACK (sortrows), (gpointer) wdata);

  cell = gtk_cell_renderer_text_new();
  gtk_tree_view_insert_column_with_data_func(GTK_TREE_VIEW(wdata->listbox), -1, _("User"), cell, data_col_3, wdata, NULL);
  g_snprintf (tempstr, sizeof (tempstr), "%s_user_width", wdata->prefix_col_str);
  gftp_lookup_global_option (tempstr, &colwidth);
  c = gtk_tree_view_get_column(GTK_TREE_VIEW(wdata->listbox), 3);
  g_object_set_data(G_OBJECT(c), "col", (gpointer)3);
  gtk_tree_view_column_set_resizable(c, TRUE);
  gtk_tree_view_column_set_clickable(c, TRUE);
  setup_column (c, colwidth);
  g_signal_connect (G_OBJECT (c), "clicked", G_CALLBACK (sortrows), (gpointer) wdata);

  cell = gtk_cell_renderer_text_new();
  gtk_tree_view_insert_column_with_data_func(GTK_TREE_VIEW(wdata->listbox), -1, _("Group"), cell, data_col_4, wdata, NULL);
  g_snprintf (tempstr, sizeof (tempstr), "%s_group_width", wdata->prefix_col_str);
  gftp_lookup_global_option (tempstr, &colwidth);
  c = gtk_tree_view_get_column(GTK_TREE_VIEW(wdata->listbox), 4);
  g_object_set_data(G_OBJECT(c), "col", (gpointer)4);
  gtk_tree_view_column_set_resizable(c, TRUE);
  gtk_tree_view_column_set_clickable(c, TRUE);
  setup_column (c, colwidth);
  g_signal_connect (G_OBJECT (c), "clicked", G_CALLBACK (sortrows), (gpointer) wdata);

  cell = gtk_cell_renderer_text_new();
  gtk_tree_view_insert_column_with_data_func(GTK_TREE_VIEW(wdata->listbox), -1, _("Date"), cell, data_col_5, wdata, NULL);
  g_snprintf (tempstr, sizeof (tempstr), "%s_date_width", wdata->prefix_col_str);
  gftp_lookup_global_option (tempstr, &colwidth);
  c = gtk_tree_view_get_column(GTK_TREE_VIEW(wdata->listbox), 5);
  g_object_set_data(G_OBJECT(c), "col", (gpointer)5);
  gtk_tree_view_column_set_resizable(c, TRUE);
  gtk_tree_view_column_set_clickable(c, TRUE);
  setup_column (c, colwidth);
  g_signal_connect (G_OBJECT (c), "clicked", G_CALLBACK (sortrows), (gpointer) wdata);

  cell = gtk_cell_renderer_text_new();
  gtk_tree_view_insert_column_with_data_func(GTK_TREE_VIEW(wdata->listbox), -1, _("Attribs"), cell, data_col_6, wdata, NULL);
  g_snprintf (tempstr, sizeof (tempstr), "%s_attribs_width", wdata->prefix_col_str);
  gftp_lookup_global_option (tempstr, &colwidth);
  c = gtk_tree_view_get_column(GTK_TREE_VIEW(wdata->listbox), 6);
  g_object_set_data(G_OBJECT(c), "col", (gpointer)6);
  gtk_tree_view_column_set_resizable(c, TRUE);
  gtk_tree_view_column_set_clickable(c, TRUE);
  setup_column (c, colwidth);
  g_signal_connect (G_OBJECT (c), "clicked", G_CALLBACK (sortrows), (gpointer) wdata);

  gtk_box_pack_start (GTK_BOX (box), scroll_list, TRUE, TRUE, 0);

  g_signal_connect (G_OBJECT (wdata->listbox), "drag_data_get", G_CALLBACK (listbox_drag), (gpointer) wdata);
  g_signal_connect (G_OBJECT (wdata->listbox), "drag_data_received", G_CALLBACK (listbox_get_drag_data), (gpointer) wdata);

  g_signal_connect (G_OBJECT (wdata->listbox), "key-press-event", G_CALLBACK (list_enter), (gpointer) wdata);
  g_signal_connect_after (G_OBJECT (wdata->listbox), "row-activated", G_CALLBACK (list_doaction), (gpointer) wdata);
  g_signal_connect (G_OBJECT (wdata->listbox), "button-press-event", G_CALLBACK (list_dblclick), (gpointer) wdata);

  return (parent);
}


static gint
menu_mouse_click (GtkWidget * widget, GdkEventButton * event, gpointer data)
{
  GtkWidget * mfactory;

  mfactory = gtk_ui_manager_get_widget (ui_manager, "/PTransfer");
  if (event->button == 3)
    gtk_menu_popup(GTK_MENU(mfactory), NULL,NULL,NULL,NULL, event->button, event->time);
  return (FALSE);
}


static GtkWidget *
CreateFTPWindows (GtkWidget * ui)
{
  GtkWidget *box, *dlbox, *winpane, *dlpane, *logpane, *mainvbox, *tempwid;
  gftp_config_list_vars * tmplistvar;
  intptr_t tmplookup;
  GtkTextBuffer * textbuf;
  GtkTextIter iter;
  GtkTextTag *tag;
  GdkColor * fore;

  memset (&window1, 0, sizeof (window1));
  memset (&window2, 0, sizeof (window2));

  gftp_lookup_global_option ("localhistory", &tmplistvar);
  window1.history = &tmplistvar->list;
  window1.histlen = &tmplistvar->num_items;

  gftp_lookup_global_option ("remotehistory", &tmplistvar);
  window2.history = &tmplistvar->list;
  window2.histlen = &tmplistvar->num_items;

  mainvbox = gtk_vbox_new (FALSE, 0);

  tempwid = CreateMenus (ui);
  gtk_box_pack_start (GTK_BOX (mainvbox), tempwid, FALSE, FALSE, 0);

  tempwid = CreateConnectToolbar (ui);
  gtk_box_pack_start (GTK_BOX (mainvbox), tempwid, FALSE, FALSE, 0);

  tempwid = CreateCommandToolbar ();
  gtk_box_pack_start (GTK_BOX (mainvbox), tempwid, FALSE, FALSE, 0);

  winpane = gtk_hpaned_new ();

  box = gtk_hbox_new (FALSE, 0);

  window1.prefix_col_str = "local";
  local_frame = CreateFTPWindow (&window1);
  gtk_box_pack_start (GTK_BOX (box), local_frame, TRUE, TRUE, 0);

  dlbox = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (dlbox), 5);
  gtk_box_pack_start (GTK_BOX (box), dlbox, FALSE, FALSE, 0);

  tempwid = gtk_image_new_from_stock (GTK_STOCK_GO_FORWARD,
                                      GTK_ICON_SIZE_SMALL_TOOLBAR);

  upload_right_arrow = gtk_button_new ();
  gtk_box_pack_start (GTK_BOX (dlbox), upload_right_arrow, TRUE, FALSE, 0);
  g_signal_connect(G_OBJECT (upload_right_arrow), "clicked", G_CALLBACK (put_files), NULL);
  gtk_container_add (GTK_CONTAINER (upload_right_arrow), tempwid);

  tempwid = gtk_image_new_from_stock (GTK_STOCK_GO_BACK, GTK_ICON_SIZE_SMALL_TOOLBAR);

  download_left_arrow = gtk_button_new ();
  gtk_box_pack_start (GTK_BOX (dlbox), download_left_arrow, TRUE, FALSE, 0);
  g_signal_connect(G_OBJECT (download_left_arrow), "clicked", G_CALLBACK (get_files), NULL);
  gtk_container_add (GTK_CONTAINER (download_left_arrow), tempwid);

  gtk_paned_pack1 (GTK_PANED (winpane), box, 1, 1);

  window2.prefix_col_str = "remote";
  remote_frame = CreateFTPWindow (&window2);

  gtk_paned_pack2 (GTK_PANED (winpane), remote_frame, 1, 1);

  dlpane = gtk_vpaned_new ();
  gtk_paned_pack1 (GTK_PANED (dlpane), winpane, 1, 1);

  transfer_scroll = gtk_scrolled_window_new (NULL, NULL);
  gftp_lookup_global_option ("transfer_height", &tmplookup);
  gtk_widget_set_size_request (transfer_scroll, -1, tmplookup);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (transfer_scroll),
                  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

  GtkTreeStore * l = gtk_tree_store_new (4, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_POINTER, G_TYPE_POINTER, -1);
  dlwdw = gtk_tree_view_new_with_model(GTK_TREE_MODEL(l));

  GtkCellRenderer * cell = gtk_cell_renderer_text_new();
  gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW (dlwdw), -1, _("Filename"), cell, "text", 0, NULL);
  gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW (dlwdw), -1, _("Progress"), cell, "text", 1, NULL);

  GtkTreeSelection * sel = gtk_tree_view_get_selection(GTK_TREE_VIEW (dlwdw));
  gtk_tree_selection_set_mode (sel, GTK_SELECTION_SINGLE);
  gtk_tree_view_set_reorderable (GTK_TREE_VIEW (dlwdw), 0);

  gftp_lookup_global_option ("file_trans_column", &tmplookup);
  GtkTreeViewColumn * col = gtk_tree_view_get_column(GTK_TREE_VIEW (dlwdw), 0);
  gtk_tree_view_column_set_fixed_width (col, tmplookup);

  gtk_container_add (GTK_CONTAINER (transfer_scroll), dlwdw);
  g_signal_connect (G_OBJECT (dlwdw), "button_press_event", G_CALLBACK (menu_mouse_click), NULL);
  gtk_paned_pack2 (GTK_PANED (dlpane), transfer_scroll, 1, 1);

  logpane = gtk_vpaned_new ();
  gtk_paned_pack1 (GTK_PANED (logpane), dlpane, 1, 1);

  log_table = gtk_table_new (1, 2, FALSE);
  gftp_lookup_global_option ("log_height", &tmplookup);
  gtk_widget_set_size_request (log_table, -1, tmplookup);

  logwdw = gtk_text_view_new ();
  gtk_text_view_set_editable (GTK_TEXT_VIEW (logwdw), FALSE);
  gtk_text_view_set_cursor_visible (GTK_TEXT_VIEW (logwdw), FALSE);
  gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (logwdw), GTK_WRAP_WORD);

  textbuf = gtk_text_view_get_buffer (GTK_TEXT_VIEW (logwdw));

  tag = gtk_text_buffer_create_tag (textbuf, "send", NULL);
  gftp_lookup_global_option ("send_color", &fore);
  g_object_set (G_OBJECT (tag), "foreground_gdk", fore, NULL);

  tag = gtk_text_buffer_create_tag (textbuf, "recv", NULL);
  gftp_lookup_global_option ("recv_color", &fore);
  g_object_set (G_OBJECT (tag), "foreground_gdk", fore, NULL);

  tag = gtk_text_buffer_create_tag (textbuf, "error", NULL);
  gftp_lookup_global_option ("error_color", &fore);
  g_object_set (G_OBJECT (tag), "foreground_gdk", fore, NULL);

  tag = gtk_text_buffer_create_tag (textbuf, "misc", NULL);
  gftp_lookup_global_option ("misc_color", &fore);
  g_object_set (G_OBJECT (tag), "foreground_gdk", &fore, NULL);

  tempwid = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (tempwid),
                                 GTK_POLICY_AUTOMATIC,
                                 GTK_POLICY_AUTOMATIC);
  gtk_container_add (GTK_CONTAINER (tempwid), logwdw);
  gtk_table_attach (GTK_TABLE (log_table), tempwid, 0, 1, 0, 1,
            GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND | GTK_SHRINK,
            0, 0);
  logwdw_vadj = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (tempwid));
  gtk_text_buffer_get_iter_at_offset (textbuf, &iter, 0);
  logwdw_textmark = gtk_text_buffer_create_mark (textbuf, "end", &iter, 1);

  gtk_paned_pack2 (GTK_PANED (logpane), log_table, 1, 1);
  gtk_box_pack_start (GTK_BOX (mainvbox), logpane, TRUE, TRUE, 0);

  gtk_widget_show_all (mainvbox);
  gftpui_show_or_hide_command ();
  return (mainvbox);
}

void
toolbar_hostedit (GtkWidget * widget, gpointer data)
{
  int (*init) (gftp_request * request);
  gftp_config_list_vars * tmplistvar;
  const char *txt;
  int num;

  if (current_wdata->request->stopable)
    {
      ftp_log (gftp_logging_error, NULL,
               _("%s: Please hit the stop button first to do anything else\n"),
               _("Connect"));
      return;
    }

  if (GFTP_IS_CONNECTED (current_wdata->request))
    gftp_disconnect (current_wdata->request);

  num = gtk_combo_box_get_active (GTK_COMBO_BOX (optionmenu));

  init = gftp_protocols[num].init;
  if (init (current_wdata->request) < 0)
    return;

  txt = gtk_entry_get_text (GTK_ENTRY (gtk_bin_get_child (GTK_BIN (GTK_COMBO_BOX_TEXT (hostedit)))));
  if (strchr (txt, '/') != NULL)
    {
      /* The user entered a URL in the host box... */
      gftpui_common_cmd_open (current_wdata, current_wdata->request, NULL, NULL, txt);
      return;
    }

  gftp_set_hostname (current_wdata->request, txt);
  if (current_wdata->request->hostname == NULL)
    return;
  alltrim (current_wdata->request->hostname);

  if (current_wdata->request->need_hostport &&
      *current_wdata->request->hostname == '\0')
    {
      ftp_log (gftp_logging_error, NULL,
           _("Error: You must type in a host to connect to\n"));
      return;
    }

  gftp_lookup_global_option ("hosthistory", &tmplistvar);
  add_history (hostedit, &tmplistvar->list, &tmplistvar->num_items,
               current_wdata->request->hostname);

  if (strchr (current_wdata->request->hostname, '/') != NULL &&
      gftp_parse_url (current_wdata->request,
                      current_wdata->request->hostname) == 0)
    {
      ftp_connect (current_wdata, current_wdata->request);
      return;
    }

  txt = gtk_entry_get_text (GTK_ENTRY (gtk_bin_get_child (GTK_BIN (GTK_COMBO_BOX_TEXT (portedit)))));
  gftp_set_port (current_wdata->request, strtol (txt, NULL, 10));

  gftp_lookup_global_option ("porthistory", &tmplistvar);
  add_history (portedit, &tmplistvar->list, &tmplistvar->num_items, txt);

  gftp_set_username (current_wdata->request, gtk_entry_get_text (GTK_ENTRY (gtk_bin_get_child (GTK_BIN (GTK_COMBO_BOX_TEXT (useredit))))));
  if (current_wdata->request->username != NULL)
    alltrim (current_wdata->request->username);


  gftp_lookup_global_option ("userhistory", &tmplistvar);
  add_history (useredit, &tmplistvar->list, &tmplistvar->num_items,
               current_wdata->request->username);

  gftp_set_password (current_wdata->request,
             gtk_entry_get_text (GTK_ENTRY (passedit)));

  gftp_set_directory (current_wdata->request, gtk_entry_get_text (GTK_ENTRY (gtk_bin_get_child (GTK_BIN (GTK_COMBO_BOX_TEXT (current_wdata->combo))))));
  if (current_wdata->request->directory != NULL)
    alltrim (current_wdata->request->directory);

  add_history (current_wdata->combo, current_wdata->history,
               current_wdata->histlen, current_wdata->request->directory);

  ftp_connect (current_wdata, current_wdata->request);
}



void
sortrows (GtkTreeViewColumn * col, gpointer data)
{
  GtkTreeViewColumn * col0;
  gint column;
  char sortcol_name[25], sortasds_name[25];
  intptr_t sortcol, sortasds;
  gftp_window_data * wdata;
  GList * templist;
  int swap_col;
  GtkSortType order;
  GtkTreeModel * m;

  wdata = data;
  g_snprintf (sortcol_name, sizeof (sortcol_name), "%s_sortcol",
              wdata->prefix_col_str);
  gftp_lookup_global_option (sortcol_name, &sortcol);

  g_snprintf (sortasds_name, sizeof (sortasds_name), "%s_sortasds",
              wdata->prefix_col_str);
  gftp_lookup_global_option (sortasds_name, &sortasds);

  col0 = gtk_tree_view_get_column(GTK_TREE_VIEW(wdata->listbox), sortcol);
  gtk_tree_view_column_set_sort_indicator (col0, FALSE);
  if (col == NULL)
  {
    column = sortcol;
    col = col0;
  }
  else
  {
    column = (gint) g_object_get_data(G_OBJECT(col), "col");
  }
  if (column == 0 || (column == sortcol && wdata->sorted))
    {
      sortasds = !sortasds;
      gftp_set_global_option (sortasds_name, GINT_TO_POINTER (sortasds));
      swap_col = 1;
    }
  else
    swap_col = 0;

  if (swap_col || !wdata->sorted)
    {
      if (sortasds)
        order = GTK_SORT_ASCENDING;
      else
        order = GTK_SORT_DESCENDING;
      gtk_tree_view_column_set_sort_indicator (col, TRUE);
      gtk_tree_view_column_set_sort_order (col, order);
    }
  else
    {
      sortcol = column;
      gftp_set_global_option (sortcol_name, GINT_TO_POINTER (sortcol));
    }

  if (!GFTP_IS_CONNECTED (wdata->request))
    return;

  m = gtk_tree_view_get_model(GTK_TREE_VIEW(wdata->listbox));
  gtk_list_store_clear (GTK_LIST_STORE(m));

  wdata->files = gftp_sort_filelist (wdata->files, sortcol, sortasds);

  templist = wdata->files;
  while (templist != NULL)
    {
      add_file_listbox (wdata, templist->data);
      templist = templist->next;
    }

  wdata->sorted = 1;
}

void
stop_button (GtkWidget * widget, gpointer data)
{
  window1.request->cancel = 1;
  window2.request->cancel = 1;
}


static int
gftp_gtk_config_file_read_color (char *str, gftp_config_vars * cv, int line)
{
  char *red, *green, *blue;
  GdkColor * color;

  if (cv->flags & GFTP_CVARS_FLAGS_DYNMEM && cv->value != NULL)
    g_free (cv->value);

  gftp_config_parse_args (str, 3, line, &red, &green, &blue);

  color = g_malloc0 (sizeof (*color));
  color->red = strtol (red, NULL, 16);
  color->green = strtol (green, NULL, 16);
  color->blue = strtol (blue, NULL, 16);

  g_free (red);
  g_free (green);
  g_free (blue);

  cv->value = color;
  cv->flags |= GFTP_CVARS_FLAGS_DYNMEM;

  return (0);
}


static int
gftp_gtk_config_file_write_color (gftp_config_vars * cv, char *buf,
                                  size_t buflen, int to_config_file)
{
  GdkColor * color;

  color = cv->value;
  g_snprintf (buf, buflen, "%x:%x:%x", color->red, color->green, color->blue);
  return (0);
}


static void
_setup_window1 ()
{
  if (gftp_protocols[GFTP_LOCAL_NUM].init (window1.request) == 0)
    {
      gftp_setup_startup_directory (window1.request,
                                    "local_startup_directory");
      gftp_connect (window1.request);
      ftp_list_files (&window1);
    }
}


static void
_setup_window2 (int argc, char **argv)
{
  intptr_t connect_to_remote_on_startup;

  gftp_lookup_request_option (window2.request, "connect_to_remote_on_startup",
                              &connect_to_remote_on_startup);

  if (argc == 2 && strncmp (argv[1], "--", 2) != 0)
    {
      if (gftp_parse_url (window2.request, argv[1]) == 0)
        ftp_connect (&window2, window2.request);
      else
        gftp_usage ();
    }
  else if (connect_to_remote_on_startup)
    {
      if (gftp_protocols[gtk_combo_box_get_active (GTK_COMBO_BOX (optionmenu))].init (current_wdata->request) == 0)
        {
          gftp_setup_startup_directory (window2.request,
                                        "remote_startup_directory");
          gftp_connect (window2.request);
          ftp_list_files (&window2);
        }
    }
  else
    {
      /* On the remote window, even though we aren't connected, draw the sort
         icon on that side */
      sortrows (NULL, &window2);
    }
}


int
main (int argc, char **argv)
{
  GtkWidget *window, *ui;
  gftp_graphic * gftp_icon;

  /* We override the read color functions because we are using a GdkColor
     structures to store the color. If I put this in lib/config_file.c, then
     the core library would be dependant on Gtk+ being present */
  gftp_option_types[gftp_option_type_color].read_function = gftp_gtk_config_file_read_color;
  gftp_option_types[gftp_option_type_color].write_function = gftp_gtk_config_file_write_color;

  gftpui_common_init (&argc, &argv, ftp_log);
  gftpui_common_child_process_done = 0;

  g_thread_init (NULL);
  gdk_threads_init();
  gdk_threads_enter ();
  gtk_init (&argc, &argv);

  graphic_hash_table = g_hash_table_new (string_hash_function,
                                         string_hash_compare);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  g_signal_connect (G_OBJECT (window), "delete_event",
              G_CALLBACK (_gftp_try_close), NULL);
  g_signal_connect (G_OBJECT (window), "destroy",
              G_CALLBACK (_gftp_force_close), NULL);
  gtk_window_set_title (GTK_WINDOW (window), gftp_version);
  gtk_widget_set_name (window, gftp_version);
  gtk_widget_realize (window);
  gftp_icon = open_xpm ("gftp.xpm");
  if (gftp_icon != NULL)
    {
      gtk_window_set_default_icon (gftp_icon->pb);
    }

  other_wdata = &window1;
  current_wdata = &window2;
  ui = CreateFTPWindows (window);
  gtk_container_add (GTK_CONTAINER (window), ui);
  gtk_widget_show (window);

  gftpui_common_about (ftp_log, NULL);

  g_timeout_add (1000, update_downloads, NULL);

  _setup_window1 ();
  _setup_window2 (argc, argv);

  gtk_main ();
  gdk_threads_leave ();

  return (0);
}


void
gftpui_show_or_hide_command (void)
{
  intptr_t cmd_in_gui;

  gftp_lookup_global_option ("cmd_in_gui", &cmd_in_gui);
  if (cmd_in_gui)
    gtk_widget_show (gftpui_command_toolbar);
  else
    gtk_widget_hide (gftpui_command_toolbar);
}
