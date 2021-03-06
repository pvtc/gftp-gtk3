/*****************************************************************************/
/*  transfer.c - functions to handle transfering files                       */
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

static int num_transfers_in_progress = 0;

static int
gftpui_common_run_ls (gftpui_callback_data * cdata)
{
  int got, matched_filespec, have_dotdot, ret;
  char *sortcol_var, *sortasds_var;
  intptr_t sortcol, sortasds;
  gftp_file * fle;

  ret = gftp_list_files (cdata->request);
  if (ret < 0)
    return (ret);

  have_dotdot = 0;
  cdata->request->gotbytes = 0;
  cdata->files = NULL;
  fle = g_malloc0 (sizeof (*fle));
  while ((got = gftp_get_next_file (cdata->request, NULL, fle)) > 0 ||
         got == GFTP_ERETRYABLE)
    {
      if (cdata->source_string == NULL)
        matched_filespec = 1;
      else
        matched_filespec = gftp_match_filespec (cdata->request, fle->file,
                                                cdata->source_string);

      if (got < 0 || strcmp (fle->file, ".") == 0 || !matched_filespec)
        {
          gftp_file_destroy (fle, 0);
          continue;
        }
      else if (strcmp (fle->file, "..") == 0)
        have_dotdot = 1;

      cdata->request->gotbytes += got;
      cdata->files = g_list_prepend (cdata->files, fle);
      fle = g_malloc0 (sizeof (*fle));
    }
  g_free (fle);

  gftp_end_transfer (cdata->request);
  cdata->request->gotbytes = -1;

  if (!have_dotdot)
    {
      fle = g_malloc0 (sizeof (*fle));
      fle->file = g_strdup ("..");
      fle->user = g_malloc0 (1);
      fle->group = g_malloc0 (1);
      fle->st_mode = S_IFDIR | S_IRUSR | S_IWUSR | S_IXUSR;
      cdata->files = g_list_prepend (cdata->files, fle);
    }

  if (cdata->files != NULL)
    {
      if (cdata->request->protonum == GFTP_LOCAL_NUM)
        {
          sortasds_var = "local_sortasds";
          sortcol_var = "local_sortcol";
        }
      else
        {
          sortasds_var = "remote_sortasds";
          sortcol_var = "remote_sortcol";
        }

      gftp_lookup_global_option (sortcol_var, &sortcol);
      gftp_lookup_global_option (sortasds_var, &sortasds);

      cdata->files = gftp_sort_filelist (cdata->files, sortcol, sortasds);
    }

  return (1);
}

int
ftp_list_files (gftp_window_data * wdata)
{
  gftpui_callback_data * cdata;

  gtk_label_set_text (GTK_LABEL (wdata->hoststxt), _("Receiving file names..."));
  cdata = g_malloc0 (sizeof (*cdata));
  cdata->request = wdata->request;
  cdata->uidata = wdata;
  cdata->run_function = gftpui_common_run_ls;
  cdata->dont_refresh = 1;

  gftpui_common_run_callback_function (cdata);

  wdata->files = cdata->files;
  g_free (cdata);

  if (wdata->files == NULL || !GFTP_IS_CONNECTED (wdata->request))
    {
      gftpui_disconnect (wdata);
      return (0);
    }

  wdata->sorted = 0;

  sortrows (NULL, (gpointer) wdata);

  GtkTreeSelection *select;
  select = gtk_tree_view_get_selection (GTK_TREE_VIEW (wdata->listbox));
  if (gtk_tree_selection_count_selected_rows(select) == 0)
  {
    GtkTreeIter iter;
    GtkTreeModel * model = gtk_tree_view_get_model(GTK_TREE_VIEW (wdata->listbox));
    if (gtk_tree_model_get_iter_first(model, &iter))
      gtk_tree_selection_select_iter(select, &iter);
  }

  return (1);
}


int
ftp_connect (gftp_window_data * wdata, gftp_request * request)
{
  if (wdata->request == request)
    gtk_label_set_text (GTK_LABEL (wdata->hoststxt), _("Connecting..."));

  return (gftpui_common_cmd_open (wdata, request, NULL, NULL, NULL));
}


void
get_files (GtkAction * a, gpointer data)
{
  transfer_window_files (&window2, &window1);
}


void
put_files (GtkAction * a, gpointer data)
{
  transfer_window_files (&window1, &window2);
}


