/*****************************************************************************/
/*  menu-items.c - menu callbacks                                            */
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
/*  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.                */
/*****************************************************************************/

#include "gftp-gtk.h"

static void
update_window_listbox (gftp_window_data * wdata)
{
  GList * templist, * filelist;
  gftp_file * tempfle;

  GtkTreeSelection *select;
  GtkTreeIter iter;
  GtkTreeModel * model;
  select = gtk_tree_view_get_selection (GTK_TREE_VIEW (wdata->listbox));
  templist = gtk_tree_selection_get_selected_rows(select, &model);
  for (filelist = templist ; filelist != NULL; filelist = g_list_next(filelist))
  {
    gtk_tree_model_get_iter(model, &iter, (GtkTreePath*)filelist->data);
    gtk_tree_model_get(model, &iter, 0, &tempfle, -1);
    tempfle->was_sel = 1;
  }
  g_list_foreach (templist, (GFunc) gtk_tree_path_free, NULL);
  g_list_free (templist);

  gtk_list_store_clear (GTK_LIST_STORE (model));
  templist = wdata->files;
  while (templist != NULL)
  {
    tempfle = templist->data;
    add_file_listbox (wdata, tempfle);
    templist = templist->next;
  }
  update_window (wdata);
}


static void
dochange_filespec (gftp_window_data * wdata, gftp_dialog_data * ddata)
{
  const char *edttext;

  wdata->show_selected = 0;

  edttext = gtk_entry_get_text (GTK_ENTRY (ddata->edit));
  if (*edttext == '\0')
    {
      ftp_log (gftp_logging_error, NULL,
               _("Change Filespec: Operation canceled...you must enter a string\n"));
      return;
    }

  if (wdata->filespec)
    g_free (wdata->filespec);

  wdata->filespec = g_strdup (edttext);
  update_window_listbox (wdata);
}


void
change_filespec (GtkAction * a, gpointer data)
{
  gftp_window_data * wdata;

  wdata = data;
  if (!check_status (_("Change Filespec"), wdata, 0, 0, 0, 1))
    return;

  MakeEditDialog (_("Change Filespec"), _("Enter the new file specification"),
                  wdata->filespec, 1, NULL, gftp_dialog_button_change,
                  dochange_filespec, wdata, NULL, NULL);
}

static void
dosave_directory_listing (const char *filename, GList * templist)
{
  gftp_file * tempfle;
  char *tempstr;
  FILE * fd;

  if ((fd = fopen (filename, "w")) == NULL)
    {
      ftp_log (gftp_logging_error, NULL,
               _("Error: Cannot open %s for writing: %s\n"), filename,
               g_strerror (errno));
      return;
    }

  while (templist != NULL)
    {
      tempfle = templist->data;

      if (!tempfle->shown)
        continue;

      tempstr = gftp_gen_ls_string (NULL, tempfle, NULL, NULL);
      fprintf (fd, "%s\n", tempstr);
      g_free (tempstr);

      templist = templist->next;
    }

  fclose (fd);
}


void
save_directory_listing (GtkAction * a, gpointer data)
{
   GtkWidget *filew;
  filew = gtk_file_chooser_dialog_new (_("Save Directory Listing"),
    GTK_WINDOW(window),
    GTK_FILE_CHOOSER_ACTION_SAVE,
    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
    GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
    NULL);
  gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (filew), TRUE);
  if (gtk_dialog_run (GTK_DIALOG (filew)) == GTK_RESPONSE_ACCEPT)
  {
    char *filename;
    filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (filew));
    dosave_directory_listing (filename, ((gftp_window_data * )data)->files);
    g_free (filename);
  }
  gtk_widget_destroy (filew);
}


void
show_selected (GtkAction * a, gpointer data)
{
  gftp_window_data * wdata;

  wdata = data;
  wdata->show_selected = 1;
  update_window_listbox (wdata);
}


void
selectall (GtkAction * a, gpointer data)
{
  gftp_window_data * wdata;

  wdata = data;
  wdata->show_selected = 0;
  GtkTreeSelection * select = gtk_tree_view_get_selection (GTK_TREE_VIEW(wdata->listbox));
  gtk_tree_selection_select_all (select);
}

