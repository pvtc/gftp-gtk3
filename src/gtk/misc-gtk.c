/*****************************************************************************/
/*  misc-gtk.c - misc stuff for the gtk+ 1.2 port of gftp                    */
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
static const char cvsid[] = "$Id$";

static GtkWidget * statuswid;


void
remove_files_window (gftp_window_data * wdata)
{
  wdata->show_selected = 0;

  GtkTreeModel * model = gtk_tree_view_get_model(GTK_TREE_VIEW (wdata->listbox));
  gtk_list_store_clear (GTK_LIST_STORE(model));
  free_file_list (wdata->files);
  wdata->files = NULL;
}


void
ftp_log (gftp_logging_level level, gftp_request * request,
         const char *string, ...)
{
  uintptr_t max_log_window_size;
  int upd, free_logstr;
  gftp_log * newlog;
  gint delsize;
  char *logstr;
  va_list argp;
  size_t len;
  GtkTextBuffer * textbuf;
  GtkTextIter iter, iter2;
  const char *descr;
  char *utf8_str;
  size_t destlen;

  va_start (argp, string);
  if (strcmp (string, "%s") == 0)
    {
      logstr = va_arg (argp, char *);
      free_logstr = 0;
    }
  else
    {
      logstr = g_strdup_vprintf (string, argp);
      free_logstr = 1;
    }
  va_end (argp);

  if ((utf8_str = gftp_string_to_utf8 (request, logstr, &destlen)) != NULL)
    {
      if (free_logstr)
        g_free (logstr);
      else
        free_logstr = 1;

      logstr = utf8_str;
    }

  if (pthread_self () != main_thread_id)
    {
      newlog = g_malloc0 (sizeof (*newlog));
      newlog->type = level;
      if (free_logstr)
        newlog->msg = logstr;
      else
        newlog->msg = g_strdup (logstr);

      pthread_mutex_lock (&log_mutex);
      gftp_file_transfer_logs = g_list_append (gftp_file_transfer_logs, newlog);
      pthread_mutex_unlock (&log_mutex);
      return;
    }

  if (gftp_logfd != NULL && level != gftp_logging_misc_nolog)
    {
      if (fwrite (logstr, strlen (logstr), 1, gftp_logfd) != 1)
        {
          fclose (gftp_logfd);
          gftp_logfd = NULL;
        }
      else
        {
          fflush (gftp_logfd);
          if (ferror (gftp_logfd))
            {
              fclose (gftp_logfd);
              gftp_logfd = NULL;
            }
        }
    }

  upd = logwdw_vadj->upper - logwdw_vadj->page_size == logwdw_vadj->value;

  gftp_lookup_global_option ("max_log_window_size", &max_log_window_size);

  switch (level)
    {
      case gftp_logging_send:
        descr = "send";
        break;
      case gftp_logging_recv:
        descr = "recv";
        break;
      case gftp_logging_error:
        descr = "error";
        break;
      default:
        descr = "misc";
        break;
    }

  textbuf = gtk_text_view_get_buffer (GTK_TEXT_VIEW (logwdw));
  len = gtk_text_buffer_get_char_count (textbuf);
  gtk_text_buffer_get_iter_at_offset (textbuf, &iter, len);
  gtk_text_buffer_insert_with_tags_by_name (textbuf, &iter, logstr, -1,
                                            descr, NULL);

  if (upd)
    {
      gtk_text_buffer_move_mark (textbuf, logwdw_textmark, &iter);
      gtk_text_view_scroll_to_mark (GTK_TEXT_VIEW (logwdw), logwdw_textmark,
                                     0, 1, 1, 1);
    }

  if (max_log_window_size > 0)
    {
      delsize = len + g_utf8_strlen (logstr, -1) - max_log_window_size;

      if (delsize > 0)
        {
          gtk_text_buffer_get_iter_at_offset (textbuf, &iter, 0);
          gtk_text_buffer_get_iter_at_offset (textbuf, &iter2, delsize);
          gtk_text_buffer_delete (textbuf, &iter, &iter2);
        }
    }

  if (free_logstr)
    g_free (logstr);
}