void
transfer_window_files (gftp_window_data * fromwdata, gftp_window_data * towdata)
{
  gftp_file * tempfle, * newfle;
  GList * templist, * filelist;
  gftp_transfer * transfer;
  int ret, disconnect;

  if (!check_status (_("Transfer Files"), fromwdata, 1, 0, 1,
       towdata->request->put_file != NULL && fromwdata->request->get_file != NULL))
    return;

  if (!GFTP_IS_CONNECTED (fromwdata->request) ||
      !GFTP_IS_CONNECTED (towdata->request))
    {
      ftp_log (gftp_logging_error, NULL,
               _("Retrieve Files: Not connected to a remote site\n"));
      return;
    }

  if (check_reconnect (fromwdata) < 0 || check_reconnect (towdata) < 0)
    return;

  transfer = g_malloc0 (sizeof (*transfer));
  transfer->fromreq = gftp_copy_request (fromwdata->request);
  transfer->toreq = gftp_copy_request (towdata->request);
  transfer->fromwdata = fromwdata;
  transfer->towdata = towdata;

  GtkTreeSelection *select;
  GtkTreeIter iter;
  GtkTreeModel * model;
  select = gtk_tree_view_get_selection (GTK_TREE_VIEW (fromwdata->listbox));
  templist = gtk_tree_selection_get_selected_rows(select, &model);
  for (filelist = templist ; filelist != NULL; filelist = g_list_next(filelist))
  {
    gtk_tree_model_get_iter(model, &iter, (GtkTreePath*)filelist->data);
    gtk_tree_model_get(model, &iter, 0, &tempfle, -1);
    if (strcmp (tempfle->file, "..") != 0)
    {
      newfle = copy_fdata (tempfle);
      transfer->files = g_list_append (transfer->files, newfle);
    }
  }
  g_list_foreach (templist, (GFunc) gtk_tree_path_free, NULL);
  g_list_free (templist);

  if (transfer->files != NULL)
    {
      gftp_swap_socks (transfer->fromreq, fromwdata->request);
      gftp_swap_socks (transfer->toreq, towdata->request);

      ret = gftp_gtk_get_subdirs (transfer);
      if (ret < 0)
        disconnect = 1;
      else
        disconnect = 0;

      if (!GFTP_IS_CONNECTED (transfer->fromreq))
        {
          gftpui_disconnect (fromwdata);
          disconnect = 1;
        }

      if (!GFTP_IS_CONNECTED (transfer->toreq))
        {
          gftpui_disconnect (towdata);
          disconnect = 1;
        }

      if (disconnect)
        {
          free_tdata (transfer);
          return;
        }

      gftp_swap_socks (fromwdata->request, transfer->fromreq);
      gftp_swap_socks (towdata->request, transfer->toreq);
    }

  if (transfer->files != NULL)
    {
      gftpui_common_add_file_transfer (transfer->fromreq, transfer->toreq,
                                       transfer->fromwdata, transfer->towdata,
                                       transfer->files);
      g_free (transfer);
    }
  else
    free_tdata (transfer);
}


static int
_gftp_getdir_thread (gftpui_callback_data * cdata)
{
  return (gftp_get_all_subdirs (cdata->user_data, NULL));
}


int
gftp_gtk_get_subdirs (gftp_transfer * transfer)
{
  gftpui_callback_data * cdata;
  long numfiles, numdirs;
  guint timeout_num;
  int ret;

  cdata = g_malloc0 (sizeof (*cdata));
  cdata->user_data = transfer;
  cdata->uidata = transfer->fromwdata;
  cdata->request = ((gftp_window_data *) transfer->fromwdata)->request;
  cdata->run_function = _gftp_getdir_thread;
  cdata->dont_check_connection = 1;
  cdata->dont_refresh = 1;

  timeout_num = g_timeout_add (100, progress_timeout, transfer);
  ret = gftpui_common_run_callback_function (cdata);
  g_source_remove (timeout_num);

  numfiles = transfer->numfiles;
  numdirs = transfer->numdirs;
  transfer->numfiles = transfer->numdirs = -1;
  update_directory_download_progress (transfer);
  transfer->numfiles = numfiles;
  transfer->numdirs = numdirs;

  g_free (cdata);

  return (ret);
}


static void
remove_file (gftp_viewedit_data * ve_proc)
{
  if (ve_proc->remote_filename == NULL)
    return;

  if (unlink (ve_proc->filename) == 0)
    ftp_log (gftp_logging_misc, NULL, _("Successfully removed %s\n"),
             ve_proc->filename);
  else
    ftp_log (gftp_logging_error, NULL,
             _("Error: Could not remove file %s: %s\n"), ve_proc->filename,
             g_strerror (errno));
}


