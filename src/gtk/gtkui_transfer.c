/*****************************************************************************/
/*  gtkui_transfer.c - GTK+ UI transfer related functions for gFTP           */
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

void
gftpui_start_current_file_in_transfer (gftp_transfer * tdata)
{
}


void
gftpui_update_current_file_in_transfer (gftp_transfer * tdata)
{
}


void
gftpui_finish_current_file_in_transfer (gftp_transfer * tdata)
{
}


void
gftpui_start_transfer (gftp_transfer * tdata)
{
  /* Not used in GTK+ port. This is polled instead */
}

static void data_col_0 (GtkTreeViewColumn *tree_column, GtkCellRenderer *cell,
    GtkTreeModel * model, GtkTreeIter *iter, gpointer data)
{
  gftp_file * fle;
  char * text;
  size_t len;
  gftp_transfer * tdata;

  tdata = (gftp_transfer *)data;
  gtk_tree_model_get(model, iter, 0, &fle, -1);

  len = strlen (tdata->toreq->directory);
  text = fle->destfile;
  if (len == 1 && (*tdata->toreq->directory) == '/')
    text++;
  if (strncmp (text, tdata->toreq->directory, len) == 0)
    text += len + 1;

  g_object_set(cell, "text", text, NULL);
}

static void data_col_1 (GtkTreeViewColumn *tree_column, GtkCellRenderer *cell,
    GtkTreeModel * model, GtkTreeIter *iter, gpointer data)
{
  gftp_file * fle;
  char temp[50];
  char * text;

  gtk_tree_model_get(model, iter, 0, &fle, -1);
  text = insert_commas (fle->size, temp, sizeof (temp));
  g_object_set(cell, "text", text, NULL);
}

static void data_col_2 (GtkTreeViewColumn *tree_column, GtkCellRenderer *cell,
    GtkTreeModel * model, GtkTreeIter *iter, gpointer data)
{
  gftp_file * fle;
  char temp[50];
  char * text;

  gtk_tree_model_get(model, iter, 0, &fle, -1);
  text = insert_commas (fle->startsize, temp, sizeof (temp));
  g_object_set(cell, "text", text, NULL);
}

static void data_col_3 (GtkTreeViewColumn *tree_column, GtkCellRenderer *cell,
    GtkTreeModel * model, GtkTreeIter *iter, gpointer data)
{
  gftp_file * fle;
  char * text;
  gtk_tree_model_get(model, iter, 0, &fle, -1);
  switch (fle->transfer_action)
  {
  case GFTP_TRANS_ACTION_OVERWRITE:
    text = _("Overwrite");
    break;
  case GFTP_TRANS_ACTION_SKIP:
    text = _("Skip");
    break;
  case GFTP_TRANS_ACTION_RESUME:
    text = _("Resume");
    break;
  default:
    text = _("Error");
    break;
  }
  g_object_set(cell, "text", text, NULL);
}


void
gftpui_add_file_to_transfer (gftp_transfer * tdata, GList * curfle)
{
  char * status;
  gftp_file * fle;
  GtkTreeIter iter;

  g_print("%s\n", "gftpui_common_add_file_transfer");

  fle = curfle->data;
  if (fle->transfer_action == GFTP_TRANS_ACTION_SKIP)
    status = _("Skipped");
  else
    status = _("Waiting...");

  g_print("%s\n", gftpui_gtk_get_utf8_file_pos (fle));

  GtkTreeModel * model = gtk_tree_view_get_model(GTK_TREE_VIEW (dlwdw));
  gtk_tree_store_append (GTK_TREE_STORE(model), &iter, tdata->user_data);
  gtk_tree_store_set(GTK_TREE_STORE(model), &iter,
    0, gftpui_gtk_get_utf8_file_pos (fle),
    1, status,
    2, tdata,
    3, curfle,
    -1);
}


void
gftpui_cancel_file_transfer (gftp_transfer * tdata)
{
  if (tdata->thread_id != NULL)
    pthread_kill (*(pthread_t *) tdata->thread_id, SIGINT);

  tdata->cancel = 1; /* FIXME */
  tdata->fromreq->cancel = 1;
  tdata->toreq->cancel = 1;
}


static void
gftpui_gtk_trans_selectall (GtkWidget * widget, gpointer data)
{
  gftp_transfer * tdata;
  tdata = data;

  GtkTreeSelection * select = gtk_tree_view_get_selection (GTK_TREE_VIEW(tdata->clist));
  gtk_tree_selection_select_all (select);
}


static void
gftpui_gtk_trans_unselectall (GtkWidget * widget, gpointer data)
{
  gftp_transfer * tdata;
  tdata = data;

  GtkTreeSelection * select = gtk_tree_view_get_selection (GTK_TREE_VIEW(tdata->clist));
  gtk_tree_selection_unselect_all (select);
}

