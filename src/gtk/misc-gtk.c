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

struct ftp_log_line
{
    gftp_logging_level level;
    char * logstr;
};

static int
do_ftp_log (void * idata)
{
  struct ftp_log_line * data;
  uintptr_t max_log_window_size;
  int upd;
  gint delsize;
  size_t len;
  GtkTextBuffer * textbuf;
  GtkTextIter iter, iter2;
  const char *descr;

  data = idata;
  if (gftp_logfd != NULL && data->level != gftp_logging_misc_nolog)
    {
      if (fwrite (data->logstr, strlen (data->logstr), 1, gftp_logfd) != 1)
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

  upd = gtk_adjustment_get_upper(logwdw_vadj) - gtk_adjustment_get_page_size(logwdw_vadj) == gtk_adjustment_get_value(logwdw_vadj);

  gftp_lookup_global_option ("max_log_window_size", &max_log_window_size);

  switch (data->level)
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
  gtk_text_buffer_insert_with_tags_by_name (textbuf, &iter, data->logstr, -1,
                                            descr, NULL);

  if (upd)
    {
      gtk_text_buffer_move_mark (textbuf, logwdw_textmark, &iter);
      gtk_text_view_scroll_to_mark (GTK_TEXT_VIEW (logwdw), logwdw_textmark,
                                     0, 1, 1, 1);
    }

  if (max_log_window_size > 0)
    {
      delsize = len + g_utf8_strlen (data->logstr, -1) - max_log_window_size;

      if (delsize > 0)
        {
          gtk_text_buffer_get_iter_at_offset (textbuf, &iter, 0);
          gtk_text_buffer_get_iter_at_offset (textbuf, &iter2, delsize);
          gtk_text_buffer_delete (textbuf, &iter, &iter2);
        }
    }
  g_free (data->logstr);
  g_free (data);
  return FALSE;
}

void
ftp_log (gftp_logging_level level, gftp_request * request,
         const char *string, ...)
{
  struct ftp_log_line * data;
  va_list argp;
  char *utf8_str;
  size_t destlen;

  data = g_malloc(sizeof(* data));
  data->level = level;

  va_start (argp, string);
  data->logstr = g_strdup_vprintf (string, argp);
  va_end (argp);

  if ((utf8_str = gftp_string_to_utf8 (request, data->logstr, &destlen)) != NULL)
    {
     g_free (data->logstr);
     data->logstr = utf8_str;
    }

 g_idle_add (do_ftp_log, data);
}

void
update_window_info (void)
{
  char *tempstr, empty[] = "";
  unsigned int port, i, j;
  GtkAction * a;

  if (current_wdata->request != NULL)
    {
      if (GFTP_IS_CONNECTED (current_wdata->request))
        {
          if ((tempstr = current_wdata->request->hostname) == NULL)
            tempstr = empty;
          gtk_entry_set_text (GTK_ENTRY (gtk_bin_get_child (GTK_BIN (hostedit))), tempstr);

          if ((tempstr = current_wdata->request->username) == NULL)
            tempstr = empty;
          gtk_entry_set_text (GTK_ENTRY (gtk_bin_get_child (GTK_BIN (useredit))), tempstr);

          if ((tempstr = current_wdata->request->password) == NULL)
            tempstr = empty;
          gtk_entry_set_text (GTK_ENTRY (passedit), tempstr);

          port = gftp_protocol_default_port (current_wdata->request);
          if (current_wdata->request->port != 0 &&
              port != current_wdata->request->port)
            {
              tempstr = g_strdup_printf ("%d", current_wdata->request->port);
              gtk_entry_set_text (GTK_ENTRY (gtk_bin_get_child (GTK_BIN (portedit))), tempstr);
              g_free (tempstr);
            }
          else
            gtk_entry_set_text (GTK_ENTRY (gtk_bin_get_child (GTK_BIN (portedit))), "");

          for (i=0, j=0; gftp_protocols[i].init != NULL; i++)
            {
              if (!gftp_protocols[i].shown)
                continue;

              if (current_wdata->request->init == gftp_protocols[i].init)
                {
                  gtk_combo_box_set_active (GTK_COMBO_BOX (optionmenu), i);
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

  a = gtk_ui_manager_get_action (ui_manager, "/MainMenu/Tools/Compare Windows");
  gtk_action_set_sensitive (a,  GFTP_IS_CONNECTED (window1.request)
                && GFTP_IS_CONNECTED (window2.request));
}


static void
set_menu_sensitive (char *path, int sensitive)
{
  GtkAction * a;
  a = gtk_ui_manager_get_action (ui_manager, path);
  gtk_action_set_sensitive (a, sensitive);
}


void
update_window (gftp_window_data * wdata)
{
  char *tempstr, *hostname, *fspec;
  int connected;

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
      gtk_label_set_text (GTK_LABEL (wdata->hoststxt), tempstr);
      g_free (tempstr);

      if (wdata->request->directory != NULL)
        gtk_entry_set_text (GTK_ENTRY (gtk_bin_get_child (GTK_BIN (wdata->combo))),
                            wdata->request->directory);
    }
  else if (wdata->hoststxt != NULL)
    {
      tempstr = g_strconcat (_("Not connected"),
                             current_wdata == wdata ? "*" : "", NULL);
      gtk_label_set_text (GTK_LABEL (wdata->hoststxt), tempstr);
      g_free (tempstr);
    }

  if (wdata == &window1)
  {
    set_menu_sensitive ("/MainMenu/Local/local_D_isconnect", connected && strcmp (wdata->request->url_prefix, "file") != 0);
    set_menu_sensitive ("/MainMenu/Local/local_Change _Filespec...", connected);
    set_menu_sensitive ("/MainMenu/Local/local_Show selected", connected);
    set_menu_sensitive ("/MainMenu/Local/local_Select _All", connected);
    set_menu_sensitive ("/MainMenu/Local/local_Select All Files", connected);
    set_menu_sensitive ("/MainMenu/Local/local_Deselect All", connected);
    set_menu_sensitive ("/MainMenu/Local/local_Save Directory Listing...", connected);
    set_menu_sensitive ("/MainMenu/Local/local_Send SITE Command...", connected && wdata->request->site != NULL);
    set_menu_sensitive ("/MainMenu/Local/local_Change Directory", connected && wdata->request->chdir!= NULL);
    set_menu_sensitive ("/MainMenu/Local/local_Permissions...", connected && wdata->request->chmod != NULL);
    set_menu_sensitive ("/MainMenu/Local/local_New Folder...", connected && wdata->request->mkdir != NULL);
    set_menu_sensitive ("/MainMenu/Local/local_Rena_me...", connected && wdata->request->rename != NULL);
    set_menu_sensitive ("/MainMenu/Local/local_Delete...", connected && wdata->request->rmdir != NULL && wdata->request->rmfile != NULL);
    set_menu_sensitive ("/MainMenu/Local/local_Edit...", connected && wdata->request->get_file != NULL);
    set_menu_sensitive ("/MainMenu/Local/local_View...", connected && wdata->request->get_file != NULL);
    set_menu_sensitive ("/MainMenu/Local/local_Refresh", connected);
  }
  else
  {
    set_menu_sensitive ("/MainMenu/Remote/remote_D_isconnect", connected && strcmp (wdata->request->url_prefix, "file") != 0);
    set_menu_sensitive ("/MainMenu/Remote/remote_Change _Filespec...", connected);
    set_menu_sensitive ("/MainMenu/Remote/remote_Show selected", connected);
    set_menu_sensitive ("/MainMenu/Remote/remote_Select _All", connected);
    set_menu_sensitive ("/MainMenu/Remote/remote_Select All Files", connected);
    set_menu_sensitive ("/MainMenu/Remote/remote_Deselect All", connected);
    set_menu_sensitive ("/MainMenu/Remote/remote_Save Directory Listing...", connected);
    set_menu_sensitive ("/MainMenu/Remote/remote_Send SITE Command...", connected && wdata->request->site != NULL);
    set_menu_sensitive ("/MainMenu/Remote/remote_Change Directory", connected && wdata->request->chdir!= NULL);
    set_menu_sensitive ("/MainMenu/Remote/remote_Permissions...", connected && wdata->request->chmod != NULL);
    set_menu_sensitive ("/MainMenu/Remote/remote_New Folder...", connected && wdata->request->mkdir != NULL);
    set_menu_sensitive ("/MainMenu/Remote/remote_Rena_me...", connected && wdata->request->rename != NULL);
    set_menu_sensitive ("/MainMenu/Remote/remote_Delete...", connected && wdata->request->rmdir != NULL && wdata->request->rmfile != NULL);
    set_menu_sensitive ("/MainMenu/Remote/remote_Edit...", connected && wdata->request->get_file != NULL);
    set_menu_sensitive ("/MainMenu/Remote/remote_View...", connected && wdata->request->get_file != NULL);
    set_menu_sensitive ("/MainMenu/Remote/remote_Refresh", connected);
  }

  connected = GFTP_IS_CONNECTED (window1.request) && GFTP_IS_CONNECTED (window2.request);
  set_menu_sensitive ("/MainMenu/Transfer/t_Start", connected);
  set_menu_sensitive ("/MainMenu/Transfer/t_Stop", connected);
  set_menu_sensitive ("/MainMenu/Transfer/t_Skip", connected);
  set_menu_sensitive ("/MainMenu/Transfer/t_Remove", connected);
  set_menu_sensitive ("/MainMenu/Transfer/t_Up", connected);
  set_menu_sensitive ("/MainMenu/Transfer/t_Down", connected);
  set_menu_sensitive ("/MainMenu/Transfer/t_Retrieve", connected);
  set_menu_sensitive ("/MainMenu/Transfer/t_Put", connected);
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
  gtk_combo_box_set_popdown_strings (GTK_COMBO_BOX_TEXT (widget), *history);
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

char *
MakeEditDialog (char *diagtxt, char *infotxt, char *deftext, int passwd_item, char *checktext, const gchar * yes_text, int * checked)
{
  gint size;
  PangoFontDescription *font_desc;
  GtkStyleContext *context;
  GtkStateFlags state;
  GtkWidget * hbox, * content_area, * dialog, * label, * image, * secondary_label, * message_area;
  GtkWidget * edit, * checkbox;
  char * ret;

  dialog = gtk_dialog_new_with_buttons (diagtxt, GTK_WINDOW(window), 0,
                                        GTK_STOCK_CANCEL, GTK_RESPONSE_NO,
                                        yes_text, GTK_RESPONSE_YES,
                                        NULL);

  content_area = gtk_dialog_get_content_area(GTK_DIALOG (dialog));

  gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);
  gtk_window_set_skip_taskbar_hint (GTK_WINDOW (dialog), TRUE);

  image = gtk_image_new_from_stock (GTK_STOCK_DIALOG_QUESTION, GTK_ICON_SIZE_DIALOG);
  gtk_widget_set_halign (image, GTK_ALIGN_CENTER);
  gtk_widget_set_valign (image, GTK_ALIGN_START);
  gtk_widget_show (image);

  label = gtk_label_new (diagtxt);
  gtk_label_set_line_wrap  (GTK_LABEL (label), TRUE);
  gtk_label_set_selectable (GTK_LABEL (label), TRUE);
  gtk_widget_set_halign (label, GTK_ALIGN_START);
  gtk_widget_set_valign (label, GTK_ALIGN_START);
  context = gtk_widget_get_style_context (label);
  state = gtk_widget_get_state_flags (label);

  size = pango_font_description_get_size (gtk_style_context_get_font (context, state));
  font_desc = pango_font_description_new ();
  pango_font_description_set_weight (font_desc, PANGO_WEIGHT_BOLD);
  pango_font_description_set_size (font_desc, size * PANGO_SCALE_LARGE);
  gtk_widget_override_font (label, font_desc);
  pango_font_description_free (font_desc);
  gtk_widget_show (label);

  secondary_label = gtk_label_new (infotxt);
  gtk_label_set_line_wrap  (GTK_LABEL (secondary_label), TRUE);
  gtk_label_set_selectable (GTK_LABEL (secondary_label), TRUE);
  gtk_widget_set_halign (secondary_label, GTK_ALIGN_START);
  gtk_widget_set_valign (secondary_label, GTK_ALIGN_START);
  gtk_widget_show (secondary_label);

  edit = gtk_entry_new ();
  gtk_widget_grab_focus (edit);
  gtk_entry_set_visibility (GTK_ENTRY (edit), passwd_item);
  if (deftext != NULL)
    {
      gtk_entry_set_text (GTK_ENTRY (edit), deftext);
      gtk_editable_select_region (GTK_EDITABLE (edit), 0, strlen (deftext));
    }
  gtk_widget_show (edit);

  message_area = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_box_pack_start (GTK_BOX (message_area), label, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (message_area), secondary_label, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (message_area), edit, TRUE, TRUE, 0);
  if (checktext != NULL)
    {
      checkbox = gtk_check_button_new_with_label (checktext);
      gtk_box_pack_start (GTK_BOX (message_area), checkbox, TRUE, TRUE, 0);
      gtk_widget_show (checkbox);
    }
  gtk_widget_show (message_area);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
  gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), message_area, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (content_area), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  gtk_container_set_border_width (GTK_CONTAINER (dialog), 5);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 5);
  gtk_box_set_spacing (GTK_BOX (content_area), 14);

  gtk_dialog_set_default_response (GTK_DIALOG(dialog), GTK_RESPONSE_YES);
  gint response = gtk_dialog_run (GTK_DIALOG(dialog));
  if (response == GTK_RESPONSE_YES)
    {
      ret = g_strdup(gtk_entry_get_text (GTK_ENTRY (edit)));
    }
  else
    {
      ret = NULL;
    }

  if (checktext != NULL)
    {
      *checked = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON (checkbox));
    }
  gtk_widget_destroy (dialog);

  return ret;
}


int
MakeYesNoDialog (char *diagtxt, char *infotxt)
{
  gint size;
  PangoFontDescription *font_desc;
  GtkStyleContext *context;
  GtkStateFlags state;
  GtkWidget * hbox, * content_area, * dialog, * label, * image, * secondary_label, * message_area;

  dialog = gtk_dialog_new_with_buttons (diagtxt, GTK_WINDOW(window), 0,
                                        GTK_STOCK_NO, GTK_RESPONSE_NO,
                                        GTK_STOCK_YES, GTK_RESPONSE_YES,
                                        NULL);

  content_area = gtk_dialog_get_content_area(GTK_DIALOG (dialog));

  gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);
  gtk_window_set_skip_taskbar_hint (GTK_WINDOW (dialog), TRUE);

  image = gtk_image_new_from_stock (GTK_STOCK_DIALOG_QUESTION, GTK_ICON_SIZE_DIALOG);
  gtk_widget_set_halign (image, GTK_ALIGN_CENTER);
  gtk_widget_set_valign (image, GTK_ALIGN_START);
  gtk_widget_show (image);

  label = gtk_label_new (diagtxt);
  gtk_label_set_line_wrap  (GTK_LABEL (label), TRUE);
  gtk_label_set_selectable (GTK_LABEL (label), TRUE);
  gtk_widget_set_halign (label, GTK_ALIGN_START);
  gtk_widget_set_valign (label, GTK_ALIGN_START);
  context = gtk_widget_get_style_context (label);
  state = gtk_widget_get_state_flags (label);
  size = pango_font_description_get_size (gtk_style_context_get_font (context, state));
  font_desc = pango_font_description_new ();
  pango_font_description_set_weight (font_desc, PANGO_WEIGHT_BOLD);
  pango_font_description_set_size (font_desc, size * PANGO_SCALE_LARGE);
  gtk_widget_override_font (label, font_desc);
  pango_font_description_free (font_desc);
  gtk_widget_show (label);

  secondary_label = gtk_label_new (infotxt);
  gtk_label_set_line_wrap  (GTK_LABEL (secondary_label), TRUE);
  gtk_label_set_selectable (GTK_LABEL (secondary_label), TRUE);
  gtk_widget_set_halign (secondary_label, GTK_ALIGN_START);
  gtk_widget_set_valign (secondary_label, GTK_ALIGN_START);
  gtk_widget_show (secondary_label);

  message_area = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_box_pack_start (GTK_BOX (message_area), label, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (message_area), secondary_label, TRUE, TRUE, 0);
  gtk_widget_show (message_area);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
  gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), message_area, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (content_area), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  gtk_container_set_border_width (GTK_CONTAINER (dialog), 5);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 5);
  gtk_box_set_spacing (GTK_BOX (content_area), 14);

  gtk_dialog_set_default_response (GTK_DIALOG(dialog), GTK_RESPONSE_YES);
  gint response = gtk_dialog_run (GTK_DIALOG(dialog));
  gtk_widget_destroy (dialog);

  return response == GTK_RESPONSE_YES;
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
  transfer->cancel = 1;
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
      gtk_window_set_decorated (GTK_WINDOW (dialog), 0);
      gtk_grab_add (dialog);
      gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(window));
      gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
      g_signal_connect (G_OBJECT (dialog), "delete_event",
                          G_CALLBACK (delete_event), NULL);
      gtk_window_set_title (GTK_WINDOW (dialog),
                _("Getting directory listings"));

      vbox = gtk_vbox_new (FALSE, 5);
      gtk_container_set_border_width (GTK_CONTAINER (vbox), 10);
      gtk_container_add (GTK_CONTAINER (dialog), vbox);
      gtk_widget_show (vbox);

      textwid = gtk_label_new (NULL);
      gtk_box_pack_start (GTK_BOX (vbox), textwid, TRUE, TRUE, 0);
      gtk_widget_show (textwid);

      statuswid = gtk_progress_bar_new ();
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

  tdata = data;

  update_directory_download_progress (tdata);

  gtk_progress_bar_pulse (GTK_PROGRESS_BAR (statuswid));

  return (1);
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

void
gtk_combo_box_set_popdown_strings (GtkComboBoxText * combo, GList * string)
{
  gtk_combo_box_text_remove_all (combo);
  while (string != NULL)
    {
      gtk_combo_box_text_append_text (combo, string->data);
      string = string->next;
    }
}
