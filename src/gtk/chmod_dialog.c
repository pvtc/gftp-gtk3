/*****************************************************************************/
/*  chmod_dialog.c - the chmod dialog box                                    */
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

static GtkWidget *suid, *sgid, *sticky, *ur, *uw, *ux, *gr, *gw, *gx, *or, *ow,
                 *ox;
static mode_t mode;


static int
do_chmod_thread (gftpui_callback_data * cdata)
{
  GList * filelist, * templist;
  gftp_window_data * wdata;
  gftp_file * tempfle;
  int error;
  GtkTreeSelection *select;
  GtkTreeIter iter;
  GtkTreeModel * model;

  wdata = cdata->uidata;
  error = 0;

  select = gtk_tree_view_get_selection (GTK_TREE_VIEW (wdata->listbox));
  templist = gtk_tree_selection_get_selected_rows(select, &model);
  for (filelist = templist ; filelist != NULL; filelist = g_list_next(filelist))
  {
    gtk_tree_model_get_iter(model, &iter, (GtkTreePath*)filelist->data);
    gtk_tree_model_get(model, &iter, 0, &tempfle, -1);

      if (gftp_chmod (wdata->request, tempfle->file, mode) != 0)
        error = 1;

      if (!GFTP_IS_CONNECTED (wdata->request))
        break;
    }
  g_list_foreach (templist, (GFunc) gtk_tree_path_free, NULL);
  g_list_free (templist);

  return (error);
}


static void
dochmod (gftp_window_data * wdata)
{
  gftpui_callback_data * cdata;

  mode = 0;
  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON (suid)))
    mode |= S_ISUID;
  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON (sgid)))
    mode |= S_ISGID;
  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON (sticky)))
    mode |= S_ISVTX;

  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON (ur)))
    mode |= S_IRUSR;
  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON (uw)))
    mode |= S_IWUSR;
  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON (ux)))
    mode |= S_IXUSR;

  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON (gr)))
    mode |= S_IRGRP;
  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON (gw)))
    mode |= S_IWGRP;
  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON (gx)))
    mode |= S_IXGRP;

  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON (or)))
    mode |= S_IROTH;
  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON (ow)))
    mode |= S_IWOTH;
  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON (ox)))
    mode |= S_IXOTH;

  if (check_reconnect (wdata) < 0)
    return;

  cdata = g_malloc0 (sizeof (*cdata));
  cdata->request = wdata->request;
  cdata->uidata = wdata;
  cdata->run_function = do_chmod_thread;

  gftpui_common_run_callback_function (cdata);

  g_free (cdata);
}