static void
gftpui_gtk_set_action (gftp_transfer * tdata, int transfer_action)
{
  GList * templist, * filelist;
  gftp_file * tempfle;

  g_static_mutex_lock (&tdata->structmutex);

  GtkTreeSelection *select;
  GtkTreeIter iter;
  GtkTreeModel * model;
  select = gtk_tree_view_get_selection (GTK_TREE_VIEW (tdata->clist));
  templist = gtk_tree_selection_get_selected_rows(select, &model);
  for (filelist = templist ; filelist != NULL; filelist = g_list_next(filelist))
  {
    gtk_tree_model_get_iter(model, &iter, (GtkTreePath*)filelist->data);
    gtk_tree_model_get(model, &iter, 0, &tempfle, -1);
    tempfle->transfer_action = transfer_action;
  }
  g_list_foreach (templist, (GFunc) gtk_tree_path_free, NULL);
  g_list_free (templist);

  g_static_mutex_unlock (&tdata->structmutex);
}

static void
gftpui_gtk_overwrite (GtkWidget * widget, gpointer data)
{
  gftpui_gtk_set_action (data, GFTP_TRANS_ACTION_OVERWRITE);
}

static void
gftpui_gtk_resume (GtkWidget * widget, gpointer data)
{
  gftpui_gtk_set_action (data, GFTP_TRANS_ACTION_RESUME);
}

static void
gftpui_gtk_skip (GtkWidget * widget, gpointer data)
{
  gftpui_gtk_set_action (data, GFTP_TRANS_ACTION_SKIP);
}

static void
gftpui_gtk_ok (GtkWidget * widget, gpointer data)
{
  gftp_transfer * tdata;
  gftp_file * tempfle;
  GList * templist;

  tdata = data;
  g_static_mutex_lock (&tdata->structmutex);
  for (templist = tdata->files; templist != NULL; templist = templist->next)
    {
      tempfle = templist->data;
      if (tempfle->transfer_action != GFTP_TRANS_ACTION_SKIP)
        break;
    }

  tdata->ready = 1;
  if (templist == NULL)
    {
      tdata->show = 0;
      tdata->done = 1;
    }
  else
    tdata->show = 1;

  g_static_mutex_unlock (&tdata->structmutex);
}


static void
gftpui_gtk_cancel (GtkWidget * widget, gpointer data)
{
  gftp_transfer * tdata;

  tdata = data;
  g_static_mutex_lock (&tdata->structmutex);
  tdata->show = 0;
  tdata->done = tdata->ready = 1;
  g_static_mutex_unlock (&tdata->structmutex);
}

static void
gftpui_gtk_transfer_action (GtkWidget * widget, gint response,
                            gpointer user_data)
{
  switch (response)
    {
      case GTK_RESPONSE_OK:
        gftpui_gtk_ok (widget, user_data);
        gtk_widget_destroy (widget);
        break;
      case GTK_RESPONSE_CANCEL:
        gftpui_gtk_cancel (widget, user_data);
        /* no break */
      default:
        gtk_widget_destroy (widget);
    }
}

