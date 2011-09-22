/*****************************************************************************/
/*  bookmarks.c - routines for the bookmarks                                 */
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

static GtkWidget * bm_hostedit, * bm_portedit, * bm_localdiredit,
                 * bm_remotediredit, * bm_useredit, * bm_passedit, * tree,
                 * bm_acctedit, * anon_chk, * bm_pathedit, * bm_protocol;
static gftp_bookmarks_var * curentry;
static guint book_mid;
static GtkActionGroup * book_group;

void
run_bookmark (GtkAction * a, gpointer data)
{
  int refresh_local;
  gftp_bookmarks_var * b;

  b = (gftp_bookmarks_var *) data;
  if (window1.request->stopable || window2.request->stopable)
    {
      ftp_log (gftp_logging_misc, NULL,
               _("%s: Please hit the stop button first to do anything else\n"),
               _("Run Bookmark"));
      return;
    }

  if (GFTP_IS_CONNECTED (current_wdata->request))
    gftpui_disconnect (current_wdata);

  if (gftp_parse_bookmark (current_wdata->request, other_wdata->request,
                           b->path, &refresh_local) < 0)
    return;

  if (refresh_local)
    gftpui_refresh (other_wdata, 0);

  ftp_connect (current_wdata, current_wdata->request);
}

static void
append_bookmark (gftp_bookmarks_var * b, const char * up, GtkActionGroup * g)
{
  char action_name[128];
  char parent_path[256];
  GtkAction * action;
  char * pos;

  if ((pos = strrchr (b->path, '/')) == NULL)
    pos = b->path;
  else
    pos++;

    g_snprintf (action_name, sizeof (action_name), "book%s", pos);
    action = gtk_action_new (action_name, pos, NULL, NULL);
    gtk_action_group_add_action (g, GTK_ACTION (action));

    if (strlen(up))
    {
        g_snprintf (parent_path, sizeof (parent_path), "/MainMenu/Bookmarks/Bookmarks Placeholder/book%s", up);
    }
    else
    {
        g_snprintf (parent_path, sizeof (parent_path), "/MainMenu/Bookmarks/Bookmarks Placeholder");
    }
    if (! b->isfolder)
    {
      g_signal_connect (G_OBJECT (action), "activate", G_CALLBACK (run_bookmark), b);
      gtk_ui_manager_add_ui (ui_manager, book_mid, parent_path,
        action_name, action_name, GTK_UI_MANAGER_MENUITEM, FALSE);
    }
    else
    {
      gtk_ui_manager_add_ui (ui_manager, book_mid, parent_path,
          action_name, action_name, GTK_UI_MANAGER_MENU, FALSE);
    }
    g_object_unref (action);
}

static void
doadd_bookmark (gpointer * data, gftp_dialog_data * ddata)
{
  const char *edttxt, *spos;
  gftp_bookmarks_var * tempentry;
  char *dpos, *proto;

  edttxt = gtk_entry_get_text (GTK_ENTRY (ddata->edit));
  if (*edttxt == '\0')
    {
      ftp_log (gftp_logging_error, NULL,
           _("Add Bookmark: You must enter a name for the bookmark\n"));
      return;
    }

  if (g_hash_table_lookup (gftp_bookmarks_htable, edttxt) != NULL)
    {
      ftp_log (gftp_logging_error, NULL,
           _("Add Bookmark: Cannot add bookmark %s because that name already exists\n"), edttxt);
      return;
    }

  tempentry = g_malloc0 (sizeof (*tempentry));

  dpos = tempentry->path = g_malloc0 ((gulong) strlen (edttxt) + 1);
  for (spos = edttxt; *spos != '\0';)
    {
      *dpos++ = *spos++;
      if (*spos == '/')
        {
          *dpos++ = '/';
          for (; *spos == '/'; spos++);
        }
    }
  *dpos = '\0';

  edttxt = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT (hostedit));
  tempentry->hostname = g_strdup (edttxt);

  edttxt = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT (portedit));
  tempentry->port = strtol (edttxt, NULL, 10);

  proto = gftp_protocols[current_wdata->request->protonum].name;
  tempentry->protocol = g_strdup (proto);

  edttxt = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT (other_wdata->combo));
  tempentry->local_dir = g_strdup (edttxt);

  edttxt = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT (current_wdata->combo));
  tempentry->remote_dir = g_strdup (edttxt);

  if ((edttxt = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT (useredit))) != NULL)
    {
      tempentry->user = g_strdup (edttxt);

      edttxt = gtk_entry_get_text (GTK_ENTRY (passedit));
      tempentry->pass = g_strdup (edttxt);
      tempentry->save_password =  gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON (ddata->checkbox));
    }

  gftp_add_bookmark (tempentry);

  append_bookmark(tempentry, "", book_group);

  gftp_write_bookmarks_file ();
}