void
selectallfiles (GtkAction * a, gpointer data)
{
  gftp_window_data * wdata;
  gftp_file * tempfle;
  GtkTreeModel * list_store;
  GtkTreeIter iter;
  gboolean valid;

  wdata = data;
  wdata->show_selected = 0;
  GtkTreeSelection * select = gtk_tree_view_get_selection (GTK_TREE_VIEW(wdata->listbox));
  list_store = gtk_tree_view_get_model(GTK_TREE_VIEW(wdata->listbox));
  valid = gtk_tree_model_get_iter_first (list_store, &iter);
  while (valid)
  {
    gtk_tree_model_get (list_store, &iter, 0, &tempfle, -1);

    if (tempfle->shown)
    {
      if (S_ISDIR (tempfle->st_mode))
        gtk_tree_selection_unselect_iter (select, &iter);
      else
        gtk_tree_selection_select_iter (select, &iter);
    }
    valid = gtk_tree_model_iter_next (list_store, &iter);
  }
}


void
deselectall (GtkAction * a, gpointer data)
{
  gftp_window_data * wdata;

  wdata = data;
  wdata->show_selected = 0;

  GtkTreeSelection * select = gtk_tree_view_get_selection (GTK_TREE_VIEW(wdata->listbox));
  gtk_tree_selection_unselect_all (select);
}


int
chdir_edit (GtkWidget * widget, gpointer data)
{
  gftp_window_data * wdata;
  const char *edttxt;
  char *tempstr;

  wdata = data;
  if (!check_status (_("Chdir"), wdata, gftpui_common_use_threads (wdata->request), 0, 0,
                     wdata->request->chdir != NULL))
    return (0);

  if (check_reconnect (wdata) < 0)
    return (0);

  edttxt = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT (wdata->combo));

  if (!GFTP_IS_CONNECTED (wdata->request) && *edttxt != '\0')
    {
      toolbar_hostedit (NULL, NULL);
      return (0);
    }

  if ((tempstr = gftp_expand_path (wdata->request, edttxt)) == NULL)
    return (FALSE);

  if (gftpui_run_chdir (wdata, tempstr))
    add_history (wdata->combo, wdata->history, wdata->histlen, edttxt);

  g_free (tempstr);
  return (0);
}


void
clearlog (GtkAction * a, gpointer data)
{
  gint len;

  GtkTextBuffer * textbuf;
  GtkTextIter iter, iter2;

  textbuf = gtk_text_view_get_buffer (GTK_TEXT_VIEW (logwdw));
  len = gtk_text_buffer_get_char_count (textbuf);
  gtk_text_buffer_get_iter_at_offset (textbuf, &iter, 0);
  gtk_text_buffer_get_iter_at_offset (textbuf, &iter2, len);
  gtk_text_buffer_delete (textbuf, &iter, &iter2);
}


void
viewlog (GtkAction * a, gpointer data)
{
  char *tempstr, *txt, *pos;
  gint textlen;
  ssize_t len;
  int fd;
  GtkTextBuffer * textbuf;
  GtkTextIter iter, iter2;

  tempstr = g_strconcat (g_get_tmp_dir (), "/gftp-view.XXXXXXXXXX", NULL);
  if ((fd = mkstemp (tempstr)) < 0)
    {
      ftp_log (gftp_logging_error, NULL,
               _("Error: Cannot open %s for writing: %s\n"), tempstr,
               g_strerror (errno));
      g_free (tempstr);
      return;
    }
  chmod (tempstr, S_IRUSR | S_IWUSR);

  textbuf = gtk_text_view_get_buffer (GTK_TEXT_VIEW (logwdw));
  textlen = gtk_text_buffer_get_char_count (textbuf);
  gtk_text_buffer_get_iter_at_offset (textbuf, &iter, 0);
  gtk_text_buffer_get_iter_at_offset (textbuf, &iter2, textlen);
  txt = gtk_text_buffer_get_text (textbuf, &iter, &iter2, 0);

  /* gtk_text_buffer_get_char_count() returns the number of characters,
     not bytes. So get the number of bytes that need to be written out */
  textlen = strlen (txt);
  pos = txt;

  while (textlen > 0)
    {
      if ((len = write (fd, pos, textlen)) == -1)
        {
          ftp_log (gftp_logging_error, NULL,
                   _("Error: Error writing to %s: %s\n"),
                   tempstr, g_strerror (errno));
          break;
        }
      textlen -= len;
      pos += len;
    }

  fsync (fd);
  lseek (fd, 0, SEEK_SET);
  view_file (tempstr, fd, 1, 1, 0, 1, NULL, NULL);
  close (fd);

  g_free (tempstr);
  g_free (txt);
}