void
update_window_info (void)
{
  char *tempstr, empty[] = "";
  unsigned int port, i, j;
  GtkWidget * tempwid;

  if (current_wdata->request != NULL)
    {
      if (GFTP_IS_CONNECTED (current_wdata->request))
        {
          if ((tempstr = current_wdata->request->hostname) == NULL)
            tempstr = empty;
          gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (hostedit)->entry), tempstr);

          if ((tempstr = current_wdata->request->username) == NULL)
            tempstr = empty;
          gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (useredit)->entry), tempstr);

          if ((tempstr = current_wdata->request->password) == NULL)
            tempstr = empty;
          gtk_entry_set_text (GTK_ENTRY (passedit), tempstr);

          port = gftp_protocol_default_port (current_wdata->request);
          if (current_wdata->request->port != 0 &&
              port != current_wdata->request->port)
            {
              tempstr = g_strdup_printf ("%d", current_wdata->request->port);
              gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (portedit)->entry), tempstr);
              g_free (tempstr);
            }
          else
            gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (portedit)->entry), "");

          for (i=0, j=0; gftp_protocols[i].init != NULL; i++)
            {
              if (!gftp_protocols[i].shown)
                continue;

              if (current_wdata->request->init == gftp_protocols[i].init)
                {
                  gtk_option_menu_set_history (GTK_OPTION_MENU (optionmenu), i);
                  break;
                }
              j++;
            }

            gtk_widget_set_tooltip_text (openurl_btn,
                                _("Disconnect from the remote server"));
        }
      else
          gtk_widget_set_tooltip_text (openurl_btn,
                              _("Connect to the site specified in the host entry. If the host entry is blank, then a dialog is presented that will allow you to enter a URL."));
    }

  update_window (&window1);
  update_window (&window2);

  tempwid = gtk_item_factory_get_widget (factory, "/Tools/Compare Windows");
  gtk_widget_set_sensitive (tempwid, GFTP_IS_CONNECTED (window1.request)
                && GFTP_IS_CONNECTED (window2.request));
}


static void
set_menu_sensitive (gftp_window_data * wdata, char *path, int sensitive)
{
  GtkWidget * tempwid;
  char * pos;

  tempwid = NULL;

  if (factory != NULL)
    tempwid = gtk_item_factory_get_widget (factory, path);
  if (tempwid)
    gtk_widget_set_sensitive (tempwid, sensitive);

  if (wdata != NULL)
    {
      if ((pos = strchr (path + 1, '/')) == NULL)
        pos = path;

      if (wdata->ifactory)
        tempwid = gtk_item_factory_get_widget (wdata->ifactory, pos);
      if (tempwid)
        gtk_widget_set_sensitive (tempwid, sensitive);
    }
}


