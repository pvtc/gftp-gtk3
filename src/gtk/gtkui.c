/*****************************************************************************/
/*  gtkui.c - GTK+ UI related functions for gFTP                             */
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
gftpui_refresh (void *uidata, int clear_cache_entry)
{
  gftp_window_data * wdata;

  wdata = uidata;
  wdata->request->refreshing = 1;

  if (!check_status (_("Refresh"), wdata, 0, 0, 0, 1))
    {
      wdata->request->refreshing = 0;
      return;
    }

  if (clear_cache_entry)
    gftp_delete_cache_entry (wdata->request, NULL, 0);

  if (check_reconnect (wdata) < 0)
    {
      wdata->request->refreshing = 0;
      return;
    }

  remove_files_window (wdata);
  ftp_list_files (wdata);

  wdata->request->refreshing = 0;
}

static int
gftpui_common_run_mkdir (gftpui_callback_data * cdata)
{
  return (gftp_make_directory (cdata->request, cdata->input_string));
}

void
gftpui_mkdir_dialog (GtkAction * a, gpointer data)
{
  gftpui_callback_data * cdata;
  gftp_window_data * wdata;

  wdata = data;
  cdata = g_malloc0 (sizeof (*cdata));
  cdata->request = wdata->request;
  cdata->uidata = wdata;
  cdata->run_function = gftpui_common_run_mkdir;

  if (!check_status (_("Mkdir"), wdata, gftpui_common_use_threads (wdata->request), 0, 0, wdata->request->mkdir != NULL))
    return;

  cdata->input_string = MakeEditDialog (_("Make Directory"), _("Enter name of directory to create"),
                  NULL, 1, NULL, GTK_STOCK_ADD, NULL);
  if (cdata->input_string != NULL)
    {
      gftpui_common_run_callback_function (cdata);
      g_free (cdata->input_string);
    }
  g_free (cdata);
}

static int
gftpui_common_run_rename (gftpui_callback_data * cdata)
{
  return (gftp_rename_file (cdata->request, cdata->source_string,
                            cdata->input_string));
}

void
gftpui_rename_dialog (GtkAction * a, gpointer data)
{
  gftpui_callback_data * cdata;
  GList *templist;
  gftp_window_data * wdata;
  gftp_file * curfle;
  char *tempstr;
  GtkTreeSelection *select;
  GtkTreeIter iter;
  GtkTreeModel * model;

  wdata = data;
  cdata = g_malloc0 (sizeof (*cdata));
  cdata->request = wdata->request;
  cdata->uidata = wdata;
  cdata->run_function = gftpui_common_run_rename;

  if (!check_status (_("Rename"), wdata, gftpui_common_use_threads (wdata->request), 1, 1, wdata->request->rename != NULL))
    return;

  select = gtk_tree_view_get_selection (GTK_TREE_VIEW (wdata->listbox));
  templist = gtk_tree_selection_get_selected_rows(select, &model);
  gtk_tree_model_get_iter(model, &iter, (GtkTreePath*)templist->data);
  gtk_tree_model_get(model, &iter, 0, &curfle, -1);
  g_list_foreach (templist, (GFunc) gtk_tree_path_free, NULL);
  g_list_free (templist);

  cdata->source_string = g_strdup (curfle->file);

  tempstr = g_strdup_printf (_("What would you like to rename %s to?"),
                             cdata->source_string);

  cdata->input_string = MakeEditDialog (_("Rename"), tempstr, cdata->source_string, 1, NULL, _("Rename"), NULL);
  g_free (tempstr);

  if (cdata->input_string != NULL)
    {
      gftpui_common_run_callback_function (cdata);
      g_free (cdata->input_string);
    }
  g_free (cdata->source_string);
  g_free (cdata);
}

static int
gftpui_common_run_site (gftpui_callback_data * cdata)
{
  return (gftp_site_cmd (cdata->request, cdata->toggled, cdata->input_string));
}

void
gftpui_site_dialog (GtkAction * a, gpointer data)
{
  gftpui_callback_data * cdata;
  gftp_window_data * wdata;

  wdata = data;
  cdata = g_malloc0 (sizeof (*cdata));
  cdata->request = wdata->request;
  cdata->uidata = wdata;
  cdata->run_function = gftpui_common_run_site;

  if (!check_status (_("Site"), wdata, 0, 0, 0, wdata->request->site != NULL))
    return;

  cdata->input_string = MakeEditDialog (_("Site"), _("Enter site-specific command"), NULL, 1,
                  _("Prepend with SITE"), GTK_STOCK_OK, &(cdata->toggled));
  if (cdata->input_string != NULL)
  {
    gftpui_common_run_callback_function (cdata);
    g_free (cdata->input_string);
  }

  g_free (cdata);
}

static int
gftpui_common_run_chdir (gftpui_callback_data * cdata)
{
  return (gftp_set_directory (cdata->request, cdata->input_string));
}