void
add_bookmark (GtkAction * a, gpointer data)
{
  const char *edttxt;

  if (!check_status (_("Add Bookmark"), current_wdata, 0, 0, 0, 1))
    return;

  edttxt = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT (hostedit));
  if (*edttxt == '\0')
    {
      ftp_log (gftp_logging_error, NULL,
           _("Add Bookmark: You must enter a hostname\n"));
      return;
    }

  MakeEditDialog (_("Add Bookmark"),
    _("Enter the name of the bookmark you want to add\nYou can separate items by a / to put it into a submenu\n(ex: Linux Sites/Debian)"),
    NULL, 1, _("Remember password"), GTK_STOCK_ADD, doadd_bookmark, data, NULL, NULL);
}


void
build_bookmarks_menu (void)
{
  gftp_bookmarks_var * tempentry = NULL;
  gftp_bookmarks_var * preventry = NULL;

  book_mid = gtk_ui_manager_new_merge_id (ui_manager);
  book_group = gtk_action_group_new("book");
  gtk_ui_manager_insert_action_group (ui_manager, book_group, 0);
  tempentry = gftp_bookmarks->children;
  while (tempentry != NULL)
  {
    append_bookmark(tempentry, "", book_group);
    if (tempentry->children != NULL)
    {
      preventry = tempentry->children;
      while (preventry != NULL)
      {
        append_bookmark(preventry, tempentry->path, book_group);
        preventry = preventry->next;
      }
    }
    tempentry = tempentry->next;
 }
}

static gftp_bookmarks_var *
copy_bookmarks (gftp_bookmarks_var * tempentry)
{
  gftp_bookmarks_var * newentry;
  newentry = g_malloc0 (sizeof (*newentry));
  newentry->isfolder = tempentry->isfolder;
  newentry->save_password = tempentry->save_password;
  if (tempentry->path)
    newentry->path = g_strdup (tempentry->path);

  if (tempentry->hostname)
    newentry->hostname = g_strdup (tempentry->hostname);

  if (tempentry->protocol)
    newentry->protocol = g_strdup (tempentry->protocol);

  if (tempentry->local_dir)
    newentry->local_dir = g_strdup (tempentry->local_dir);

  if (tempentry->remote_dir)
    newentry->remote_dir = g_strdup (tempentry->remote_dir);

  if (tempentry->user)
    newentry->user = g_strdup (tempentry->user);

  if (tempentry->pass)
    newentry->pass = g_strdup (tempentry->pass);

  if (tempentry->acct)
    newentry->acct = g_strdup (tempentry->acct);

  newentry->port = tempentry->port;

  gftp_copy_local_options (&newentry->local_options_vars,
                           &newentry->local_options_hash,
                           &newentry->num_local_options_vars,
                           tempentry->local_options_vars,
                           tempentry->num_local_options_vars);
  newentry->num_local_options_vars = tempentry->num_local_options_vars;

  return newentry;
}