void
update_window (gftp_window_data * wdata)
{
  char *tempstr, *hostname, *fspec;
  int connected, start;

  connected = GFTP_IS_CONNECTED (wdata->request);
  if (connected)
    {
      fspec = wdata->show_selected ? "Selected" : strcmp (wdata->filespec, "*") == 0 ?  _("All Files") : wdata->filespec;

      if (wdata->request->hostname == NULL ||
          wdata->request->protonum == GFTP_LOCAL_NUM)
        hostname = "";
      else
        hostname = wdata->request->hostname;

      tempstr = g_strconcat (hostname, *hostname == '\0' ? "[" : " [",
                             gftp_protocols[wdata->request->protonum].name,
                             wdata->request->cached ? _("] (Cached) [") : "] [",
                             fspec, "]", current_wdata == wdata ? "*" : "", NULL);
      gtk_label_set (GTK_LABEL (wdata->hoststxt), tempstr);
      g_free (tempstr);

      if (wdata->request->directory != NULL)
        gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (wdata->combo)->entry),
                            wdata->request->directory);
    }
  else if (wdata->hoststxt != NULL)
    {
      tempstr = g_strconcat (_("Not connected"),
                             current_wdata == wdata ? "*" : "", NULL);
      gtk_label_set (GTK_LABEL (wdata->hoststxt), tempstr);
      g_free (tempstr);
    }

  if (wdata == &window1)
    start = local_start;
  else
    start = remote_start;

  set_menu_sensitive (wdata, menus[start + 3].path, connected &&
                      strcmp (wdata->request->url_prefix, "file") != 0);
  set_menu_sensitive (wdata, menus[start + 5].path, connected);
  set_menu_sensitive (wdata, menus[start + 6].path, connected);
  set_menu_sensitive (wdata, menus[start + 7].path, connected);
  set_menu_sensitive (wdata, menus[start + 8].path, connected);
  set_menu_sensitive (wdata, menus[start + 9].path, connected);
  set_menu_sensitive (wdata, menus[start + 11].path, connected);
  set_menu_sensitive (wdata, menus[start + 12].path, connected &&
                      wdata->request->site != NULL);
  set_menu_sensitive (wdata, menus[start + 13].path, connected &&
                      wdata->request->chdir!= NULL);
  set_menu_sensitive (wdata, menus[start + 14].path, connected &&
                      wdata->request->chmod != NULL);
  set_menu_sensitive (wdata, menus[start + 15].path, connected &&
                      wdata->request->mkdir != NULL);
  set_menu_sensitive (wdata, menus[start + 16].path, connected &&
                      wdata->request->rename != NULL);
  set_menu_sensitive (wdata, menus[start + 17].path, connected &&
                      wdata->request->rmdir != NULL &&
                      wdata->request->rmfile != NULL);
  set_menu_sensitive (wdata, menus[start + 18].path, connected &&
                      wdata->request->get_file != NULL);
  set_menu_sensitive (wdata, menus[start + 19].path, connected &&
                      wdata->request->get_file != NULL);
  set_menu_sensitive (wdata, menus[start + 20].path, connected);

  connected = GFTP_IS_CONNECTED (window1.request) && GFTP_IS_CONNECTED (window2.request);

  start = trans_start;
  set_menu_sensitive (NULL, menus[start + 2].path, connected);
  set_menu_sensitive (NULL, menus[start + 3].path, connected);
  set_menu_sensitive (NULL, menus[start + 5].path, connected);
  set_menu_sensitive (NULL, menus[start + 6].path, connected);

  set_menu_sensitive (NULL, menus[start + 7].path, connected);
  set_menu_sensitive (NULL, menus[start + 8].path, connected);

  set_menu_sensitive (NULL, menus[start + 10].path, connected);
  set_menu_sensitive (NULL, menus[start + 11].path, connected);

  gtk_widget_set_sensitive (download_left_arrow, connected);
  gtk_widget_set_sensitive (upload_right_arrow, connected);
}


GtkWidget *
toolbar_pixmap (char *filename)
{
  gftp_graphic * graphic;
  GtkWidget *pix;

  if (filename == NULL || *filename == '\0')
    return (NULL);

  graphic = open_xpm (filename);

  if (graphic == NULL)
    return (NULL);

  if ((pix = gtk_image_new_from_pixbuf (graphic->pb)) == NULL)
    return (NULL);

  gtk_widget_show (pix);
  return (pix);
}


gftp_graphic *
open_xpm (char *filename)
{
  gftp_graphic * graphic;
  char *exfile;
  GError * error = NULL;

  if ((graphic = g_hash_table_lookup (graphic_hash_table, filename)) != NULL)
    return (graphic);

  if ((exfile = get_xpm_path (filename, 0)) == NULL)
    return (NULL);

  graphic = g_malloc0 (sizeof (*graphic));
  graphic->pb =  gdk_pixbuf_new_from_file(exfile, &error);
  g_free (exfile);
  if (graphic->pb == NULL && error != NULL)
    {
      g_free (graphic);
      ftp_log (gftp_logging_error, NULL, _("Error opening file %s: %s\n"),
               exfile, error->message);
      g_error_free (error);
      return (NULL);
    }

  graphic->filename = g_strdup (filename);
  g_hash_table_insert (graphic_hash_table, graphic->filename, graphic);

  return (graphic);
}