static void
free_edit_data (gftp_viewedit_data * ve_proc)
{
  int i;

  if (ve_proc->torequest)
    gftp_request_destroy (ve_proc->torequest, 1);
  if (ve_proc->filename)
    g_free (ve_proc->filename);
  if (ve_proc->remote_filename)
    g_free (ve_proc->remote_filename);
  for (i = 0; ve_proc->argv[i] != NULL; i++)
    g_free (ve_proc->argv[i]);
  g_free (ve_proc->argv);
  g_free (ve_proc);
}

static int
_check_viewedit_process_status (gftp_viewedit_data * ve_proc, int ret)
{
  int procret;

  if (WIFEXITED (ret))
    {
      procret = WEXITSTATUS (ret);
      if (procret != 0)
        {
          ftp_log (gftp_logging_error, NULL,
                   _("Error: Child %d returned %d\n"), ve_proc->pid, procret);
          if (ve_proc->view)
            remove_file (ve_proc);

          return (0);
        }
      else
        {
          ftp_log (gftp_logging_misc, NULL,
                   _("Child %d returned successfully\n"), ve_proc->pid);
          return (1);
        }
    }
  else
    {
      ftp_log (gftp_logging_error, NULL,
               _("Error: Child %d did not terminate properly\n"),
               ve_proc->pid);
      return (0);
    }
}


static int
_prompt_to_upload_edited_file (gftp_viewedit_data * ve_proc)
{
  struct stat st;
  char *str;

  if (stat (ve_proc->filename, &st) == -1)
    {
      ftp_log (gftp_logging_error, NULL,
               _("Error: Cannot get information about file %s: %s\n"),
           ve_proc->filename, g_strerror (errno));
      return (0);
    }
  else if (st.st_mtime == ve_proc->st.st_mtime)
    {
      ftp_log (gftp_logging_misc, NULL, _("File %s was not changed\n"),
           ve_proc->filename);
      remove_file (ve_proc);
      return (0);
    }
  else
    {
      memcpy (&ve_proc->st, &st, sizeof (ve_proc->st));
      str = g_strdup_printf (_("File %s has changed.\nWould you like to upload it?"),
                             ve_proc->remote_filename);

      if (MakeYesNoDialog (_("Edit File"), str))
        {
          gftp_transfer * tdata;
          gftp_file * tempfle;
          GList * newfile;

          tempfle = g_malloc0 (sizeof (*tempfle));
          tempfle->destfile = gftp_build_path (ve_proc->torequest,
                                               ve_proc->torequest->directory,
                                               ve_proc->remote_filename, NULL);
          ve_proc->remote_filename = NULL;
          tempfle->file = ve_proc->filename;
          ve_proc->filename = NULL;
          tempfle->done_rm = 1;
          newfile = g_list_append (NULL, tempfle);
          tdata = gftpui_common_add_file_transfer (ve_proc->fromwdata->request,
                                                   ve_proc->torequest,
                                                   ve_proc->fromwdata,
                                                   ve_proc->towdata, newfile);
          free_edit_data (ve_proc);

          if (tdata != NULL)
            tdata->conn_error_no_timeout = 1;
        }
      else
        {
          remove_file (ve_proc);
          free_edit_data (ve_proc);
        }
      g_free (str);
      return (1);
    }
}

static void
do_check_done_process (pid_t pid, int ret)
{
  gftp_viewedit_data * ve_proc;
  GList * curdata, *deldata;
  int ok;

  curdata = viewedit_processes;
  while (curdata != NULL)
    {
      ve_proc = curdata->data;
      if (ve_proc->pid != pid)
        continue;

      deldata = curdata;
      curdata = curdata->next;

      viewedit_processes = g_list_remove_link (viewedit_processes,
                                               deldata);

      ok = _check_viewedit_process_status (ve_proc, ret);
      if (!ve_proc->view && ve_proc->dontupload)
        gftpui_refresh (ve_proc->fromwdata, 1);

      if (ok && !ve_proc->view && !ve_proc->dontupload)
    {
      /* We were editing the file. Upload it */
          if (_prompt_to_upload_edited_file (ve_proc))
            break; /* Don't free the ve_proc structure */
        }
      else if (ve_proc->view && ve_proc->rm)
        {
      /* After viewing the file delete the tmp file */
      remove_file (ve_proc);
    }

      free_edit_data (ve_proc);
      break;
    }
}


static void
check_done_process (void)
{
  pid_t pid;
  int ret;

  gftpui_common_child_process_done = 0;
  while ((pid = waitpid (-1, &ret, WNOHANG)) > 0)
    {
      do_check_done_process (pid, ret);
    }
}