static void
bm_apply_changes (GtkTreeModel * model)
{
  GtkTreeIter up;
  GtkTreeIter iter;
  gftp_bookmarks_var * prev0;
  gftp_bookmarks_var * prev1;
  gftp_bookmarks_var * child0;
  gftp_bookmarks_var * child1;
  gftp_bookmarks_var * entry0;
  gftp_bookmarks_var * entry1;
  int valid;

  if (gftp_bookmarks != NULL)
    {
      g_object_unref(book_group);
      gtk_ui_manager_remove_ui (ui_manager, book_mid);
      g_hash_table_destroy (gftp_bookmarks_htable);
      gftp_bookmarks_destroy (gftp_bookmarks);
    }

  gftp_bookmarks = g_malloc0 (sizeof (*gftp_bookmarks));
  gftp_bookmarks->path = g_malloc0 (1);
  gftp_bookmarks->isfolder = 1;
  valid = gtk_tree_model_get_iter_first (model, &up);
  child0 = NULL;
  prev0 = NULL;
  while (valid)
  {
    gtk_tree_model_get (model, &up, 0, &entry0, -1);
    if (child0 == NULL)
    {
      child0 = entry0;
    }
    if (prev0 != NULL)
    {
       prev0->next = entry0;
    }
    entry0->prev = gftp_bookmarks;
    valid = gtk_tree_model_iter_children (model, &iter, &up);
    child1 = NULL;
    prev1 = NULL;
    while (valid)
    {
      gtk_tree_model_get (model, &iter, 0, &entry1, -1);
      if (child1 == NULL)
      {
        child1 = entry1;
      }
      if (prev1 != NULL)
      {
        prev1->next = entry1;
      }
      entry1->prev = entry0;
      entry1->children = NULL;
      valid = gtk_tree_model_iter_next (model, &iter);
      prev1 = entry1;
    }
    if (prev1 != NULL)
    {
        prev1->next = NULL;
    }
    entry0->children = child1;
    if (child1 != NULL)
    {
        entry0->isfolder = 1;
    }
    valid = gtk_tree_model_iter_next (model, &up);
    prev0 = entry0;
  }
  if (prev0 != NULL)
  {
      prev0->next = NULL;
  }
  gftp_bookmarks->children = child0;
  gftp_bookmarks_htable = build_bookmarks_hash_table (gftp_bookmarks);

  build_bookmarks_menu ();
  gftp_write_bookmarks_file ();
}

static void
bm_clean_changes (GtkTreeModel * model)
{
  GtkTreeIter up;
  GtkTreeIter iter;
  gftp_bookmarks_var * entry0;
  gftp_bookmarks_var * entry1;
  int valid;

  model = gtk_tree_view_get_model (GTK_TREE_VIEW (tree));
  valid = gtk_tree_model_get_iter_first (model, &up);
  while (valid)
  {
    gtk_tree_model_get (model, &up, 0, &entry0, -1);
    valid = gtk_tree_model_iter_children (model, &iter, &up);
    while (valid)
    {
      gtk_tree_model_get (model, &iter, 0, &entry1, -1);
      gftp_free_bookmark (entry1, 1);
      valid = gtk_tree_model_iter_next (model, &iter);
    }
  }
}


static void data_col_0 (GtkTreeViewColumn *tree_column, GtkCellRenderer *cell,
    GtkTreeModel * model, GtkTreeIter *iter, gpointer data)
{
  gftp_bookmarks_var * entry;
  char * pos;

  gtk_tree_model_get(model, iter, 0, &entry, -1);
      if ((pos = strrchr (entry->path, '/')) == NULL)
        pos = entry->path;
      else
        pos++;

  g_object_set(cell, "text", pos, NULL);
}


static void
do_make_new (gpointer data, gftp_dialog_data * ddata)
{
  gftp_bookmarks_var * newentry = NULL;
  const char *str;
  GtkTreeModel * model;
  GtkTreeIter iter;
  GtkTreeIter parent;

  str = gtk_entry_get_text (GTK_ENTRY (ddata->edit));
  if (*str == '\0')
    {
      ftp_log (gftp_logging_misc, NULL, _("You must specify a name for the bookmark."));
      return;
    }

  newentry = g_malloc0 (sizeof (*newentry));
  newentry->path = g_strdup (str);
  if (data)
    newentry->isfolder = 1;

  model = gtk_tree_view_get_model (GTK_TREE_VIEW (tree));
  gtk_tree_model_get_iter_first(model, &parent);
  gtk_tree_store_append (GTK_TREE_STORE(model), &iter, &parent);
  gtk_tree_store_set(GTK_TREE_STORE(model), &iter, 0, newentry, -1);
}


static void
new_folder_entry (gpointer data)
{
  MakeEditDialog (_("New Folder"),
          _("Enter the name of the new folder to create"), NULL, 1,
          NULL, GTK_STOCK_ADD, do_make_new, (gpointer) 0x1, NULL, NULL);
}


static void
new_item_entry (gpointer data)
{
  MakeEditDialog (_("New Item"),
          _("Enter the name of the new item to create"), NULL, 1,
          NULL, GTK_STOCK_ADD, do_make_new, NULL, NULL, NULL);
}