void
gftp_free_pixmap (char *filename)
{
  gftp_graphic * graphic;

  if ((graphic = g_hash_table_lookup (graphic_hash_table, filename)) == NULL)
    return;

  g_object_unref (graphic->pb);
  g_hash_table_remove (graphic_hash_table, filename);
  g_free (graphic->filename);
  g_free (graphic);
}

GdkPixbuf *
gftp_get_pixmap (char *filename)
{
  gftp_graphic * graphic;

  if (filename == NULL || *filename == '\0')
    {
      return NULL;
    }

  if ((graphic = g_hash_table_lookup (graphic_hash_table, filename)) == NULL)
    graphic = open_xpm (filename);

  if (graphic == NULL)
    {
      return NULL;
    }

  return graphic->pb;
}


int
check_status (char *name, gftp_window_data *wdata,
              unsigned int check_other_stop, unsigned int only_one,
              unsigned int at_least_one, unsigned int func)
{
  gftp_window_data * owdata;

  owdata = wdata == &window1 ? &window2 : &window1;

  if (wdata->request->stopable)
    {
      ftp_log (gftp_logging_error, NULL,
           _("%s: Please hit the stop button first to do anything else\n"),
           name);
      return (0);
    }

  if (check_other_stop && owdata->request->stopable)
    {
      ftp_log (gftp_logging_error, NULL,
           _("%s: Please hit the stop button first to do anything else\n"),
           name);
      return (0);
    }

  if (!GFTP_IS_CONNECTED (wdata->request))
    {
      ftp_log (gftp_logging_error, NULL,
           _("%s: Not connected to a remote site\n"), name);
      return (0);
    }

  if (!func)
    {
      ftp_log (gftp_logging_error, NULL,
           _("%s: This feature is not available using this protocol\n"),
           name);
      return (0);
    }

  GtkTreeSelection *select;
  select = gtk_tree_view_get_selection (GTK_TREE_VIEW (wdata->listbox));
  if (only_one && gtk_tree_selection_count_selected_rows(select) != 1)
    {
      ftp_log (gftp_logging_error, NULL,
           _("%s: You must only have one item selected\n"), name);
      return (0);
    }

  if (at_least_one && !only_one && gtk_tree_selection_count_selected_rows(select) == 0)
    {
      ftp_log (gftp_logging_error, NULL,
           _("%s: You must have at least one item selected\n"), name);
      return (0);
    }
  return (1);
}


static gchar *
gftp_item_factory_translate (const char *path, gpointer func_data)
{
  const gchar *strip_prefix = func_data;
  const char *result;

  if (strip_prefix)
    {
      char *tmp_path = g_strconcat (strip_prefix, path, NULL);
      result = gettext (tmp_path);
      if (result == tmp_path)
        result = path;
      g_free (tmp_path);
    }
  else
    result = gettext (path);

  return (char *)result;
}


GtkItemFactory *
item_factory_new (GType container_type, const char *path,
          GtkAccelGroup *accel_group, const char *strip_prefix)
{
  GtkItemFactory *result = gtk_item_factory_new (container_type, path, accel_group);
  gchar *strip_prefix_dup = g_strdup (strip_prefix);

  gtk_item_factory_set_translate_func (result, gftp_item_factory_translate,
                       strip_prefix_dup, NULL);

  if (strip_prefix_dup)
    g_object_set_data_full (G_OBJECT (result), "gftp-strip-prefix",
                  strip_prefix_dup, (GDestroyNotify)g_free);

  return result;
}