int
gftpui_run_chdir (gpointer uidata, char *directory)
{
  gftpui_callback_data * cdata;
  gftp_window_data * wdata;
  char *tempstr;
  int ret;

  wdata = uidata;
  if ((tempstr = gftp_expand_path (wdata->request, directory)) == NULL)
    return (FALSE);

  cdata = g_malloc0 (sizeof (*cdata));
  cdata->request = wdata->request;
  cdata->uidata = wdata;
  cdata->run_function = gftpui_common_run_chdir;
  cdata->input_string = tempstr;
  cdata->dont_clear_cache = 1;

  ret = gftpui_common_run_callback_function (cdata);

  g_free(tempstr);
  g_free (cdata);
  return (ret);
}


void
gftpui_chdir_dialog (GtkAction * a, gpointer data)
{
  GList *templist;
  gftp_window_data * wdata;
  gftp_file * curfle;
  char *tempstr;
  GtkTreeSelection *select;
  GtkTreeIter iter;
  GtkTreeModel * model;

  wdata = data;
  if (!check_status (_("Chdir"), wdata, gftpui_common_use_threads (wdata->request), 1, 0,
                     wdata->request->chdir != NULL))
    return;

  select = gtk_tree_view_get_selection (GTK_TREE_VIEW (wdata->listbox));
  templist = gtk_tree_selection_get_selected_rows(select, &model);
  gtk_tree_model_get_iter(model, &iter, (GtkTreePath*)templist->data);
  gtk_tree_model_get(model, &iter, 0, &curfle, -1);
  g_list_foreach (templist, (GFunc) gtk_tree_path_free, NULL);
  g_list_free (templist);

  tempstr = gftp_build_path (wdata->request, wdata->request->directory,
                             curfle->file, NULL);
  gftpui_run_chdir (wdata, tempstr);
  g_free (tempstr);
}

void
gftpui_disconnect (void * uidata)
{
  gftp_window_data * wdata;

  wdata = uidata;
  gftp_delete_cache_entry (wdata->request, NULL, 1);
  gftp_disconnect (wdata->request);
  remove_files_window (wdata);

  /* Free the request structure so that all old settings are purged. */
  gftp_request_destroy (wdata->request, 0);
  gftp_gtk_init_request (wdata);

  update_window_info ();
}


char *
gftpui_gtk_get_utf8_file_pos (gftp_file * fle)
{
  char *pos;

  if ((pos = strrchr (fle->file, '/')) != NULL)
    pos++;
  else
    pos = fle->file;

  return (pos);
}

int
gftpui_protocol_ask_yes_no (gftp_request * request, char *title,
                            char *question)
{
  int answer;

  GDK_THREADS_ENTER ();
  answer = MakeYesNoDialog (title, question);
  GDK_THREADS_LEAVE ();

  return (answer);
}

char *
gftpui_protocol_ask_user_input (gftp_request * request, char *title,
                                char *question, int shown)
{
  char * text;

  GDK_THREADS_ENTER ();
  text = MakeEditDialog (title, question, NULL, shown, NULL, GTK_STOCK_OK, NULL);
  GDK_THREADS_LEAVE ();

  return text;
}

static int
gftpui_common_run_connect (gftpui_callback_data * cdata)
{
  return (gftp_connect (cdata->request));
}

static void
gftpui_prompt_username (void *uidata, gftp_request * request)
{
  char * text;

  text = MakeEditDialog (_("Enter Username"),
                  _("Please enter your username for this site"), NULL,
                  1, NULL, _("Connect"), NULL);
  if (text != NULL)
    {
      gftp_set_username (request, text);
      g_free(text);
    }
}

static void
gftpui_prompt_password (void *uidata, gftp_request * request)
{
  char * text;

  text = MakeEditDialog (_("Enter Password"),
                  _("Please enter your password for this site"), NULL,
                  0, NULL, _("Connect"), NULL);
  if (text != NULL)
    {
      gftp_set_password (request, text);
      g_free(text);
    }
}

int
gftpui_common_cmd_open (void *uidata, gftp_request * request,
                        void *other_uidata, gftp_request * other_request,
                        const char *command)
{
  GtkWidget * toplevel = gtk_widget_get_toplevel (openurl_btn);
  GdkDisplay * display = gtk_widget_get_display (toplevel);
  gftpui_callback_data * cdata;
  intptr_t retries;

  if (GFTP_IS_CONNECTED (request))
    gftpui_disconnect (uidata);

  if (command != NULL)
    {
      if (*command == '\0')
        {
          request->logging_function (gftp_logging_error, request,
                                     _("usage: open " GFTP_URL_USAGE "\n"));
          return (1);
        }

      if (gftp_parse_url (request, command) < 0)
        return (1);
    }

  if (gftp_need_username (request))
    gftpui_prompt_username (uidata, request);

  if (gftp_need_password (request))
    gftpui_prompt_password (uidata, request);

  gftp_lookup_request_option (request, "retries", &retries);

  cdata = g_malloc0 (sizeof (*cdata));
  cdata->request = request;
  cdata->uidata = uidata;
  cdata->run_function = gftpui_common_run_connect;
  cdata->retries = retries;
  cdata->dont_check_connection = 1;

  if (request->refreshing)
    cdata->dont_refresh = 1;

  GdkCursor * busyCursor = gdk_cursor_new_for_display (display, GDK_WATCH);
  gdk_window_set_cursor (gtk_widget_get_window(toplevel), busyCursor);

  gftpui_common_run_callback_function (cdata);

  gdk_window_set_cursor (gtk_widget_get_window(toplevel), NULL);
  gdk_cursor_unref (busyCursor);

  g_free (cdata);

  return (1);
}