void
chmod_dialog (GtkAction * a, gpointer data)
{
  GtkWidget *tempwid, *dialog, *hbox, *vbox;
  GList * templist;
  gftp_window_data * wdata;
  gftp_file * tempfle;

  wdata = data;
  if (!check_status (_("Chmod"), wdata, gftpui_common_use_threads (wdata->request), 0, 1, wdata->request->chmod != NULL))
    return;

  dialog = gtk_dialog_new_with_buttons (_("Chmod"), GTK_WINDOW(window), 0,
                                        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                        GTK_STOCK_OK, GTK_RESPONSE_OK,
                                        NULL);
  gtk_box_set_spacing (GTK_BOX (gtk_dialog_get_content_area(GTK_DIALOG (dialog))), 5);
  gtk_container_set_border_width (GTK_CONTAINER (gtk_dialog_get_content_area(GTK_DIALOG (dialog))), 10);

  tempwid = gtk_label_new (_("You can now adjust the attributes of your file(s)\nNote: Not all ftp servers support the chmod feature"));
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area(GTK_DIALOG (dialog))), tempwid, FALSE,
              FALSE, 0);
  gtk_widget_show (tempwid);

  hbox = gtk_hbox_new (TRUE, 5);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area(GTK_DIALOG (dialog))), hbox, FALSE, FALSE,
              0);
  gtk_widget_show (hbox);

  tempwid = gtk_frame_new (_("Special"));
  gtk_box_pack_start (GTK_BOX (hbox), tempwid, FALSE, FALSE, 0);
  gtk_widget_show (tempwid);

  vbox = gtk_vbox_new (TRUE, 5);
  gtk_container_add (GTK_CONTAINER (tempwid), vbox);
  gtk_widget_show (vbox);

  suid = gtk_check_button_new_with_label (_("SUID"));
  gtk_box_pack_start (GTK_BOX (vbox), suid, FALSE, FALSE, 0);
  gtk_widget_show (suid);

  sgid = gtk_check_button_new_with_label (_("SGID"));
  gtk_box_pack_start (GTK_BOX (vbox), sgid, FALSE, FALSE, 0);
  gtk_widget_show (sgid);

  sticky = gtk_check_button_new_with_label (_("Sticky"));
  gtk_box_pack_start (GTK_BOX (vbox), sticky, FALSE, FALSE, 0);
  gtk_widget_show (sticky);

  tempwid = gtk_frame_new (_("User"));
  gtk_box_pack_start (GTK_BOX (hbox), tempwid, FALSE, FALSE, 0);
  gtk_widget_show (tempwid);

  vbox = gtk_vbox_new (TRUE, 5);
  gtk_container_add (GTK_CONTAINER (tempwid), vbox);
  gtk_widget_show (vbox);

  ur = gtk_check_button_new_with_label (_("Read"));
  gtk_box_pack_start (GTK_BOX (vbox), ur, FALSE, FALSE, 0);
  gtk_widget_show (ur);

  uw = gtk_check_button_new_with_label (_("Write"));
  gtk_box_pack_start (GTK_BOX (vbox), uw, FALSE, FALSE, 0);
  gtk_widget_show (uw);

  ux = gtk_check_button_new_with_label (_("Execute"));
  gtk_box_pack_start (GTK_BOX (vbox), ux, FALSE, FALSE, 0);
  gtk_widget_show (ux);

  tempwid = gtk_frame_new (_("Group"));
  gtk_box_pack_start (GTK_BOX (hbox), tempwid, FALSE, FALSE, 0);
  gtk_widget_show (tempwid);

  vbox = gtk_vbox_new (TRUE, 5);
  gtk_container_add (GTK_CONTAINER (tempwid), vbox);
  gtk_widget_show (vbox);

  gr = gtk_check_button_new_with_label (_("Read"));
  gtk_box_pack_start (GTK_BOX (vbox), gr, FALSE, FALSE, 0);
  gtk_widget_show (gr);

  gw = gtk_check_button_new_with_label (_("Write"));
  gtk_box_pack_start (GTK_BOX (vbox), gw, FALSE, FALSE, 0);
  gtk_widget_show (gw);

  gx = gtk_check_button_new_with_label (_("Execute"));
  gtk_box_pack_start (GTK_BOX (vbox), gx, FALSE, FALSE, 0);
  gtk_widget_show (gx);

  tempwid = gtk_frame_new (_("Other"));
  gtk_box_pack_start (GTK_BOX (hbox), tempwid, FALSE, FALSE, 0);
  gtk_widget_show (tempwid);

  vbox = gtk_vbox_new (TRUE, 5);
  gtk_container_add (GTK_CONTAINER (tempwid), vbox);
  gtk_widget_show (vbox);

  or = gtk_check_button_new_with_label (_("Read"));
  gtk_box_pack_start (GTK_BOX (vbox), or, FALSE, FALSE, 0);
  gtk_widget_show (or);

  ow = gtk_check_button_new_with_label (_("Write"));
  gtk_box_pack_start (GTK_BOX (vbox), ow, FALSE, FALSE, 0);
  gtk_widget_show (ow);

  ox = gtk_check_button_new_with_label (_("Execute"));
  gtk_box_pack_start (GTK_BOX (vbox), ox, FALSE, FALSE, 0);
  gtk_widget_show (ox);

  GtkTreeSelection *select;
  GtkTreeIter iter;
  GtkTreeModel * model;
  select = gtk_tree_view_get_selection (GTK_TREE_VIEW (wdata->listbox));
  if (gtk_tree_selection_count_selected_rows(select) == 1)
  {
    templist = gtk_tree_selection_get_selected_rows(select, &model);
    gtk_tree_model_get_iter(model, &iter, (GtkTreePath*)templist->data);
    gtk_tree_model_get(model, &iter, 0, &tempfle, -1);
    g_list_foreach (templist, (GFunc) gtk_tree_path_free, NULL);
    g_list_free (templist);

      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (suid),
                                    tempfle->st_mode & S_ISUID);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ur),
                                    tempfle->st_mode & S_IRUSR);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (uw),
                                    tempfle->st_mode & S_IWUSR);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ux),
                                    tempfle->st_mode & S_IXUSR);

      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (sgid),
                                    tempfle->st_mode & S_ISGID);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (gr),
                                    tempfle->st_mode & S_IRGRP);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (gw),
                                    tempfle->st_mode & S_IWGRP);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (gx),
                                    tempfle->st_mode & S_IXGRP);

      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (sticky),
                                    tempfle->st_mode & S_ISVTX);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (or),
                                    tempfle->st_mode & S_IROTH);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ow),
                                    tempfle->st_mode & S_IWOTH);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ox),
                                    tempfle->st_mode & S_IXOTH);
    }
  gtk_dialog_set_default_response (GTK_DIALOG(dialog), GTK_RESPONSE_OK);
  gint response = gtk_dialog_run (GTK_DIALOG(dialog));
  if (response == GTK_RESPONSE_OK)
  {
     dochmod (wdata);
  }
  gtk_widget_destroy (dialog);
}