void
create_item_factory (GtkItemFactory * ifactory, gint n_entries,
                     GtkItemFactoryEntry * entries, gpointer callback_data)
{
  const char *strip_prefix;
  size_t strip_prefix_len;
  int i;

  strip_prefix = g_object_get_data (G_OBJECT (ifactory), "gftp-strip-prefix");
  if (strip_prefix)
    strip_prefix_len = strlen (strip_prefix);
  else
    strip_prefix_len = 0;

  for (i = 0; i < n_entries; i++)
    {
      GtkItemFactoryEntry dummy_item = entries[i];
      if (strip_prefix && strncmp (entries[i].path, strip_prefix, strip_prefix_len) == 0)
    dummy_item.path += strip_prefix_len;

      gtk_item_factory_create_item (ifactory, &dummy_item, callback_data, 1);
    }
}

void
add_history (GtkWidget * widget, GList ** history, unsigned int *histlen,
             const char *str)
{
  GList *node, *delnode;
  char *tempstr;
  int i;

  if (str == NULL || *str == '\0')
    return;

  for (node = *history; node != NULL; node = node->next)
    {
      if (strcmp ((char *) node->data, str) == 0)
    break;
    }

  if (node == NULL)
    {
      if (*histlen >= MAX_HIST_LEN)
    {
      node = *history;
      for (i = 1; i < MAX_HIST_LEN; i++)
        node = node->next;
      node->prev->next = NULL;
      node->prev = NULL;
      delnode = node;
      while (delnode != NULL)
        {
          if (delnode->data)
        g_free (delnode->data);
          delnode = delnode->next;
        }
      g_list_free (node);
    }
      tempstr = g_strdup (str);
      *history = g_list_prepend (*history, tempstr);
      ++*histlen;
    }
  else if (node->prev != NULL)
    {
      node->prev->next = node->next;
      if (node->next != NULL)
    node->next->prev = node->prev;
      node->prev = NULL;
      node->next = *history;
      if (node->next != NULL)
    node->next->prev = node;
      *history = node;
    }
  gtk_combo_set_popdown_strings (GTK_COMBO (widget), *history);
}


int
check_reconnect (gftp_window_data *wdata)
{
  return (wdata->request->cached && wdata->request->datafd < 0 &&
          !wdata->request->always_connected &&
      !ftp_connect (wdata, wdata->request) ? -1 : 0);
}


void
add_file_listbox (gftp_window_data * wdata, gftp_file * fle)
{
  GtkTreeIter iter;

  if (wdata->show_selected)
    {
      fle->shown = fle->was_sel;
      if (!fle->shown)
        return;
    }
  else if (!gftp_match_filespec (wdata->request, fle->file, wdata->filespec))
    {
      fle->shown = 0;
      fle->was_sel = 0;
      return;
    }
  else
    fle->shown = 1;

  GtkTreeModel * model = gtk_tree_view_get_model(GTK_TREE_VIEW (wdata->listbox));
  gtk_list_store_append (GTK_LIST_STORE(model), &iter);
  gtk_list_store_set(GTK_LIST_STORE(model), &iter, 0, fle, -1);
  if (fle->was_sel)
  {
    GtkTreeSelection * select;
    select = gtk_tree_view_get_selection (GTK_TREE_VIEW (wdata->listbox));
    fle->was_sel = 0;
    gtk_tree_selection_select_iter (select, &iter);
  }
}


void
destroy_dialog (gftp_dialog_data * ddata)
{
  if (ddata->dialog != NULL)
    {
      gtk_widget_destroy (ddata->dialog);
      ddata->dialog = NULL;
    }
}


#if GTK_MAJOR_VERSION == 1
static void
ok_dialog_response (GtkWidget * widget, gftp_dialog_data * ddata)
{
  if (ddata->edit == NULL)
    {
      gtk_widget_destroy (ddata->dialog);
      ddata->dialog = NULL;
      ddata->checkbox = NULL;
    }

  if (ddata->yesfunc != NULL)
    ddata->yesfunc (ddata->yespointer, ddata);

  if (ddata->edit != NULL &&
      ddata->dialog != NULL)
    gtk_widget_destroy (ddata->dialog);

  g_free (ddata);
}