gftp_transfer *
gftpui_common_add_file_transfer (gftp_request * fromreq, gftp_request * toreq,
                                 void *fromuidata, void *touidata,
                                 GList * files)
{
  intptr_t append_transfers, one_transfer, overwrite_default;
  GList * templist, *curfle;
  gftp_transfer * tdata;
  gftp_file * tempfle;
  int show_dialog;

  gftp_lookup_request_option (fromreq, "overwrite_default", &overwrite_default);
  gftp_lookup_request_option (fromreq, "append_transfers", &append_transfers);
  gftp_lookup_request_option (fromreq, "one_transfer", &one_transfer);

  if (!overwrite_default)
    {
      for (templist = files; templist != NULL; templist = templist->next)
        {
          tempfle = templist->data;
          if (tempfle->startsize > 0 && !S_ISDIR (tempfle->st_mode))
            break;
        }

      show_dialog = templist != NULL;
    }
  else
    show_dialog = 0;

  tdata = NULL;
  if (append_transfers && one_transfer && !show_dialog)
    {
      if (g_thread_supported ())
        g_static_mutex_lock (&gftpui_common_transfer_mutex);

      for (templist = gftp_file_transfers;
           templist != NULL;
           templist = templist->next)
        {
          tdata = templist->data;

          if (g_thread_supported ())
            g_static_mutex_lock (&tdata->structmutex);

          if (!compare_request (tdata->fromreq, fromreq, 0) ||
              !compare_request (tdata->toreq, toreq, 0) ||
              tdata->curfle == NULL)
            {
              if (g_thread_supported ())
                g_static_mutex_unlock (&tdata->structmutex);

              continue;
            }

          tdata->files = g_list_concat (tdata->files, files);

          for (curfle = files; curfle != NULL; curfle = curfle->next)
            {
              tempfle = curfle->data;

              if (S_ISDIR (tempfle->st_mode))
                tdata->numdirs++;
              else
                tdata->numfiles++;

              if (tempfle->transfer_action != GFTP_TRANS_ACTION_SKIP)
                tdata->total_bytes += tempfle->size;

              gftpui_add_file_to_transfer (tdata, curfle);
            }

          if (g_thread_supported ())
            g_static_mutex_unlock (&tdata->structmutex);

          break;
        }

      if (g_thread_supported ())
        g_static_mutex_unlock (&gftpui_common_transfer_mutex);
    }
  else
    templist = NULL;

  if (templist == NULL)
    {
      tdata = gftp_tdata_new ();
      tdata->fromreq = gftp_copy_request (fromreq);
      tdata->toreq = gftp_copy_request (toreq);

      tdata->fromwdata = fromuidata;
      tdata->towdata = touidata;

      if (!show_dialog)
        tdata->show = tdata->ready = 1;

      tdata->files = files;
      for (curfle = files; curfle != NULL; curfle = curfle->next)
        {
          tempfle = curfle->data;
          if (S_ISDIR (tempfle->st_mode))
            tdata->numdirs++;
          else
            tdata->numfiles++;

          if (tempfle->transfer_action != GFTP_TRANS_ACTION_SKIP)
            tdata->total_bytes += tempfle->size;
        }

      if (g_thread_supported ())
        g_static_mutex_lock (&gftpui_common_transfer_mutex);

      gftp_file_transfers = g_list_append (gftp_file_transfers, tdata);

      if (g_thread_supported ())
        g_static_mutex_unlock (&gftpui_common_transfer_mutex);

      if (show_dialog)
        gftpui_ask_transfer (tdata);
    }

  return (tdata);
}

void
gftpui_common_cancel_file_transfer (gftp_transfer * tdata)
{
  g_static_mutex_lock (&tdata->structmutex);

  if (tdata->started)
    {
      tdata->cancel = 1;
      tdata->fromreq->cancel = 1;
      tdata->toreq->cancel = 1;
      tdata->skip_file = 0;
    }
  else
    tdata->done = 1;

  tdata->fromreq->stopable = 0;
  tdata->toreq->stopable = 0;

  g_static_mutex_unlock (&tdata->structmutex);

  tdata->fromreq->logging_function (gftp_logging_misc, tdata->fromreq,
                                    _("Stopping the transfer on host %s\n"),
                                    tdata->toreq->hostname);
}