static void
on_next_transfer (gftp_transfer * tdata, GtkTreeModel * model, GtkTreeIter * iter)
{
  intptr_t refresh_files;
  gftp_file * tempfle;

  tdata->next_file = 0;
  for (; tdata->updfle != tdata->curfle; tdata->updfle = tdata->updfle->next)
    {
      tempfle = tdata->updfle->data;

      if (tempfle->done_view || tempfle->done_edit)
        {
          if (tempfle->transfer_action != GFTP_TRANS_ACTION_SKIP)
            {
              view_file (tempfle->destfile, 0, tempfle->done_view,
                         tempfle->done_rm, 1, 0, tempfle->file, NULL);
            }
        }
      else if (tempfle->done_rm)
    tdata->fromreq->rmfile (tdata->fromreq, tempfle->file);

      if (tempfle->transfer_action == GFTP_TRANS_ACTION_SKIP)
  gtk_tree_store_set(GTK_TREE_STORE(model), iter, 1, _("Skipped"), -1);
      else
  gtk_tree_store_set(GTK_TREE_STORE(model), iter, 1, _("Finished"), -1);
    }

  gftp_lookup_request_option (tdata->fromreq, "refresh_files", &refresh_files);

  if (refresh_files && tdata->curfle && tdata->curfle->next &&
      compare_request (tdata->toreq,
                       ((gftp_window_data *) tdata->towdata)->request, 1))
    gftpui_refresh (tdata->towdata, 1);
}

static void
show_transfer (gftp_transfer * tdata, GtkTreeModel * model, GtkTreeIter * iter)
{
  gftp_file * tempfle;
  GList * templist;
  char * status;
  GtkTreeIter sub;
  char * text;

  gtk_tree_store_append (GTK_TREE_STORE(model), iter, NULL);
  gtk_tree_store_set(GTK_TREE_STORE(model), iter,
    0, tdata->fromreq->hostname,
    1, _("Waiting..."),
    2, tdata,
    3, NULL,
    -1);
  tdata->show = 0;
  tdata->curfle = tdata->updfle = tdata->files;

  tdata->total_bytes = 0;
  for (templist = tdata->files; templist != NULL; templist = templist->next)
  {
    tempfle = templist->data;
    if (tempfle->transfer_action == GFTP_TRANS_ACTION_SKIP)
      status = _("Skipped");
    else
    {
      tdata->total_bytes += tempfle->size;
      status = _("Waiting...");
    }

    gtk_tree_store_append (GTK_TREE_STORE(model), &sub, iter);
    gtk_tree_store_set(GTK_TREE_STORE(model), &sub,
      0, gftpui_gtk_get_utf8_file_pos (tempfle),
      1, status,
      2, tdata,
      3, templist,
      -1);
  }

  if (!tdata->toreq->stopable && gftp_need_password (tdata->toreq))
    {
      tdata->toreq->stopable = 1;
      text = MakeEditDialog (_("Enter Password"),
              _("Please enter your password for this site"), NULL, 0,
              NULL, _("Connect"), NULL);
      if (text != NULL)
        {
          gftp_set_password (tdata->toreq, text);
          tdata->toreq->stopable = 0;
          g_free(text);
        }
      else
        {
          if (! tdata->toreq->stopable != 0)
            gftpui_common_cancel_file_transfer (tdata);
        }
    }
  if (!tdata->fromreq->stopable && gftp_need_password (tdata->fromreq))
    {
      tdata->fromreq->stopable = 1;
      text = MakeEditDialog (_("Enter Password"),
              _("Please enter your password for this site"), NULL, 0,
              NULL, _("Connect"), NULL);
      if (text != NULL)
      {
          gftp_set_password (tdata->fromreq, text);
          tdata->fromreq->stopable = 0;
          g_free(text);
      }
      else
      {
        if (! tdata->fromreq->stopable == 0)
           gftpui_common_cancel_file_transfer (tdata);
      }
    }
}

#define GFTP_IS_SAME_HOST_STOP_TRANS(wdata,trequest) \
  ((wdata) != NULL && (wdata)->request != NULL && \
  (wdata)->request->datafd < 0 && !(wdata)->request->always_connected && \
  (wdata)->request->cached && !(wdata)->request->stopable && \
  trequest->datafd > 0 && !trequest->always_connected && \
  compare_request (trequest, (wdata)->request, 0))