static void
cancel_dialog_response (GtkWidget * widget, gftp_dialog_data * ddata)
{
  if (ddata->edit == NULL)
    {
      gtk_widget_destroy (ddata->dialog);
      ddata->dialog = NULL;
      ddata->checkbox = NULL;
    }

  if (ddata->nofunc != NULL)
    ddata->nofunc (ddata->nopointer, ddata);

  if (ddata->edit != NULL &&
      ddata->dialog != NULL)
    gtk_widget_destroy (ddata->dialog);

  g_free (ddata);
}
#else
static void
dialog_response (GtkWidget * widget, gint response, gftp_dialog_data * ddata)
{
  if (ddata->edit == NULL)
    {
      gtk_widget_destroy (ddata->dialog);
      ddata->dialog = NULL;
      ddata->checkbox = NULL;
    }

  switch (response)
    {
      case GTK_RESPONSE_YES:
        if (ddata->yesfunc != NULL)
          ddata->yesfunc (ddata->yespointer, ddata);
        break;
      default:
        if (ddata->nofunc != NULL)
          ddata->nofunc (ddata->nopointer, ddata);
        break;
    }

  if (ddata->edit != NULL &&
      ddata->dialog != NULL)
    gtk_widget_destroy (ddata->dialog);

  g_free (ddata);
}
#endif


static gint
dialog_keypress (GtkWidget * widget, GdkEventKey * event, gpointer data)
{
  if (event->type != GDK_KEY_PRESS)
    return (FALSE);

  if (event->keyval == GDK_KP_Enter || event->keyval == GDK_Return)
    {
      dialog_response (widget, GTK_RESPONSE_YES, data);
      return (TRUE);
    }
  else if (event->keyval == GDK_Escape)
    {
      dialog_response (widget, GTK_RESPONSE_NO, data);
      return (TRUE);
    }

  return (FALSE);
}


void
MakeEditDialog (char *diagtxt, char *infotxt, char *deftext, int passwd_item,
        char *checktext,
                gftp_dialog_button okbutton, void (*okfunc) (), void *okptr,
        void (*cancelfunc) (), void *cancelptr)
{
  GtkWidget * tempwid, * dialog;
  gftp_dialog_data * ddata;
  const gchar * yes_text;

  ddata = g_malloc0 (sizeof (*ddata));
  ddata->yesfunc = okfunc;
  ddata->yespointer = okptr;
  ddata->nofunc = cancelfunc;
  ddata->nopointer = cancelptr;

  switch (okbutton)
    {
      case gftp_dialog_button_ok:
        yes_text = GTK_STOCK_OK;
        break;
      case gftp_dialog_button_create:
        yes_text = GTK_STOCK_ADD;
        break;
      case gftp_dialog_button_change:
        yes_text = _("Change");
        break;
      case gftp_dialog_button_connect:
        yes_text = _("Connect");
        break;
      case gftp_dialog_button_rename:
        yes_text = _("Rename");
        break;
      default:
        yes_text = GTK_STOCK_MISSING_IMAGE;
        break;
    }

  dialog = gtk_dialog_new_with_buttons (_(diagtxt), NULL, 0,
                                        GTK_STOCK_CANCEL,
                                        GTK_RESPONSE_NO,
                                        yes_text,
                                        GTK_RESPONSE_YES,
                                        NULL);

  gtk_container_border_width (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), 10);
  gtk_box_set_spacing (GTK_BOX (GTK_DIALOG (dialog)->vbox), 5);
  gtk_window_set_wmclass (GTK_WINDOW(dialog), "edit", "gFTP");
  gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_MOUSE);
  gtk_grab_add (dialog);
  gtk_widget_realize (dialog);

  ddata->dialog = dialog;

  tempwid = gtk_label_new (infotxt);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), tempwid, TRUE,
              TRUE, 0);
  gtk_widget_show (tempwid);

  ddata->edit = gtk_entry_new ();
  g_signal_connect (G_OBJECT (ddata->edit), "key_press_event",
                      G_CALLBACK (dialog_keypress), (gpointer) ddata);

  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), ddata->edit, TRUE,
              TRUE, 0);
  gtk_widget_grab_focus (ddata->edit);
  gtk_entry_set_visibility (GTK_ENTRY (ddata->edit), passwd_item);

  if (deftext != NULL)
    {
      gtk_entry_set_text (GTK_ENTRY (ddata->edit), deftext);
      gtk_entry_select_region (GTK_ENTRY (ddata->edit), 0, strlen (deftext));
    }
  gtk_widget_show (ddata->edit);

  if (checktext != NULL)
    {
      ddata->checkbox = gtk_check_button_new_with_label (checktext);
      gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox),
                          ddata->checkbox, TRUE, TRUE, 0);
      gtk_widget_show (ddata->checkbox);
    }

  g_signal_connect (G_OBJECT (dialog), "response",
                    G_CALLBACK (dialog_response), ddata);

  gtk_widget_show (dialog);
}


