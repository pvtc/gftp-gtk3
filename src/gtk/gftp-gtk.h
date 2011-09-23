/*****************************************************************************/
/*  gftp-gtk.h - include file for the gftp gtk+ 1.2 port                     */
/*  Copyright (C) 1998-2003 Brian Masney <masneyb@gftp.org>                  */
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

#ifndef __GFTP_GTK_H
#define __GFTP_GTK_H

#include "../../lib/gftp.h"
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#define GFTP_MENU_ITEM_ASCII    1
#define GFTP_MENU_ITEM_BINARY   2
#define GFTP_MENU_ITEM_WIN1 3
#define GFTP_MENU_ITEM_WIN2 4

typedef struct gftp_window_data_tag
{
  GtkWidget *combo,         /* Entry widget/history for the user to enter
                   a directory */
            *hoststxt,      /* Show which directory we're in */
            *listbox;       /* Our listbox showing the files */
  unsigned int sorted : 1,  /* Is the output sorted? */
               show_selected : 1, /* Show only selected files */
               *histlen;    /* Pointer to length of history */
  char *filespec;       /* Filespec for the listbox */
  gftp_request * request;   /* The host that we are connected to */
  GList * files,        /* Files in the listbox */
        ** history;     /* History of the directories */
  char *prefix_col_str;
} gftp_window_data;

typedef struct gftp_graphic_tag
{
  char * filename;
  GdkPixbuf * pb;
} gftp_graphic;

typedef struct gftp_viewedit_data_tag
{
   char *filename,              /* File we are viewing/editing currently */
        *remote_filename;       /* The filename on the remote computer */
   struct stat st;              /* Vital file statistics */
   pid_t pid;                   /* Our process id */
   char **argv;                 /* Our arguments we passed to execvp. We will
                                   free it when the process terminates. This
                                   is the safest place to free this */
   unsigned int view : 1,       /* View or edit this file */
                rm : 1,         /* Delete this file after we're done with it */
                dontupload : 1; /* Don't upload this file after we're done
                   editing it */
   gftp_window_data * fromwdata, /* The window we are viewing this file in */
                    * towdata;
   gftp_request * torequest;
} gftp_viewedit_data;


typedef struct gftp_textcomboedt_widget_data_tag
{
  GtkWidget * combo,
            * text;
  gftp_config_vars * cv;
  char * custom_edit_value;
} gftp_textcomboedt_widget_data;


typedef struct gftp_options_dialog_data_tag
{
  GtkWidget * dialog,
            * notebook,
            * box,
            * table;
  unsigned int tbl_col_num,
               tbl_row_num;
  gftp_option_type_enum last_option;
  gftp_bookmarks_var * bm;
} gftp_options_dialog_data;


extern gftp_window_data window1, window2, * other_wdata, * current_wdata;
extern GtkWidget * stop_btn, * hostedit, * useredit, * passedit,
                 * portedit, * logwdw, * dlwdw, * optionmenu, * openurl_btn, *window;
extern GtkAdjustment * logwdw_vadj;

extern GtkTextMark * logwdw_textmark;

extern int local_start, remote_start, trans_start;
extern GHashTable * graphic_hash_table;
extern GtkUIManager * ui_manager;
extern GList * viewedit_processes;


/* bookmarks.c */
void run_bookmark               (GtkAction * a, gpointer data );

void add_bookmark               (GtkAction * a, gpointer data );

void edit_bookmarks                 (GtkAction * a, gpointer data );

void build_bookmarks_menu           ( void );

/* chmod_dialog.c */
void chmod_dialog               (GtkAction * a,  gpointer data );

/* delete_dialog.c */
void delete_dialog              (GtkAction * a,  gpointer data );

/* dnd.c */
void openurl_get_drag_data          ( GtkWidget * widget,
                          GdkDragContext * context,
                          gint x,
                          gint y,
                          GtkSelectionData * selection_data,
                          guint info,
                          guint32 clk_time,
                          gpointer data );

void listbox_drag               ( GtkWidget * widget,
                          GdkDragContext * context,
                          GtkSelectionData * selection_data,
                          guint info,
                          guint32 clk_time,
                          gpointer data );

void listbox_get_drag_data          ( GtkWidget * widget,
                          GdkDragContext * context,
                          gint x,
                          gint y,
                          GtkSelectionData * selection_data,
                          guint info,
                          guint32 clk_time,
                          gpointer data );

/* gftp-gtk.c */
void gftp_gtk_init_request          ( gftp_window_data * wdata );

void toolbar_hostedit               ( GtkWidget * widget,
                          gpointer data );

void sortrows                   ( GtkTreeViewColumn * col,
                          gpointer data );

void stop_button                ( GtkWidget * widget,
                          gpointer data );

/* gtkui.c */
void gftpui_run_command             ( GtkWidget * widget, gpointer data );

void gftpui_mkdir_dialog            (GtkAction * a,  gpointer data );

void gftpui_rename_dialog           (GtkAction * a,  gpointer data );

void gftpui_site_dialog             (GtkAction * a,  gpointer data );

int gftpui_run_chdir                ( gpointer uidata, char *directory );

void gftpui_chdir_dialog            (GtkAction * a,  gpointer data );

char * gftpui_gtk_get_utf8_file_pos         ( gftp_file * fle );

/* menu_items.c */
void change_filespec                (GtkAction * a, gpointer data );

void save_directory_listing             (GtkAction * a, gpointer data );

void show_selected              (GtkAction * a, gpointer data );

void selectall                  (GtkAction * a, gpointer data );

void selectallfiles                 (GtkAction * a, gpointer data );