static void
do_delete_entry (void * e, void * f)
{
  int valid;
  gftp_bookmarks_var * entry;
  gftp_bookmarks_var * entry0;
  GtkTreeSelection *select;
  GtkTreeIter iter;
  GtkTreeIter iter0;
  GtkTreeModel * model;

  select = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree));
  if (! gtk_tree_selection_get_selected (select, &model, &iter))
    return;
  gtk_tree_model_get(model, &iter, 0, &entry, -1);
  valid = gtk_tree_model_iter_children (model, &iter0, &iter);
  while (valid)
    {
      gtk_tree_model_get(model, &iter0, 0, &entry0, -1);
      gtk_tree_store_remove (GTK_TREE_STORE(model), &iter0);
      gftp_free_bookmark (entry0, 1);
      valid = gtk_tree_model_iter_next (model, &iter0);
    }
  gtk_tree_store_remove (GTK_TREE_STORE(model), &iter);
  gftp_free_bookmark (entry, 1);
}


static void
delete_entry (gpointer data)
{
  gftp_bookmarks_var * entry;
  char *tempstr;
  GtkTreeSelection *select;
  GtkTreeIter iter;
  GtkTreeModel * model;

  select = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree));
  if (! gtk_tree_selection_get_selected (select, &model, &iter))
    return;
  gtk_tree_model_get(model, &iter, 0, &entry, -1);

  if (entry == NULL || entry->prev == NULL)
    return;

  if (!entry->isfolder)
    do_delete_entry (entry, NULL);
  else
    {
      tempstr = g_strdup_printf (_("Are you sure you want to erase the bookmark\n%s and all its children?"), entry->path);
      MakeYesNoDialog (_("Delete Bookmark"), tempstr, do_delete_entry, entry,
                       NULL, NULL);
      g_free (tempstr);
    }
}


static void
set_userpass_visible (GtkWidget * checkbutton, GtkWidget * entry)
{
  gtk_widget_set_sensitive (bm_useredit, !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON (anon_chk)));
  gtk_widget_set_sensitive (bm_passedit, !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON (anon_chk)));
  gtk_widget_set_sensitive (bm_acctedit, !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON (anon_chk)));
}

static void
build_bookmarks_tree (gftp_bookmarks_var * up)
{
  GtkTreeModel * model;
  GtkTreeIter iter;
  GtkTreeIter fils;
  gftp_bookmarks_var * tempentry = NULL;
  gftp_bookmarks_var * preventry = NULL;
  gftp_bookmarks_var * new;

  model = gtk_tree_view_get_model (GTK_TREE_VIEW (tree));
  tempentry = up->children;
  while (tempentry != NULL)
  {
    new = copy_bookmarks(tempentry);
    gtk_tree_store_append (GTK_TREE_STORE(model), &iter, NULL);
    gtk_tree_store_set(GTK_TREE_STORE(model), &iter, 0, new, -1);
    if (tempentry->children != NULL)
    {
      preventry = tempentry->children;
      while (preventry != NULL)
      {
        new = copy_bookmarks(preventry);
        gtk_tree_store_append (GTK_TREE_STORE(model), &fils, &iter);
        gtk_tree_store_set(GTK_TREE_STORE(model), &fils, 0, new, -1);
        preventry = preventry->next;
      }
    }
    tempentry = tempentry->next;
 }
}


static void
entry_apply_changes (gftp_bookmarks_var * entry)
{
  char *pos, tempchar, *tempstr;
  const char *str;

  tempstr = g_strdup (gtk_entry_get_text (GTK_ENTRY (bm_pathedit)));
  while ((pos = strchr (tempstr, '/')) != NULL)
    *pos = ' ';

  if ((pos = strrchr (entry->path, '/')) != NULL)
    {
      tempchar = *pos;
      *pos = '\0';
      entry->path = gftp_build_path (NULL, entry->path, tempstr, NULL);
      *pos = tempchar;
      g_free (tempstr);
    }
  else
    entry->path = tempstr;

  str = gtk_entry_get_text (GTK_ENTRY (bm_hostedit));
  if (entry->hostname != NULL)
    g_free (entry->hostname);
  entry->hostname = g_strdup (str);

  str = gtk_entry_get_text (GTK_ENTRY (bm_portedit));
  entry->port = strtol (str, NULL, 10);

  str = gtk_combo_box_text_get_active_text (GTK_COMBO_BOX_TEXT (bm_protocol));
  if (entry->protocol != NULL)
    g_free (entry->protocol);
  entry->protocol = g_strdup (str);

  str = gtk_entry_get_text (GTK_ENTRY (bm_remotediredit));
  if (entry->remote_dir != NULL)
    g_free (entry->remote_dir);
  entry->remote_dir = g_strdup (str);

  str = gtk_entry_get_text (GTK_ENTRY (bm_localdiredit));
  if (entry->local_dir != NULL)
    g_free (entry->local_dir);
  entry->local_dir = g_strdup (str);

  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON (anon_chk)))
    str = GFTP_ANONYMOUS_USER;
  else
    str = gtk_entry_get_text (GTK_ENTRY (bm_useredit));

  if (entry->user != NULL)
    g_free (entry->user);
  entry->user = g_strdup (str);

  if (strcasecmp (entry->user, GFTP_ANONYMOUS_USER) == 0)
    str = "@EMAIL@";
  else
    str = gtk_entry_get_text (GTK_ENTRY (bm_passedit));
  if (entry->pass != NULL)
    g_free (entry->pass);
  entry->pass = g_strdup (str);
  entry->save_password = *entry->pass != '\0';

  str = gtk_entry_get_text (GTK_ENTRY (bm_acctedit));
  if (entry->acct != NULL)
    g_free (entry->acct);
  entry->acct = g_strdup (str);

  gftp_gtk_save_bookmark_options ();
}