static void
transfer_done (GList * node, GtkTreeModel * model, GtkTreeIter * iter)
{
  gftp_transfer * tdata;

  tdata = node->data;
  if (tdata->started)
    {
      if (GFTP_IS_SAME_HOST_STOP_TRANS ((gftp_window_data *) tdata->fromwdata,
                                         tdata->fromreq))
        {
          gftp_copy_param_options (((gftp_window_data *) tdata->fromwdata)->request, tdata->fromreq);

          gftp_swap_socks (((gftp_window_data *) tdata->fromwdata)->request,
                           tdata->fromreq);
        }
      else
    gftp_disconnect (tdata->fromreq);

      if (GFTP_IS_SAME_HOST_STOP_TRANS ((gftp_window_data *) tdata->towdata,
                                         tdata->toreq))
        {
          gftp_copy_param_options (((gftp_window_data *) tdata->towdata)->request, tdata->toreq);

          gftp_swap_socks (((gftp_window_data *) tdata->towdata)->request,
                           tdata->toreq);
        }
      else
    gftp_disconnect (tdata->toreq);

      if (tdata->towdata != NULL && compare_request (tdata->toreq,
                           ((gftp_window_data *) tdata->towdata)->request, 1))
        gftpui_refresh (tdata->towdata, 1);

      num_transfers_in_progress--;
    }

  if ((!tdata->show && tdata->started) ||
      (tdata->done && !tdata->started))
  {
    gtk_tree_store_remove(GTK_TREE_STORE(model), iter);
  }

  g_static_mutex_lock (&gftpui_common_transfer_mutex);

  node = g_list_find(gftp_file_transfers, tdata);
  gftp_file_transfers = g_list_remove_link (gftp_file_transfers, node);
  g_static_mutex_unlock (&gftpui_common_transfer_mutex);

  gdk_window_set_title (gtk_widget_get_parent_window (GTK_WIDGET(dlwdw)),
                        gftp_version);

  free_tdata (tdata);
}

#define GFTP_IS_SAME_HOST_START_TRANS(wdata,trequest) \
  ((wdata) != NULL && (wdata)->request != NULL && \
  (wdata)->request->datafd > 0 && !(wdata)->request->always_connected && \
  !(wdata)->request->stopable && \
  compare_request (trequest, (wdata)->request, 0))

static void
create_transfer (gftp_transfer * tdata, GtkTreeModel * model, GtkTreeIter * iter)
{
  GError * error;
  if (tdata->fromreq->stopable)
    return;

  if (GFTP_IS_SAME_HOST_START_TRANS ((gftp_window_data *) tdata->fromwdata,
                                     tdata->fromreq))
    {
      gftp_swap_socks (tdata->fromreq,
                       ((gftp_window_data *) tdata->fromwdata)->request);
      update_window (tdata->fromwdata);
    }

  if (GFTP_IS_SAME_HOST_START_TRANS ((gftp_window_data *) tdata->towdata,
                                     tdata->toreq))
    {
      gftp_swap_socks (tdata->toreq,
                       ((gftp_window_data *) tdata->towdata)->request);
      update_window (tdata->towdata);
    }

  num_transfers_in_progress++;
  tdata->started = 1;
  tdata->stalled = 1;

  gtk_tree_store_set(GTK_TREE_STORE(model), iter, 1, _("Connecting"), -1);

  g_thread_create (gftpui_common_transfer_files, tdata, FALSE, &error);
}


static void
_setup_dlstr (gftp_transfer * tdata, gftp_file * fle, char *dlstr,
              size_t dlstr_len)
{
  int hours, mins, secs, stalled, usesentdescr;
  unsigned long remaining_secs, lkbs;
  char gotstr[50], ofstr[50];
  struct timeval tv;

  stalled = 1;
  gettimeofday (&tv, NULL);
  usesentdescr = (tdata->fromreq->protonum == GFTP_LOCAL_NUM);

  insert_commas (fle->size, ofstr, sizeof (ofstr));
  insert_commas (tdata->curtrans + tdata->curresumed, gotstr, sizeof (gotstr));

  if (tv.tv_sec - tdata->lasttime.tv_sec <= 5)
    {
      remaining_secs = (fle->size - tdata->curtrans - tdata->curresumed) / 1024;

      lkbs = (unsigned long) tdata->kbs;
      if (lkbs > 0)
        remaining_secs /= lkbs;

      hours = remaining_secs / 3600;
      remaining_secs -= hours * 3600;
      mins = remaining_secs / 60;
      remaining_secs -= mins * 60;
      secs = remaining_secs;

      if (!(hours < 0 || mins < 0 || secs < 0))
        {
          stalled = 0;
          if (usesentdescr)
            {
              g_snprintf (dlstr, dlstr_len,
                          _("Sent %s of %s at %.2fKB/s, %02d:%02d:%02d est. time remaining"), gotstr, ofstr, tdata->kbs, hours, mins, secs);
            }
          else
            {
              g_snprintf (dlstr, dlstr_len,
                          _("Recv %s of %s at %.2fKB/s, %02d:%02d:%02d est. time remaining"), gotstr, ofstr, tdata->kbs, hours, mins, secs);
            }
        }
    }

  if (stalled)
    {
      tdata->stalled = 1;
      if (usesentdescr)
        {
          g_snprintf (dlstr, dlstr_len,
                      _("Sent %s of %s, transfer stalled, unknown time remaining"),
                      gotstr, ofstr);
        }
      else
        {
          g_snprintf (dlstr, dlstr_len,
                      _("Recv %s of %s, transfer stalled, unknown time remaining"),
                      gotstr, ofstr);
        }
    }
}


