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
dochange_filespec (gftp_window_data * wdata, const char *edttext)
{
  wdata->show_selected = 0;

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
  char * text;

  wdata = data;
  if (!check_status (_("Change Filespec"), wdata, 0, 0, 0, 1))
    return;
  text = MakeEditDialog (_("Change Filespec"), _("Enter the new file specification"),
    wdata->filespec, 1, NULL, _("Change"), NULL);
  if (text != NULL)
  {
    dochange_filespec(wdata, text);
  }
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

      tempstr = gftp_gen_ls_string (NULL, tempfle);
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
  GtkWidget * dialog;
  const gchar * authors[] = {
    "Copyright (C) 1998-2007",
    "Brian Masney <masneyb@gftp.org>",
    "Copyright (C) 2011",
    "Virgile Petit <povitecu@gmail.org>",
    NULL
    };

  dialog = gtk_about_dialog_new();
  gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(window));;
  gtk_about_dialog_set_logo(GTK_ABOUT_DIALOG(dialog), gftp_get_pixmap("gftp-logo.xpm"));
  gtk_about_dialog_set_translator_credits(GTK_ABOUT_DIALOG(dialog), _("Translated by"));
  gtk_window_set_title (GTK_WINDOW(dialog), _("About gFTP"));
  gtk_about_dialog_set_version (GTK_ABOUT_DIALOG(dialog), gftp_version);
  gtk_about_dialog_set_license_type(GTK_ABOUT_DIALOG(dialog), GTK_LICENSE_GPL_2_0);
  gtk_about_dialog_set_website(GTK_ABOUT_DIALOG(dialog), "http://www.gftp.org/");
  gtk_about_dialog_set_authors (GTK_ABOUT_DIALOG(dialog), authors);
  gtk_dialog_run (GTK_DIALOG(dialog));
  gtk_widget_destroy (dialog);
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