void
MakeYesNoDialog (char *diagtxt, char *infotxt,
                 void (*yesfunc) (), gpointer yespointer,
                 void (*nofunc) (), gpointer nopointer)
{
  GtkWidget * text, * dialog;
  gftp_dialog_data * ddata;

  ddata = g_malloc0 (sizeof (*ddata));
  ddata->yesfunc = yesfunc;
  ddata->yespointer = yespointer;
  ddata->nofunc = nofunc;
  ddata->nopointer = nopointer;
  dialog = gtk_dialog_new_with_buttons (_(diagtxt), NULL, 0,
                                        GTK_STOCK_NO,
                                        GTK_RESPONSE_NO,
                                        GTK_STOCK_YES,
                                        GTK_RESPONSE_YES,
                                        NULL);

  gtk_container_border_width (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), 10);
  gtk_box_set_spacing (GTK_BOX (GTK_DIALOG (dialog)->vbox), 5);
  gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_MOUSE);
  gtk_window_set_wmclass (GTK_WINDOW(dialog), "yndiag", "gFTP");
  gtk_grab_add (dialog);
  gtk_widget_realize (dialog);

  ddata->dialog = dialog;

  text = gtk_label_new (infotxt);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), text, TRUE, TRUE, 0);
  gtk_widget_show (text);

  g_signal_connect (G_OBJECT (dialog), "response",
                    G_CALLBACK (dialog_response), ddata);

  gtk_widget_show (dialog);
}


static gint
delete_event (GtkWidget * widget, GdkEvent * event, gpointer data)
{
  return (TRUE);
}


static void
trans_stop_button (GtkWidget * widget, gpointer data)
{
  gftp_transfer * transfer;

  transfer = data;
  pthread_kill (((gftp_window_data *) transfer->fromwdata)->tid, SIGINT);
}