static void sel_change (GtkTreeSelection * selection, gpointer user_data)
{
  gftp_bookmarks_var * entry;
  unsigned int num, i;
  char *pos;

  GtkTreeIter iter;
  GtkTreeModel * model;

   if (curentry != NULL)
   {
      entry_apply_changes (curentry);
    }

  if (! gtk_tree_selection_get_selected (selection, &model, &iter))
    return;

  gtk_tree_model_get(model, &iter, 0, &entry, -1);

  if (entry == NULL)
    return;

  curentry = entry;
  if ((pos = strrchr (entry->path, '/')) == NULL)
    pos = entry->path;
  else
    pos++;
  if (pos)
    gtk_entry_set_text (GTK_ENTRY (bm_pathedit), pos);
  else
    gtk_entry_set_text (GTK_ENTRY (bm_pathedit), "");

  gtk_widget_set_sensitive (bm_hostedit, ! entry->isfolder);
  if (entry->hostname)
    gtk_entry_set_text (GTK_ENTRY (bm_hostedit), entry->hostname);
  else
    gtk_entry_set_text (GTK_ENTRY (bm_hostedit), "");

  gtk_widget_set_sensitive (bm_portedit, ! entry->isfolder);
  if (entry->port)
    {
      pos = g_strdup_printf ("%d", entry->port);
      gtk_entry_set_text (GTK_ENTRY (bm_portedit), pos);
      g_free (pos);
    }
  else
  {
    gtk_entry_set_text (GTK_ENTRY (bm_portedit), "");
  }

  gtk_widget_set_sensitive (bm_protocol, ! entry->isfolder);
  num = 0;
  for (i = 0; gftp_protocols[i].name; i++)
    {
      if (entry->protocol &&
          strcmp (gftp_protocols[i].name, entry->protocol) == 0)
    num = i;
    }
  gtk_combo_box_set_active (GTK_COMBO_BOX (bm_protocol), num);

  gtk_widget_set_sensitive (bm_remotediredit, ! entry->isfolder);
  if (entry->remote_dir)
    gtk_entry_set_text (GTK_ENTRY (bm_remotediredit), entry->remote_dir);
  else
  {
    gtk_entry_set_text (GTK_ENTRY (bm_remotediredit), "");
  }
  gtk_widget_set_sensitive (bm_localdiredit, ! entry->isfolder);
  if (entry->local_dir)
    gtk_entry_set_text (GTK_ENTRY (bm_localdiredit), entry->local_dir);
  else
  {
    gtk_entry_set_text (GTK_ENTRY (bm_localdiredit), "");
  }
  gtk_widget_set_sensitive (bm_useredit, ! entry->isfolder);
  if (entry->user)
    gtk_entry_set_text (GTK_ENTRY (bm_useredit), entry->user);
  else
  {
    gtk_entry_set_text (GTK_ENTRY (bm_useredit), "");
  }
  gtk_widget_set_sensitive (bm_passedit, ! entry->isfolder);
  if (entry->pass)
    gtk_entry_set_text (GTK_ENTRY (bm_passedit), entry->pass);
  else
  {
    gtk_entry_set_text (GTK_ENTRY (bm_passedit), "");
  }
  gtk_widget_set_sensitive (bm_acctedit, ! entry->isfolder);
  if (entry->acct)
    gtk_entry_set_text (GTK_ENTRY (bm_acctedit), entry->acct);
  else
  {
    gtk_entry_set_text (GTK_ENTRY (bm_acctedit), "");
  }
  gtk_widget_set_sensitive (anon_chk, ! entry->isfolder);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (anon_chk), entry->user
                && strcmp (entry->user, "anonymous") == 0);

  gftp_gtk_set_bookmark_options (entry);
}