static void
update_file_status (gftp_transfer * tdata, GtkTreeModel * model, GtkTreeIter * iter)
{
  char totstr[150], winstr[150], dlstr[150];
  unsigned long remaining_secs, lkbs;
  int hours, mins, secs, pcent;
  intptr_t show_trans_in_title;
  gftp_file * tempfle;

  g_static_mutex_lock (&tdata->statmutex);
  tempfle = tdata->curfle->data;

  remaining_secs = (tdata->total_bytes - tdata->trans_bytes - tdata->resumed_bytes) / 1024;

  lkbs = (unsigned long) tdata->kbs;
  if (lkbs > 0)
    remaining_secs /= lkbs;

  hours = remaining_secs / 3600;
  remaining_secs -= hours * 3600;
  mins = remaining_secs / 60;
  remaining_secs -= mins * 60;
  secs = remaining_secs;

  if (hours < 0 || mins < 0 || secs < 0)
    {
      g_static_mutex_unlock (&tdata->statmutex);
      return;
    }

  if ((double) tdata->total_bytes > 0)
    pcent = (int) ((double) (tdata->trans_bytes + tdata->resumed_bytes) / (double) tdata->total_bytes * 100.0);
  else
    pcent = 0;

  if (pcent > 100)
    g_snprintf (totstr, sizeof (totstr),
    _("Unknown percentage complete. (File %ld of %ld)"),
    tdata->current_file_number, tdata->numdirs + tdata->numfiles);
  else
    g_snprintf (totstr, sizeof (totstr),
    _("%d%% complete, %02d:%02d:%02d est. time remaining. (File %ld of %ld)"),
    pcent, hours, mins, secs, tdata->current_file_number,
    tdata->numdirs + tdata->numfiles);

  *dlstr = '\0';
  if (!tdata->stalled)
    _setup_dlstr (tdata, tempfle, dlstr, sizeof (dlstr));

  g_static_mutex_unlock (&tdata->statmutex);

  gtk_tree_store_set(GTK_TREE_STORE(model), iter, 1, totstr, -1);

  gftp_lookup_global_option ("show_trans_in_title", &show_trans_in_title);
  if (gftp_file_transfers->data == tdata && show_trans_in_title)
    {
      g_snprintf (winstr, sizeof(winstr),  "%s: %s", gftp_version, totstr);
      gdk_window_set_title (gtk_widget_get_parent_window (GTK_WIDGET(dlwdw)),
                            winstr);
    }

  if (*dlstr != '\0')
    gtk_tree_store_set(GTK_TREE_STORE(model), iter, 1, dlstr, -1);
}


static void
update_window_transfer_bytes (gftp_window_data * wdata)
{
  char *tempstr, *temp1str;

  if (wdata->request->gotbytes == -1)
    {
      update_window (wdata);
      wdata->request->gotbytes = 0;
    }
  else
    {
      tempstr = insert_commas (wdata->request->gotbytes, NULL, 0);
      temp1str = g_strdup_printf (_("Retrieving file names...%s bytes"),
                                  tempstr);
      gtk_label_set_text (GTK_LABEL (wdata->hoststxt), temp1str);
      g_free (tempstr);
      g_free (temp1str);
    }
}

static int get_node_iter(GtkTreeModel * list_store, GtkTreeIter * iter, gftp_transfer * tdata)
{
  gftp_transfer * td;
  int valid = gtk_tree_model_get_iter_first (list_store, iter);
  while (valid && tdata != td)
  {
    gtk_tree_model_get (list_store, iter, 2, &td, -1);

    if (tdata == td)
    {
      continue;
    }
    valid = gtk_tree_model_iter_next (list_store, iter);
  }
  return tdata != td;
}

