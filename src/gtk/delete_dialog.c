/*****************************************************************************/
/*  delete_dialog.c - the delete dialog                                      */
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

static void
_gftpui_common_del_purge_cache (gpointer key, gpointer value,
                                gpointer user_data)
{
  gftp_delete_cache_entry (NULL, key, 0);
  g_free (key);
}

static int
gftpui_common_run_delete (gftpui_callback_data * cdata)
{
  char *tempstr, description[BUFSIZ];
  gftp_file * tempfle;
  GHashTable * rmhash;
  GList * templist;
  int success, ret;

  for (templist = cdata->files;
       templist->next != NULL;
       templist = templist->next);

  if (cdata->request->use_cache)
    rmhash = g_hash_table_new (string_hash_function, string_hash_compare);
  else
    rmhash = NULL;

  ret = 0;
  for (; templist != NULL; templist = templist->prev)
    {
      tempfle = templist->data;

      if (S_ISDIR (tempfle->st_mode))
        success = gftp_remove_directory (cdata->request, tempfle->file);
      else
        success = gftp_remove_file (cdata->request, tempfle->file);

      if (success < 0)
        ret = success;
      else if (rmhash != NULL)
        {
          gftp_generate_cache_description (cdata->request, description,
                                           sizeof (description), 0);
          if (g_hash_table_lookup (rmhash, description) == NULL)
            {
              tempstr = g_strdup (description);
              g_hash_table_insert (rmhash, tempstr, NULL);
            }
        }

      if (!GFTP_IS_CONNECTED (cdata->request))
        break;
    }

  if (rmhash != NULL)
    {
      g_hash_table_foreach (rmhash, _gftpui_common_del_purge_cache, NULL);
      g_hash_table_destroy (rmhash);
    }

  return (ret);
}

static void
askdel (gftp_transfer * transfer)
{
  char *tempstr;
  int ok;
  if (transfer->numfiles > 0 && transfer->numdirs > 0)
    {
      tempstr = g_strdup_printf (_("Are you sure you want to delete these %ld files and %ld directories"), transfer->numfiles, transfer->numdirs);
    }
  else if (transfer->numfiles > 0)
    {
      tempstr = g_strdup_printf (_("Are you sure you want to delete these %ld files"), transfer->numfiles);
    }
  else if (transfer->numdirs > 0)
    {
      tempstr = g_strdup_printf (_("Are you sure you want to delete these %ld directories"), transfer->numdirs);
    }
  else
    return;
  ok = MakeYesNoDialog (_("Delete Files/Directories"), tempstr);
  g_free (tempstr);
  if (ok)
    {
      gftpui_callback_data * cdata;

      g_return_if_fail (transfer != NULL);
      g_return_if_fail (transfer->files != NULL);

      cdata = g_malloc0 (sizeof (*cdata));
      cdata->request = ((gftp_window_data *)transfer->fromwdata)->request;
      cdata->files = transfer->files;
      cdata->uidata = transfer->fromwdata;
      cdata->run_function = gftpui_common_run_delete;

      gftpui_common_run_callback_function (cdata);

      g_free (cdata);
    }
  free_tdata (transfer);
}

void
delete_dialog (GtkAction * a, gpointer data)
{
  gftp_file * tempfle, * newfle;
  GList * templist, * filelist;
  gftp_transfer * transfer;
  gftp_window_data * wdata;
  int ret;
  GtkTreeSelection *select;
  GtkTreeIter iter;
  GtkTreeModel * model;

  wdata = data;
  if (!check_status (_("Delete"), wdata,
      gftpui_common_use_threads (wdata->request), 0, 1, 1))
    return;

  transfer = g_malloc0 (sizeof (*transfer));
  transfer->fromreq = gftp_copy_request (wdata->request);
  transfer->fromwdata = wdata;

  select = gtk_tree_view_get_selection (GTK_TREE_VIEW (wdata->listbox));
  templist = gtk_tree_selection_get_selected_rows(select, &model);
  for (filelist = templist ; filelist != NULL; filelist = g_list_next(filelist))
  {
    gtk_tree_model_get_iter(model, &iter, (GtkTreePath*)filelist->data);
    gtk_tree_model_get(model, &iter, 0, &tempfle, -1);

      if (strcmp (tempfle->file, "..") == 0 ||
          strcmp (tempfle->file, ".") == 0)
        continue;
      newfle = copy_fdata (tempfle);
      transfer->files = g_list_append (transfer->files, newfle);
  }
  g_list_foreach (templist, (GFunc) gtk_tree_path_free, NULL);
  g_list_free (templist);

  if (transfer->files == NULL)
    {
      free_tdata (transfer);
      return;
    }

  gftp_swap_socks (transfer->fromreq, wdata->request);

  ret = gftp_gtk_get_subdirs (transfer);

  gftp_swap_socks (wdata->request, transfer->fromreq);

  if (!GFTP_IS_CONNECTED (wdata->request))
    {
      gftpui_disconnect (wdata);
      return;
    }

  if (!ret)
    return;

  askdel (transfer);
}