void deselectall                (GtkAction * a, gpointer data );

int chdir_edit                  ( GtkWidget * widget, gpointer data );

void clearlog                   (GtkAction * a,  gpointer data );

void viewlog                    (GtkAction * a,  gpointer data );

void savelog                    (GtkAction * a,  gpointer data );

void clear_cache                (GtkAction * a,  gpointer data );

void compare_windows                (GtkAction * a,  gpointer data );

void about_dialog               (GtkAction * a,  gpointer data );

void remove_files_window            ( gftp_window_data * wdata );

void ftp_log                    ( gftp_logging_level level,
                          gftp_request * request,
                          const char *string,
                          ... ) GFTP_LOG_FUNCTION_ATTRIBUTES;

void update_window_info             ( void );

void update_window              ( gftp_window_data * wdata );

gftp_graphic * open_xpm             ( char *filename );
void gftp_free_pixmap               ( char *filename );

GdkPixbuf * gftp_get_pixmap (char *filename);

int check_status                ( char *name,
                          gftp_window_data * wdata,
                          unsigned int check_other_stop,
                          unsigned int only_one,
                          unsigned int at_least_one,
                          unsigned int func );

void add_history                ( GtkWidget * widget,
                          GList ** history,
                          unsigned int *histlen,
                          const char *str );

int check_reconnect                 ( gftp_window_data * wdata );

void add_file_listbox               ( gftp_window_data * wdata,
                          gftp_file * fle );

char * MakeEditDialog ( char *diagtxt,
                        char *infotxt,
                        char *deftext,
                        int passwd_item,
                        char *checktext,
                        const char * yestext,
                        int * check );

int MakeYesNoDialog                ( char *diagtxt, char *infotxt );

void update_directory_download_progress     ( gftp_transfer * transfer );

int progress_timeout                ( gpointer data );

char * get_xpm_path                 ( char *filename,
                          int quit_on_err );

void gtk_combo_box_set_popdown_strings (GtkComboBoxText * combo, GList * string);

/* options_dialog.c */
void options_dialog                 (GtkAction * a,  gpointer data );

void gftp_gtk_setup_bookmark_options        ( GtkWidget * notebook );
void gftp_gtk_set_bookmark_options        ( gftp_bookmarks_var * bm );

void gftp_gtk_save_bookmark_options         ( void );

/* transfer.c */
int ftp_list_files              ( gftp_window_data * wdata );

int ftp_connect                 ( gftp_window_data * wdata,
                          gftp_request * request );

gint update_downloads               ( gpointer data );

void get_files                  (GtkAction * a,  gpointer data );

void put_files                  (GtkAction * a,  gpointer data );

void transfer_window_files          ( gftp_window_data * fromwdata,
                          gftp_window_data * towdata );

int gftp_gtk_get_subdirs            ( gftp_transfer * transfer );

void *do_getdir_thread              ( void * data );

void start_transfer             (GtkAction * a,  gpointer data );

void stop_transfer              (GtkAction * a,  gpointer data );

void skip_transfer              (GtkAction * a,  gpointer data );

void remove_file_transfer           (GtkAction * a,  gpointer data );

void move_transfer_up               (GtkAction * a,  gpointer data );

void move_transfer_down             (GtkAction * a,  gpointer data );

/* view_dialog.c */
void edit_dialog                (GtkAction * a,  gpointer data );

void view_dialog                (GtkAction * a,  gpointer data );

void view_file                  ( char *filename,
                          int fd,
                          unsigned int viewedit,
                          unsigned int del_file,
                          unsigned int start_pos,
                          unsigned int dontupload,
                          char *remote_filename,
                          gftp_window_data * wdata );


typedef struct _gftpui_callback_data gftpui_callback_data;

struct _gftpui_callback_data
{
  gftp_request * request;
  gftp_window_data * uidata;
  char *input_string,
       *source_string;
  GList * files;
  void *user_data;
  int retries;
  int (*run_function) (gftpui_callback_data * cdata);
  int dont_check_connection;
  int dont_refresh;
  int dont_clear_cache;
  int toggled;
};


typedef enum _gftpui_common_request_type
{
  gftpui_common_request_none,
  gftpui_common_request_local,
  gftpui_common_request_remote
} gftpui_common_request_type;


#define gftpui_common_use_threads(request)  (gftp_protocols[(request)->protonum].use_threads)

extern sigjmp_buf gftpui_common_jmp_environment;
extern volatile int gftpui_common_use_jmp_environment;
extern GStaticMutex gftpui_common_transfer_mutex;
extern volatile sig_atomic_t gftpui_common_child_process_done;

/* gftpui.c */
int gftpui_run_callback_function    ( gftpui_callback_data * cdata );

int gftpui_common_run_callback_function ( gftpui_callback_data * cdata );

int gftpui_common_cmd_open      ( void *uidata,
                      gftp_request * request,
                      void *other_uidata,
                      gftp_request * other_request,
                      const char *command );

gftp_transfer * gftpui_common_add_file_transfer ( gftp_request * fromreq,
                          gftp_request * toreq,
                          void *fromuidata,
                          void *touidata,
                          GList * files );

void gftpui_common_cancel_file_transfer ( gftp_transfer * tdata );

int gftpui_common_transfer_files    ( gftp_transfer * tdata );

/* gftpuicallback.c */
void gftpui_refresh             ( void *uidata,  int clear_cache_entry );

void gftpui_add_file_to_transfer    ( gftp_transfer * tdata, GList * curfle );

void gftpui_ask_transfer        ( gftp_transfer * tdata );

void gftpui_disconnect          ( void *uidata );

#endif