gint
update_downloads (gpointer data)
{
  intptr_t do_one_transfer_at_a_time, start_transfers;
  GList * templist, * next;
  gftp_transfer * tdata;
  GtkTreeModel * model;
  GtkTreeIter iter;

  if (window1.request->gotbytes != 0)
    update_window_transfer_bytes (&window1);
  if (window2.request->gotbytes != 0)
    update_window_transfer_bytes (&window2);

  if (gftpui_common_child_process_done)
    check_done_process ();

  model = gtk_tree_view_get_model(GTK_TREE_VIEW (dlwdw));
  for (templist = gftp_file_transfers; templist != NULL;)
    {
      tdata = templist->data;

    get_node_iter(model, &iter, tdata);
    if (tdata->ready)
    {
      g_static_mutex_lock (&tdata->structmutex);
      if (tdata->next_file)
        on_next_transfer (tdata, model, &iter);
      else if (tdata->show)
        show_transfer (tdata, model, &iter);
      else if (tdata->done)
      {
        next = templist->next;
        g_static_mutex_unlock (&tdata->structmutex);
        transfer_done (templist, model, &iter);
        templist = next;
        continue;
      }
      if (tdata->curfle != NULL)
      {
        gftp_lookup_global_option ("one_transfer",
                                   &do_one_transfer_at_a_time);
        gftp_lookup_global_option ("start_transfers", &start_transfers);

        if (!tdata->started && start_transfers &&
            (num_transfers_in_progress == 0 || !do_one_transfer_at_a_time))
          create_transfer (tdata, model, &iter);

        if (tdata->started)
          update_file_status (tdata, model, &iter);
      }
      g_static_mutex_unlock (&tdata->structmutex);
    }
      templist = templist->next;
  }

  g_timeout_add (500, update_downloads, NULL);
  return (0);
}


void
start_transfer (GtkAction * a, gpointer data)
{
  GtkTreeSelection *select;
  GtkTreeIter iter;
  GtkTreeModel * model;
  gftp_transfer * transfer;

  select = gtk_tree_view_get_selection (GTK_TREE_VIEW (dlwdw));
  if (! gtk_tree_selection_get_selected (select, &model, &iter))
    {
      ftp_log (gftp_logging_error, NULL,
           _("There are no file transfers selected\n"));
      return;
    }
  gtk_tree_model_get(model, &iter, 2, &transfer, -1);

  g_static_mutex_lock (&transfer->structmutex);
  if (!transfer->started)
    create_transfer (transfer, model, &iter);
  g_static_mutex_unlock (&transfer->structmutex);
}

void
stop_transfer (GtkAction * a, gpointer data)
{
  gftp_transfer * transfer;
  GtkTreeSelection *select;
  GtkTreeIter iter;
  GtkTreeModel * model;

  select = gtk_tree_view_get_selection (GTK_TREE_VIEW (dlwdw));
  if (! gtk_tree_selection_get_selected (select, &model, &iter))
  {
    ftp_log (gftp_logging_error, NULL,
        _("There are no file transfers selected\n"));
    return;
  }
  gtk_tree_model_get(model, &iter, 2, &transfer, -1);
  gftpui_common_cancel_file_transfer (transfer);
}

static void
gftpui_common_skip_file_transfer (gftp_transfer * tdata, gftp_file * curfle)
{
  g_static_mutex_lock (&tdata->structmutex);

  if (tdata->started && !(curfle->transfer_action & GFTP_TRANS_ACTION_SKIP))
    {
      curfle->transfer_action = GFTP_TRANS_ACTION_SKIP;
      if (tdata->curfle != NULL && curfle == tdata->curfle->data)
        {
          tdata->cancel = 1;
          tdata->fromreq->cancel = 1;
          tdata->toreq->cancel = 1;
          tdata->skip_file = 1;
        }
      else if (!curfle->transfer_done)
        tdata->total_bytes -= curfle->size;
    }

  g_static_mutex_unlock (&tdata->structmutex);

  if (curfle != NULL)
    tdata->fromreq->logging_function (gftp_logging_misc, tdata->fromreq,
                                      _("Skipping file %s on host %s\n"),
                                      curfle->file, tdata->toreq->hostname);
}

void
skip_transfer (GtkAction * a, gpointer data)
{
  gftp_transfer * transfer;
  GtkTreeSelection *select;
  GtkTreeIter iter;
  GtkTreeModel * model;

  select = gtk_tree_view_get_selection (GTK_TREE_VIEW (dlwdw));
   if (! gtk_tree_selection_get_selected (select, &model, &iter))
    {
      ftp_log (gftp_logging_error, NULL,
          _("There are no file transfers selected\n"));
      return;
    }
  gtk_tree_model_get(model, &iter, 2, &transfer, -1);
  gftpui_common_skip_file_transfer (transfer,
                                    transfer->curfle->data);
}