static void
dosavelog (const char *filename)
{
  char *txt, *pos;
  gint textlen;
  ssize_t len;
  FILE *fd;
  int ok;
  GtkTextBuffer * textbuf;
  GtkTextIter iter, iter2;

  if ((fd = fopen (filename, "w")) == NULL)
    {
      ftp_log (gftp_logging_error, NULL,
               _("Error: Cannot open %s for writing: %s\n"), filename,
               g_strerror (errno));
      return;
    }

  textbuf = gtk_text_view_get_buffer (GTK_TEXT_VIEW (logwdw));
  textlen = gtk_text_buffer_get_char_count (textbuf);
  gtk_text_buffer_get_iter_at_offset (textbuf, &iter, 0);
  gtk_text_buffer_get_iter_at_offset (textbuf, &iter2, textlen);
  txt = gtk_text_buffer_get_text (textbuf, &iter, &iter2, 0);

  /* gtk_text_buffer_get_char_count() returns the number of characters,
     not bytes. So get the number of bytes that need to be written out */
  textlen = strlen (txt);

  ok = 1;
  pos = txt;
  do
    {
      if ((len = write (fileno (fd), pos, textlen)) == -1)
        {
          ok = 0;
          ftp_log (gftp_logging_error, NULL,
                   _("Error: Error writing to %s: %s\n"),
                   filename, g_strerror (errno));
          break;
        }

      textlen -= len;
      pos += len;
    } while (textlen > 0);

  if (ok)
    ftp_log (gftp_logging_misc, NULL,
             _("Successfully wrote the log file to %s\n"), filename);

  fclose (fd);
  g_free (txt);
}


void
savelog (GtkAction * a, gpointer data)
{
  GtkWidget *filew;
  filew = gtk_file_chooser_dialog_new (_("Save Log"),
    GTK_WINDOW(window),
    GTK_FILE_CHOOSER_ACTION_SAVE,
    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
    GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
    NULL);
  gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (filew), TRUE);
  gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (filew), "gftp.log");
  if (gtk_dialog_run (GTK_DIALOG (filew)) == GTK_RESPONSE_ACCEPT)
  {
    char *filename;
    filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (filew));
    dosavelog (filename);
    g_free (filename);
  }
  gtk_widget_destroy (filew);
}

void
clear_cache (GtkAction * a, gpointer data)
{
  gftp_clear_cache_files ();
}