void
update_directory_download_progress (gftp_transfer * transfer)
{
  static GtkWidget * dialog = NULL, * textwid, * stopwid;
  char tempstr[255];
  GtkWidget * vbox;

  if (transfer->numfiles < 0 || transfer->numdirs < 0)
    {
      if (dialog != NULL)
        gtk_widget_destroy (dialog);
      dialog = NULL;
      return;
    }

  if (dialog == NULL)
    {
      dialog = gtk_window_new (GTK_WINDOW_TOPLEVEL);
      gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER);
      gtk_window_set_decorated (GTK_WINDOW (dialog), 0);
      gtk_grab_add (dialog);
      g_signal_connect (G_OBJECT (dialog), "delete_event",
                          G_CALLBACK (delete_event), NULL);
      gtk_window_set_title (GTK_WINDOW (dialog),
                _("Getting directory listings"));
      gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_MOUSE);
      gtk_window_set_wmclass (GTK_WINDOW(dialog), "dirlist", "gFTP");

      vbox = gtk_vbox_new (FALSE, 5);
      gtk_container_border_width (GTK_CONTAINER (vbox), 10);
      gtk_container_add (GTK_CONTAINER (dialog), vbox);
      gtk_widget_show (vbox);

      textwid = gtk_label_new (NULL);
      gtk_box_pack_start (GTK_BOX (vbox), textwid, TRUE, TRUE, 0);
      gtk_widget_show (textwid);

      statuswid = gtk_progress_bar_new ();
      gtk_progress_set_activity_mode (GTK_PROGRESS (statuswid), 1);
      gtk_progress_bar_set_activity_step (GTK_PROGRESS_BAR (statuswid), 3);
      gtk_progress_bar_set_activity_blocks (GTK_PROGRESS_BAR (statuswid), 5);
      gtk_box_pack_start (GTK_BOX (vbox), statuswid, TRUE, TRUE, 0);
      gtk_widget_show (statuswid);

      stopwid = gtk_button_new_with_label (_("  Stop  "));
      g_signal_connect (G_OBJECT (stopwid), "clicked",
                          G_CALLBACK (trans_stop_button), transfer);
      gtk_box_pack_start (GTK_BOX (vbox), stopwid, TRUE, TRUE, 0);
      gtk_widget_show (stopwid);

      gtk_widget_show (dialog);
    }

  g_snprintf (tempstr, sizeof (tempstr),
              _("Received %ld directories\nand %ld files"),
              transfer->numdirs, transfer->numfiles);
  gtk_label_set_text (GTK_LABEL (textwid), tempstr);
}


int
progress_timeout (gpointer data)
{
  gftp_transfer * tdata;
  double val;

  tdata = data;

  update_directory_download_progress (tdata);

  val = gtk_progress_get_value (GTK_PROGRESS (statuswid));
  if (val >= 1.0)
    val = 0.0;
  else
    val += 0.10;
  gtk_progress_bar_update (GTK_PROGRESS_BAR (statuswid), val);

  return (1);
}


void
display_cached_logs (void)
{
  gftp_log * templog;
  GList * templist;

  pthread_mutex_lock (&log_mutex);
  templist = gftp_file_transfer_logs;
  while (templist != NULL)
    {
      templog = (gftp_log *) templist->data;
      ftp_log (templog->type, NULL, "%s", templog->msg);
      g_free (templog->msg);
      g_free (templog);
      templist->data = NULL;
      templist = templist->next;
    }
  g_list_free (gftp_file_transfer_logs);
  gftp_file_transfer_logs = NULL;
  pthread_mutex_unlock (&log_mutex);
}


char *
get_xpm_path (char *filename, int quit_on_err)
{
  char *tempstr, *exfile, *share_dir;

  tempstr = g_strconcat (BASE_CONF_DIR, "/", filename, NULL);
  exfile = gftp_expand_path (NULL, tempstr);
  g_free (tempstr);
  if (access (exfile, F_OK) != 0)
    {
      g_free (exfile);
      share_dir = gftp_get_share_dir ();

      tempstr = g_strconcat (share_dir, "/", filename, NULL);
      exfile = gftp_expand_path (NULL, tempstr);
      g_free (tempstr);
      if (access (exfile, F_OK) != 0)
    {
      g_free (exfile);
      exfile = g_strconcat ("/usr/share/icons/", filename, NULL);
      if (access (exfile, F_OK) != 0)
        {
          g_free (exfile);
          if (!quit_on_err)
        return (NULL);

          printf (_("gFTP Error: Cannot find file %s in %s or %s\n"),
              filename, share_dir, BASE_CONF_DIR);
          exit (EXIT_FAILURE);
        }
    }
    }
  return (exfile);
}