void
remove_file_transfer (GtkAction * a, gpointer data)
{
  gftp_transfer * transfer;
  GList * curfle;
  GtkTreeSelection *select;
  GtkTreeIter iter;
  GtkTreeModel * model;
  select = gtk_tree_view_get_selection (GTK_TREE_VIEW (dlwdw));
  if (! gtk_tree_selection_get_selected (select, &model, &iter))
  {
    ftp_log (gftp_logging_error, NULL,
      _("There are no file transfers selected\n"));
    return;
  }
  gtk_tree_model_get(model, &iter, 2, &transfer, 3, &curfle, -1);
  if (curfle == NULL || curfle->data == NULL)
    return;

  gftpui_common_skip_file_transfer (transfer, curfle->data);

  gtk_tree_store_set(GTK_TREE_STORE(model), &iter, 1, _("Skipped"), -1);
}


void
move_transfer_up (GtkAction * a, gpointer data)
{
  GList * firstentry, * secentry, * lastentry;
  gftp_transfer * transfer;
  GList * curfle;
  GtkTreeSelection *select;
  GtkTreeIter iter;
  GtkTreeIter prev;
  GtkTreeModel * model;
  GtkTreePath *path;

  select = gtk_tree_view_get_selection (GTK_TREE_VIEW (dlwdw));
  if (! gtk_tree_selection_get_selected (select, &model, &iter))
  {
      ftp_log (gftp_logging_error, NULL,
          _("There are no file transfers selected\n"));
      return;
    }
  gtk_tree_model_get(model, &iter, 2, &transfer, 3, &curfle, -1);
  if (curfle == NULL)
    return;

  g_static_mutex_lock (&transfer->structmutex);
  if (curfle->prev != NULL && (!transfer->started ||
      (transfer->curfle != curfle &&
       transfer->curfle != curfle->prev)))
    {
      if (curfle->prev->prev == NULL)
        {
          firstentry = curfle->prev;
          lastentry = curfle->next;
          transfer->files = curfle;
          curfle->next = firstentry;
          transfer->files->prev = NULL;
          firstentry->prev = curfle;
          firstentry->next = lastentry;
          if (lastentry != NULL)
            lastentry->prev = firstentry;
        }
      else
        {
          firstentry = curfle->prev->prev;
          secentry = curfle->prev;
          lastentry = curfle->next;
          firstentry->next = curfle;
          curfle->prev = firstentry;
          curfle->next = secentry;
          secentry->prev = curfle;
          secentry->next = lastentry;
          if (lastentry != NULL)
            lastentry->prev = secentry;
        }
        prev = iter;
        path = gtk_tree_model_get_path (model, &iter);
        if (gtk_tree_path_prev (path))
        {
          gtk_tree_model_get_iter (model, &prev, path);
          gtk_tree_store_swap(GTK_TREE_STORE(model), &iter, &prev);
        }
        gtk_tree_path_free (path);
    }
  g_static_mutex_unlock (&transfer->structmutex);
}


void
move_transfer_down (GtkAction * a, gpointer data)
{
  GList * firstentry, * secentry, * lastentry;
  gftp_transfer * transfer;
  GList * curfle;
  GtkTreeSelection *select;
  GtkTreeIter iter;
  GtkTreeIter next;
  GtkTreeModel * model;
  GtkTreePath *path;

  select = gtk_tree_view_get_selection (GTK_TREE_VIEW (dlwdw));
  if (! gtk_tree_selection_get_selected (select, &model, &iter))
  {
    ftp_log (gftp_logging_error, NULL,
        _("There are no file transfers selected\n"));
    return;
  }
  gtk_tree_model_get(model, &iter, 2, &transfer, 3, &curfle, -1);
  if (curfle == NULL)
    return;

  g_static_mutex_lock (&transfer->structmutex);
  if (curfle->next != NULL && (!transfer->started ||
      (transfer->curfle != curfle &&
       transfer->curfle != curfle->next)))
    {
      if (curfle->prev == NULL)
        {
          firstentry = curfle->next;
          lastentry = curfle->next->next;
          transfer->files = firstentry;
          transfer->files->prev = NULL;
          transfer->files->next = curfle;
          curfle->prev = transfer->files;
          curfle->next = lastentry;
          if (lastentry != NULL)
            lastentry->prev = curfle;
        }
      else
        {
          firstentry = curfle->prev;
          secentry = curfle->next;
          lastentry = curfle->next->next;
          firstentry->next = secentry;
          secentry->prev = firstentry;
          secentry->next = curfle;
          curfle->prev = secentry;
          curfle->next = lastentry;
          if (lastentry != NULL)
            lastentry->prev = curfle;
        }
        next = iter;
        path = gtk_tree_model_get_path (model, &iter);
        gtk_tree_path_next (path);
        gtk_tree_model_get_iter (model, &next, path);
        gtk_tree_store_swap(GTK_TREE_STORE(model), &iter, &next);
        gtk_tree_path_free (path);
    }
  g_static_mutex_unlock (&transfer->structmutex);
}