void
about_dialog (GtkAction * a, gpointer data)
{
  GtkWidget * tempwid, * notebook, * box, * label, * view, * vscroll, * dialog;
  char *tempstr, *temp1str, *no_license_agreement, *str, buf[255], *share_dir;
  size_t len;
  FILE * fd;
  GtkTextBuffer * textbuf;
  GtkTextIter iter;
  gint textlen;

  share_dir = gftp_get_share_dir ();
  no_license_agreement = g_strdup_printf (_("Cannot find the license agreement file COPYING. Please make sure it is in either %s or in %s"), BASE_CONF_DIR, share_dir);

  dialog = gtk_dialog_new_with_buttons (_("About gFTP"), GTK_WINDOW(window), 0,
                                        GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
                                        NULL);
  gtk_container_set_border_width (GTK_CONTAINER (gtk_dialog_get_content_area(GTK_DIALOG (dialog))), 10);
  gtk_box_set_spacing (GTK_BOX (gtk_dialog_get_content_area(GTK_DIALOG (dialog))), 5);

  notebook = gtk_notebook_new ();
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area(GTK_DIALOG (dialog))), notebook, TRUE,
              TRUE, 0);
  gtk_widget_show (notebook);

  box = gtk_vbox_new (TRUE, 5);
  gtk_container_set_border_width (GTK_CONTAINER (box), 10);
  gtk_widget_show (box);

  tempwid = toolbar_pixmap ("gftp-logo.xpm");
  gtk_box_pack_start (GTK_BOX (box), tempwid, FALSE, FALSE, 0);
  gtk_widget_show (tempwid);

  tempstr = g_strdup_printf (_("%s\nCopyright (C) 1998-2007 Brian Masney <masneyb@gftp.org>\nOfficial Homepage: http://www.gftp.org/\n"), gftp_version);
  str = _("Translated by");
  if (strcmp (str, "Translated by") != 0)
    {
      tempstr = g_realloc (tempstr,
                           (gulong) (strlen (tempstr) + strlen (str) + 1));
      strcat (tempstr, str);
    }
  tempwid = gtk_label_new (tempstr);
  g_free (tempstr);
  gtk_box_pack_start (GTK_BOX (box), tempwid, FALSE, FALSE, 0);
  gtk_widget_show (tempwid);

  label = gtk_label_new (_("About"));
  gtk_widget_show (label);

  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), box, label);

  box = gtk_vbox_new (FALSE, 5);
  gtk_container_set_border_width (GTK_CONTAINER (box), 10);
  gtk_widget_show (box);

  tempwid = gtk_table_new (1, 2, FALSE);
  gtk_box_pack_start (GTK_BOX (box), tempwid, TRUE, TRUE, 0);
  gtk_widget_show (tempwid);

  view = gtk_text_view_new ();
  gtk_text_view_set_editable (GTK_TEXT_VIEW (view), FALSE);
  gtk_text_view_set_cursor_visible (GTK_TEXT_VIEW (view), FALSE);
  gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (view), GTK_WRAP_WORD);

  vscroll = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (vscroll),
                                 GTK_POLICY_AUTOMATIC,
                                 GTK_POLICY_AUTOMATIC);

  gtk_container_add (GTK_CONTAINER (vscroll), view);
  gtk_widget_show (view);

  gtk_table_attach (GTK_TABLE (tempwid), vscroll, 0, 1, 0, 1,
                    GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND | GTK_SHRINK,
                    0, 0);
  gtk_widget_show (vscroll);

  textbuf = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));

  label = gtk_label_new (_("License Agreement"));
  gtk_widget_show (label);

  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), box, label);

  tempstr = g_strconcat ("/usr/share/common-licenses/GPL", NULL);
  if (access (tempstr, F_OK) != 0)
    {
      g_free (tempstr);
      temp1str = g_strconcat (share_dir, "/COPYING", NULL);
      tempstr = gftp_expand_path (NULL, temp1str);
      g_free (temp1str);
      if (access (tempstr, F_OK) != 0)
    {
      g_free (tempstr);
          tempstr = gftp_expand_path (NULL, BASE_CONF_DIR "/COPYING");
      if (access (tempstr, F_OK) != 0)
        {
              textlen = gtk_text_buffer_get_char_count (textbuf);
              gtk_text_buffer_get_iter_at_offset (textbuf, &iter, textlen);
              gtk_text_buffer_insert (textbuf, &iter, no_license_agreement, -1);
          gtk_widget_show (dialog);
          return;
        }
    }
    }

  if ((fd = fopen (tempstr, "r")) == NULL)
    {

      textlen = gtk_text_buffer_get_char_count (textbuf);
      gtk_text_buffer_get_iter_at_offset (textbuf, &iter, textlen);
      gtk_text_buffer_insert (textbuf, &iter, no_license_agreement, -1);
      gtk_widget_show (dialog);
      g_free (tempstr);
      return;
    }
  g_free (tempstr);

  memset (buf, 0, sizeof (buf));
  while ((len = fread (buf, 1, sizeof (buf) - 1, fd)))
    {
      buf[len] = '\0';
      textlen = gtk_text_buffer_get_char_count (textbuf);
      gtk_text_buffer_get_iter_at_offset (textbuf, &iter, textlen);
      gtk_text_buffer_insert (textbuf, &iter, buf, -1);
    }
  fclose (fd);

  gtk_dialog_run (GTK_DIALOG(dialog));
  gtk_widget_destroy (dialog);

  g_free (no_license_agreement);
  gftp_free_pixmap ("gftp-logo.xpm");
}


static void
_do_compare_windows (gftp_window_data * win1, gftp_window_data * win2)
{
  gftp_file * curfle, * otherfle;
  GList * otherlist;
  int curdir, othdir;
  GtkTreeModel * list_store;
  GtkTreeIter iter;
  gboolean valid;

  GtkTreeSelection * select = gtk_tree_view_get_selection (GTK_TREE_VIEW(win1->listbox));
  list_store = gtk_tree_view_get_model(GTK_TREE_VIEW(win1->listbox));
  valid = gtk_tree_model_get_iter_first (list_store, &iter);
  while (valid)
  {
    gtk_tree_model_get (list_store, &iter, 0, &curfle, -1);
    if (!curfle->shown)
    {
      otherlist = win2->files;
      while (otherlist != NULL)
      {
        otherfle = otherlist->data;
        if (otherfle->shown)
        {
          curdir = S_ISDIR (curfle->st_mode);
          othdir = S_ISDIR (otherfle->st_mode);

          if (strcmp (otherfle->file, curfle->file) == 0 &&
            curdir == othdir &&
            (curdir || otherfle->size == curfle->size))
            break;

            otherlist = otherlist->next;
        }
      }
      if (otherlist == NULL)
      {
        gtk_tree_selection_select_iter (select, &iter);
      }
    }
    valid = gtk_tree_model_iter_next (list_store, &iter);
  }
}

void
compare_windows (GtkAction * a, gpointer data)
{
  if (!check_status (_("Compare Windows"), &window2, 1, 0, 0, 1))
    return;

  deselectall (NULL, &window1);
  deselectall (NULL, &window2);

  _do_compare_windows (&window1, &window2);
  _do_compare_windows (&window2, &window1);
}