static gint
bm_enter (GtkWidget * widget, GdkEventKey * event, gpointer data)
{
  if (event->type == GDK_KEY_PRESS)
  {
    if (event->keyval == GDK_KEY_KP_Delete || event->keyval == GDK_KEY_Delete)
    {
      delete_entry (NULL);
      return TRUE;
    }
  }
  return FALSE;
}

void
edit_bookmarks (GtkAction * a, gpointer data)
{
  GtkWidget * edit_bookmarks_dialog;
  GtkWidget * scroll;
  GtkWidget * table, * tempwid, * notebook;
  unsigned int i;

  curentry = NULL;
  edit_bookmarks_dialog = gtk_dialog_new_with_buttons (
    _("Edit Bookmarks"), GTK_WINDOW(window), 0,
    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
    GTK_STOCK_SAVE, GTK_RESPONSE_OK,
    NULL);
  GtkTreeStore * l = gtk_tree_store_new (1, G_TYPE_POINTER);
  tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(l));
  gtk_tree_view_set_headers_visible(GTK_TREE_VIEW (tree), FALSE);
  GtkCellRenderer * cell = gtk_cell_renderer_text_new();
  gtk_tree_view_insert_column_with_data_func(GTK_TREE_VIEW (tree), -1, NULL, cell, data_col_0, NULL, NULL);

  GtkTreeSelection * select = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree));
  gtk_tree_selection_set_mode (select, GTK_SELECTION_BROWSE);
  gtk_tree_view_set_reorderable (GTK_TREE_VIEW (tree), 1);

  scroll = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
                  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

  gtk_container_set_border_width (GTK_CONTAINER (scroll), 2);
  gtk_widget_show (scroll);

  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scroll), tree);

  notebook = gtk_notebook_new ();
  gtk_widget_show (notebook);

  table = gtk_table_new (11, 2, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 5);
  gtk_table_set_row_spacings (GTK_TABLE (table), 5);
  gtk_table_set_col_spacings (GTK_TABLE (table), 5);
  gtk_widget_show (table);

  tempwid = gtk_label_new (_("Bookmark"));
  gtk_widget_show (tempwid);

  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), table, tempwid);

  tempwid = gtk_label_new (_("Description:"));
  gtk_misc_set_alignment (GTK_MISC (tempwid), 1, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), tempwid, 0, 1, 0, 1);
  gtk_widget_show (tempwid);

  bm_pathedit = gtk_entry_new ();
  gtk_table_attach_defaults (GTK_TABLE (table), bm_pathedit, 1, 2, 0, 1);
  gtk_widget_show (bm_pathedit);

  tempwid = gtk_label_new (_("Hostname:"));
  gtk_misc_set_alignment (GTK_MISC (tempwid), 1, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), tempwid, 0, 1, 1, 2);
  gtk_widget_show (tempwid);

  bm_hostedit = gtk_entry_new ();
  gtk_table_attach_defaults (GTK_TABLE (table), bm_hostedit, 1, 2, 1, 2);
  gtk_widget_show (bm_hostedit);

  tempwid = gtk_label_new (_("Port:"));
  gtk_misc_set_alignment (GTK_MISC (tempwid), 1, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), tempwid, 0, 1, 2, 3);
  gtk_widget_show (tempwid);

  bm_portedit = gtk_entry_new ();
  gtk_table_attach_defaults (GTK_TABLE (table), bm_portedit, 1, 2, 2, 3);
  gtk_widget_show (bm_portedit);

  tempwid = gtk_label_new (_("Protocol:"));
  gtk_misc_set_alignment (GTK_MISC (tempwid), 1, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), tempwid, 0, 1, 3, 4);
  gtk_widget_show (tempwid);

  bm_protocol = gtk_combo_box_text_new ();
  gtk_table_attach_defaults (GTK_TABLE (table), bm_protocol, 1, 2, 3, 4);
  gtk_widget_show (bm_protocol);
  for (i = 0; gftp_protocols[i].name; i++)
    {
      gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (bm_protocol), gftp_protocols[i].name);
    }

  tempwid = gtk_label_new (_("Remote Directory:"));
  gtk_misc_set_alignment (GTK_MISC (tempwid), 1, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), tempwid, 0, 1, 4, 5);
  gtk_widget_show (tempwid);

  bm_remotediredit = gtk_entry_new ();
  gtk_table_attach_defaults (GTK_TABLE (table), bm_remotediredit, 1, 2, 4, 5);
  gtk_widget_show (bm_remotediredit);

  tempwid = gtk_label_new (_("Local Directory:"));
  gtk_misc_set_alignment (GTK_MISC (tempwid), 1, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), tempwid, 0, 1, 5, 6);
  gtk_widget_show (tempwid);

  bm_localdiredit = gtk_entry_new ();
  gtk_table_attach_defaults (GTK_TABLE (table), bm_localdiredit, 1, 2, 5, 6);
  gtk_widget_show (bm_localdiredit);

  tempwid = gtk_hseparator_new ();
  gtk_table_attach_defaults (GTK_TABLE (table), tempwid, 0, 2, 7, 8);
  gtk_widget_show (tempwid);

  tempwid = gtk_label_new (_("Username:"));
  gtk_misc_set_alignment (GTK_MISC (tempwid), 1, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), tempwid, 0, 1, 8, 9);
  gtk_widget_show (tempwid);

  bm_useredit = gtk_entry_new ();
  gtk_table_attach_defaults (GTK_TABLE (table), bm_useredit, 1, 2, 8, 9);
  gtk_widget_show (bm_useredit);

  tempwid = gtk_label_new (_("Password:"));
  gtk_misc_set_alignment (GTK_MISC (tempwid), 1, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), tempwid, 0, 1, 9, 10);
  gtk_widget_show (tempwid);

  bm_passedit = gtk_entry_new ();
  gtk_table_attach_defaults (GTK_TABLE (table), bm_passedit, 1, 2, 9, 10);
  gtk_entry_set_visibility (GTK_ENTRY (bm_passedit), FALSE);
  gtk_widget_show (bm_passedit);

  tempwid = gtk_label_new (_("Account:"));
  gtk_misc_set_alignment (GTK_MISC (tempwid), 1, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), tempwid, 0, 1, 10, 11);
  gtk_widget_show (tempwid);

  bm_acctedit = gtk_entry_new ();
  gtk_table_attach_defaults (GTK_TABLE (table), bm_acctedit, 1, 2, 10, 11);
  gtk_entry_set_visibility (GTK_ENTRY (bm_acctedit), FALSE);
  gtk_widget_show (bm_acctedit);

  anon_chk = gtk_check_button_new_with_label (_("Log in as ANONYMOUS"));
  gtk_table_attach_defaults (GTK_TABLE (table), anon_chk, 0, 2, 11, 12);

  g_signal_connect (G_OBJECT (anon_chk), "toggled",
    G_CALLBACK (set_userpass_visible), NULL);
  gtk_widget_show (anon_chk);

  gftp_gtk_setup_bookmark_options (notebook);

  GtkWidget *hpaned = gtk_hpaned_new ();
  gtk_paned_pack1 (GTK_PANED (hpaned), scroll, TRUE, FALSE);
  gtk_paned_pack2 (GTK_PANED (hpaned), notebook, FALSE, FALSE);


  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area(GTK_DIALOG (edit_bookmarks_dialog))),
                      hpaned, TRUE, TRUE, 0);
  gtk_widget_show (hpaned);

  g_signal_connect (G_OBJECT (tree), "key-press-event", G_CALLBACK (bm_enter), NULL);
  g_signal_connect (G_OBJECT (select), "changed", G_CALLBACK (sel_change), NULL);

  build_bookmarks_tree (gftp_bookmarks);
  gtk_widget_show (tree);

  gtk_dialog_set_default_response (GTK_DIALOG(edit_bookmarks_dialog), GTK_RESPONSE_OK);
  gint response = gtk_dialog_run (GTK_DIALOG(edit_bookmarks_dialog));
  if (response == GTK_RESPONSE_OK)
  {
    bm_apply_changes (GTK_TREE_MODEL(l));
  }
  else
  {
    bm_clean_changes (GTK_TREE_MODEL(l));
  }
  gtk_widget_destroy (edit_bookmarks_dialog);
}