void
gftpui_ask_transfer (gftp_transfer * tdata)
{
  GtkWidget * dialog, * tempwid, * scroll, * hbox;
  gftp_file * tempfle;
  GList * templist;
  GtkTreeIter iter;

  dialog = gtk_dialog_new_with_buttons (_("Transfer Files"), NULL, 0,
                                        GTK_STOCK_CANCEL,
                                        GTK_RESPONSE_CANCEL,
                                        GTK_STOCK_OK,
                                        GTK_RESPONSE_OK,
                                        NULL);
  gtk_container_set_border_width (GTK_CONTAINER (gtk_dialog_get_content_area(GTK_DIALOG (dialog))), 10);
  gtk_box_set_spacing (GTK_BOX (gtk_dialog_get_content_area(GTK_DIALOG (dialog))), 5);

  tempwid = gtk_label_new (_("The following file(s) exist on both the local and remote computer\nPlease select what you would like to do"));
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area(GTK_DIALOG (dialog))), tempwid, FALSE,
              FALSE, 0);
  gtk_widget_show (tempwid);

  scroll = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_set_size_request (scroll, 450, 200);

  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
                  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

  GtkListStore * l = gtk_list_store_new (1, G_TYPE_POINTER);
  tdata->clist = gtk_tree_view_new_with_model(GTK_TREE_MODEL(l));
  GtkCellRenderer * cell = gtk_cell_renderer_text_new();
  gtk_tree_view_insert_column_with_data_func(GTK_TREE_VIEW(tdata->clist), -1, _("Filename"), cell, data_col_0, tdata, NULL);
  gtk_tree_view_insert_column_with_data_func(GTK_TREE_VIEW(tdata->clist), -1, tdata->fromreq->hostname, cell, data_col_1, NULL, NULL);
  gtk_tree_view_insert_column_with_data_func(GTK_TREE_VIEW(tdata->clist), -1, tdata->toreq->hostname, cell, data_col_2, NULL, NULL);
  gtk_tree_view_insert_column_with_data_func(GTK_TREE_VIEW(tdata->clist), -1, _("Action"), cell, data_col_3, NULL, NULL);

  gtk_container_add (GTK_CONTAINER (scroll), tdata->clist);
  GtkTreeSelection * select = gtk_tree_view_get_selection (GTK_TREE_VIEW(tdata->clist));
  gtk_tree_selection_set_mode (select, GTK_SELECTION_MULTIPLE);
  GtkTreeViewColumn * c;
  c = gtk_tree_view_get_column(GTK_TREE_VIEW(tdata->clist), 0);
  gtk_tree_view_column_set_fixed_width(c, 100);
  c = gtk_tree_view_get_column(GTK_TREE_VIEW(tdata->clist), 1);
  gtk_tree_view_column_set_fixed_width(c, 85);
  c = gtk_tree_view_get_column(GTK_TREE_VIEW(tdata->clist), 2);
  gtk_tree_view_column_set_fixed_width(c, 85);
  c = gtk_tree_view_get_column(GTK_TREE_VIEW(tdata->clist), 3);
  gtk_tree_view_column_set_fixed_width(c, 85);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area(GTK_DIALOG (dialog))), scroll, TRUE, TRUE, 0);
  gtk_widget_show (tdata->clist);
  gtk_widget_show (scroll);

  for (templist = tdata->files; templist != NULL;
       templist = templist->next)
    {
      tempfle = templist->data;
      if (tempfle->startsize == 0 || S_ISDIR (tempfle->st_mode))
        {
           tempfle->shown = 0;
           continue;
        }
      tempfle->shown = 1;
      gftp_get_transfer_action (tdata->fromreq, tempfle);
      gtk_list_store_append (l, &iter);
      gtk_list_store_set (l, &iter, 0, tempfle, -1);
    }

  gtk_tree_selection_select_all (select);

  hbox = gtk_hbox_new (TRUE, 20);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area(GTK_DIALOG (dialog))), hbox, TRUE, TRUE, 0);
  gtk_widget_show (hbox);

  tempwid = gtk_button_new_with_label (_("Overwrite"));
  gtk_box_pack_start (GTK_BOX (hbox), tempwid, TRUE, TRUE, 0);
  g_signal_connect (G_OBJECT (tempwid), "clicked",
              G_CALLBACK (gftpui_gtk_overwrite), (gpointer) tdata);
  gtk_widget_show (tempwid);

  tempwid = gtk_button_new_with_label (_("Resume"));
  gtk_box_pack_start (GTK_BOX (hbox), tempwid, TRUE, TRUE, 0);
  g_signal_connect (G_OBJECT (tempwid), "clicked",
              G_CALLBACK (gftpui_gtk_resume), (gpointer) tdata);
  gtk_widget_show (tempwid);

  tempwid = gtk_button_new_with_label (_("Skip File"));
  gtk_box_pack_start (GTK_BOX (hbox), tempwid, TRUE, TRUE, 0);
  g_signal_connect (G_OBJECT (tempwid), "clicked",
                      G_CALLBACK (gftpui_gtk_skip), (gpointer) tdata);
  gtk_widget_show (tempwid);

  hbox = gtk_hbox_new (TRUE, 20);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area(GTK_DIALOG (dialog))), hbox, TRUE, TRUE, 0);
  gtk_widget_show (hbox);

  tempwid = gtk_button_new_with_label (_("Select All"));
  gtk_box_pack_start (GTK_BOX (hbox), tempwid, TRUE, TRUE, 0);
  g_signal_connect (G_OBJECT (tempwid), "clicked",
              G_CALLBACK (gftpui_gtk_trans_selectall), (gpointer) tdata);
  gtk_widget_show (tempwid);

  tempwid = gtk_button_new_with_label (_("Deselect All"));
  gtk_box_pack_start (GTK_BOX (hbox), tempwid, TRUE, TRUE, 0);
  g_signal_connect (G_OBJECT (tempwid), "clicked",
              G_CALLBACK (gftpui_gtk_trans_unselectall), (gpointer) tdata);
  gtk_widget_show (tempwid);

  g_signal_connect (G_OBJECT (dialog), "response",
                    G_CALLBACK (gftpui_gtk_transfer_action),(gpointer) tdata);

  gtk_widget_show (dialog);
  dialog = NULL;
}
