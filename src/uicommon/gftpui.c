/*****************************************************************************/
/*  gftpui.c - UI related functions for gFTP                                 */
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
/*  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111 USA      */
/*****************************************************************************/

#include "gftpui.h"
static const char cvsid[] = "$Id$";

sigjmp_buf gftpui_common_jmp_environment;
volatile int gftpui_common_use_jmp_environment = 0;

static void *gftpui_common_local_uidata, *gftpui_common_remote_uidata;
static gftp_request * gftpui_common_local_request,
                    * gftpui_common_remote_request;
GStaticMutex gftpui_common_transfer_mutex = G_STATIC_MUTEX_INIT;


static gftp_logging_func
_gftpui_common_log (gftp_request * request)
{
  if (request == NULL)
    return (gftpui_common_local_request->logging_function);
  else
    return (request->logging_function);
}


static void *
_gftpui_common_thread_callback (void * data)
{ 
  intptr_t network_timeout, sleep_time;
  gftpui_callback_data * cdata;
  int success, sj;

  cdata = data;
  gftp_lookup_request_option (cdata->request, "network_timeout",
                              &network_timeout);
  gftp_lookup_request_option (cdata->request, "sleep_time",
                              &sleep_time);

  sj = sigsetjmp (gftpui_common_jmp_environment, 1);
  gftpui_common_use_jmp_environment = 1;

  success = GFTP_ERETRYABLE;
  if (sj == 0)
    {
      while (1)
        {
          if (network_timeout > 0)
            alarm (network_timeout);
          success = cdata->run_function (cdata);
          alarm (0);

          if (success == GFTP_EFATAL || success == 0 || cdata->retries == 0)
            break;

          cdata->request->logging_function (gftp_logging_misc, cdata->request,
                       _("Waiting %d seconds until trying to connect again\n"),
                       sleep_time);
          alarm (sleep_time);
          pause ();
          cdata->retries--;
        }
    }
  else
    {
      gftp_disconnect (cdata->request);
      cdata->request->logging_function (gftp_logging_error, cdata->request,
                                        _("Operation canceled\n"));
    }

  gftpui_common_use_jmp_environment = 0;
  cdata->request->stopable = 0;

  return (GINT_TO_POINTER (success));
}


int
gftpui_common_run_callback_function (gftpui_callback_data * cdata)
{
  int ret;

  if (gftpui_check_reconnect (cdata) < 0)
    return (0);

  if (gftp_protocols[cdata->request->protonum].use_threads)
    ret = GPOINTER_TO_INT (gftpui_generic_thread (_gftpui_common_thread_callback, cdata));
  else
    ret = GPOINTER_TO_INT (cdata->run_function (cdata));

  if (ret == 0 && cdata->run_function != gftpui_common_run_ls)
    gftpui_refresh (cdata->uidata);

  return (ret == 0);
}


RETSIGTYPE
gftpui_common_signal_handler (int signo)
{
  signal (signo, gftpui_common_signal_handler);

  if (gftpui_common_use_jmp_environment)
    siglongjmp (gftpui_common_jmp_environment, signo == SIGINT ? 1 : 2);
  else if (signo == SIGINT)
    exit (1);
}


void
gftpui_common_about (gftp_logging_func logging_function, gpointer logdata)
{
  char *str;

  logging_function (gftp_logging_misc, logdata, "%s, Copyright (C) 1998-2003 Brian Masney <", gftp_version);
  logging_function (gftp_logging_recv, logdata, "masneyb@gftp.org");
  logging_function (gftp_logging_misc, logdata, _(">. If you have any questions, comments, or suggestions about this program, please feel free to email them to me. You can always find out the latest news about gFTP from my website at http://www.gftp.org/\n"));
  logging_function (gftp_logging_misc, logdata, _("gFTP comes with ABSOLUTELY NO WARRANTY; for details, see the COPYING file. This is free software, and you are welcome to redistribute it under certain conditions; for details, see the COPYING file\n"));

  str = _("Translated by");
  if (strcmp (str, "Translated by") != 0)
    logging_function (gftp_logging_misc, logdata, "%s\n", str);
}


static int
gftpui_common_cmd_about (void *uidata, gftp_request * request, char *command)
{
  gftpui_common_about (_gftpui_common_log (request),
                       gftpui_common_local_request);
  return (1);
}


static int
gftpui_common_cmd_ascii (void *uidata, gftp_request * request, char *command)
{
  gftp_set_global_option ("ascii_transfers", GINT_TO_POINTER(1));
  return (1);
}


static int
gftpui_common_cmd_binary (void *uidata, gftp_request * request, char *command)
{
  gftp_set_global_option ("ascii_transfers", GINT_TO_POINTER(0));
  return (1);
}


static int
gftpui_common_cmd_chmod (void *uidata, gftp_request * request, char *command)
{
  gftpui_callback_data * cdata;
  char *pos;

  if (!GFTP_IS_CONNECTED (request))
    {
      request->logging_function (gftp_logging_error, request,
                                 _("Error: Not connected to a remote site\n"));

      return (1);
    }

  if ((pos = strchr (command, ' ')) != NULL)
    *pos++ = '\0';

  if (*command == '\0' || pos == NULL || *pos == '\0')
    {
      request->logging_function (gftp_logging_error, request,
                                 _("usage: chmod <mode> <file>\n"));
    }
  else
    {
      cdata = g_malloc0 (sizeof (*cdata));
      cdata->request = request;
      cdata->uidata = uidata;
      cdata->input_string = command;
      cdata->source_string = pos;
      cdata->run_function = gftpui_common_run_chmod;

      gftpui_common_run_callback_function (cdata);

      g_free (cdata);
    }

  return (1);
}


static int
gftpui_common_cmd_rename (void *uidata, gftp_request * request, char *command)
{
  gftpui_callback_data * cdata;
  char *pos;
  
  if (!GFTP_IS_CONNECTED (request))
    {
      request->logging_function (gftp_logging_error, request,
                                 _("Error: Not connected to a remote site\n"));
      return (1);
    }
    
  if ((pos = strchr (command, ' ')) != NULL)
    *pos++ = '\0';
    
  if (*command == '\0' || pos == NULL || *pos == '\0')
    {
      request->logging_function (gftp_logging_error, request,
                                 _("usage: rename <old name> <new name>\n"));
    }
  else
    {
      cdata = g_malloc0 (sizeof (*cdata));
      cdata->request = request;
      cdata->uidata = uidata;
      cdata->source_string = command;
      cdata->input_string = pos;
      cdata->run_function = gftpui_common_run_rename;

      gftpui_common_run_callback_function (cdata);

      g_free (cdata);
    }

  return (1);
}


static int
gftpui_common_cmd_delete (void *uidata, gftp_request * request, char *command)
{
  gftpui_callback_data * cdata;

  if (!GFTP_IS_CONNECTED (request))
    {
      request->logging_function (gftp_logging_error, request,
                                 _("Error: Not connected to a remote site\n"));
      return (1);
    }
  else if (*command == '\0')
    {
      request->logging_function (gftp_logging_error, request,
                                 _("usage: delete <file>\n"));
    }
  else
    {
      cdata = g_malloc0 (sizeof (*cdata));
      cdata->request = request;
      cdata->uidata = uidata;
      cdata->input_string = command;
      cdata->run_function = gftpui_common_run_delete;

      gftpui_common_run_callback_function (cdata);

      g_free (cdata);
    }

  return (1);
}


static int
gftpui_common_cmd_rmdir (void *uidata, gftp_request * request, char *command)
{
  gftpui_callback_data * cdata;

  if (!GFTP_IS_CONNECTED (request))
    {
      request->logging_function (gftp_logging_error, request,
                                 _("Error: Not connected to a remote site\n"));
      return (1);
    }
  else if (*command == '\0')
    {
      request->logging_function (gftp_logging_error, request,
                                 _("usage: rmdir <directory>\n"));
    }
  else
    {
      cdata = g_malloc0 (sizeof (*cdata));
      cdata->request = request;
      cdata->uidata = uidata;
      cdata->input_string = command;
      cdata->run_function = gftpui_common_run_rmdir;

      gftpui_common_run_callback_function (cdata);

      g_free (cdata);
    }

  return (1);
}


static int
gftpui_common_cmd_site (void *uidata, gftp_request * request, char *command)
{
  gftpui_callback_data * cdata;

  if (!GFTP_IS_CONNECTED (request))
    {
      request->logging_function (gftp_logging_error, request,
                                 _("Error: Not connected to a remote site\n"));
      return (1);
    }
  else if (*command == '\0')
    {
      request->logging_function (gftp_logging_error, request,
                     _("usage: site <site command>\n"));
    }
  else
    {
      cdata = g_malloc0 (sizeof (*cdata));
      cdata->request = request;
      cdata->uidata = uidata;
      cdata->input_string = command;
      cdata->run_function = gftpui_common_run_site;

      gftpui_common_run_callback_function (cdata);

      g_free (cdata);
    }

  return (1);
}


static int
gftpui_common_cmd_mkdir (void *uidata, gftp_request * request, char *command)
{
  gftpui_callback_data * cdata;

  if (!GFTP_IS_CONNECTED (request))
    {
      request->logging_function (gftp_logging_error, request,
                                 _("Error: Not connected to a remote site\n"));
      return (1);
    }
  else if (*command == '\0')
    {
      request->logging_function (gftp_logging_error, request,
                     _("usage: mkdir <new directory>\n"));
    }
  else
    {
      cdata = g_malloc0 (sizeof (*cdata));
      cdata->request = request;
      cdata->uidata = uidata;
      cdata->input_string = command;
      cdata->run_function = gftpui_common_run_mkdir;

      gftpui_common_run_callback_function (cdata);

      g_free (cdata);
    }

  return (1);
}


static int
gftpui_common_cmd_chdir (void *uidata, gftp_request * request, char *command)
{
  gftpui_callback_data * cdata;
  char *tempstr, *newdir = NULL;

  if (!GFTP_IS_CONNECTED (request))
    {
      request->logging_function (gftp_logging_error, request,
                                 _("Error: Not connected to a remote site\n"));
      return (1);
    }
  else if (*command == '\0')
    {
      request->logging_function (gftp_logging_error, request,
                                 _("usage: chdir <directory>\n"));
      return (1);
    }
  else if (request->protonum == GFTP_LOCAL_NUM)
    {
      if (*command != '/' && request->directory != NULL)
        {
          tempstr = gftp_build_path (request->directory, command, NULL);
          newdir = expand_path (tempstr);
          g_free (tempstr);
        }
      else
        newdir = expand_path (command);

      if (newdir == NULL)
        {
          request->logging_function (gftp_logging_error, request,
                                     _("usage: chdir <directory>\n"));
          return (1);
        }
    }

  cdata = g_malloc0 (sizeof (*cdata));
  cdata->request = request;
  cdata->uidata = uidata;
  cdata->input_string = newdir != NULL ? newdir : command;
  cdata->run_function = gftpui_common_run_chdir;

  gftpui_common_run_callback_function (cdata);

  g_free (cdata);

  if (newdir != NULL)
    g_free (newdir);

  return (1);
}


static int
gftpui_common_cmd_close (void *uidata, gftp_request * request, char *command)
{
  gftp_disconnect (request);
  return (1);
}


static int
gftpui_common_cmd_pwd (void *uidata, gftp_request * request, char *command)
{
  if (!GFTP_IS_CONNECTED (request))
    {
      request->logging_function (gftp_logging_error, request,
                                 _("Error: Not connected to a remote site\n"));
      return (1);
    }

  request->logging_function (gftp_logging_misc, request,
                             "%s\n", request->directory);

  return (1);
}


static int
gftpui_common_cmd_quit (void *uidata, gftp_request * request, char *command)
{
  gftp_shutdown();

  return (0);
}


static int
gftpui_common_cmd_clear (void *uidata, gftp_request * request, char *command)
{
  gftp_logging_func logfunc;

  if (strcasecmp (command, "cache") == 0)
    gftp_clear_cache_files ();
  else
    {
      logfunc = _gftpui_common_log (request);
      logfunc (gftp_logging_error, request, _("Invalid argument\n"));
    }

  return (1);
}


static int
gftpui_common_clear_show_subhelp (char *topic)
{
  gftp_logging_func logfunc;

  logfunc = gftpui_common_local_request->logging_function;
  if (strcmp (topic, "cache") == 0)
    {
      logfunc (gftp_logging_misc, NULL, _("Clear the directory cache\n"));
      return (1);
    }

  return (0);
}


static int
gftpui_common_set_show_subhelp (char *topic)
{ 
  gftp_logging_func logfunc;
  gftp_config_vars * cv;
    
  logfunc = gftpui_common_local_request->logging_function;
  if ((cv = g_hash_table_lookup (gftp_global_options_htable, topic)) != NULL)
    {
      logfunc (gftp_logging_misc, NULL, "%s\n", cv->comment);
      return (1);
    }

  return (0);
}   


static int
gftpui_common_cmd_ls (void *uidata, gftp_request * request, char *command)
{
  char *startcolor, *endcolor, *tempstr;
  gftpui_callback_data * cdata;
  GList * templist;
  gftp_file * fle;

  if (!GFTP_IS_CONNECTED (request))
    {
      request->logging_function (gftp_logging_error, request,
                     _("Error: Not connected to a remote site\n"));
      return (1);
    }

  cdata = g_malloc0 (sizeof (*cdata));
  cdata->request = request;
  cdata->uidata = uidata;
  cdata->source_string = *command != '\0' ? command : NULL;
  cdata->run_function = gftpui_common_run_ls;

  gftpui_common_run_callback_function (cdata);

  templist = cdata->files;
  while (templist != NULL)
    {
      fle = templist->data;

      gftpui_lookup_file_colors (fle, &startcolor, &endcolor);
      tempstr = gftp_gen_ls_string (fle, startcolor, endcolor);
      request->logging_function (gftp_logging_misc_nolog, request, "%s\n",
                                 tempstr);
      g_free (tempstr);

      templist = templist->next;
      gftp_file_destroy (fle);
      g_free (fle);
    }


  if (cdata->files != NULL)
    g_list_free (cdata->files);
  g_free (cdata);

  return (1);
}


int
gftpui_common_cmd_open (void *uidata, gftp_request * request, char *command)
{
  gftpui_callback_data * cdata;
  intptr_t retries;
  char *tempstr;

  if (GFTP_IS_CONNECTED (request))
    {
      gftp_disconnect (request); /* FIXME */
    }
  
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

  if (request->need_userpass)
    {
      if (request->username == NULL || *request->username == '\0')
        {
          if ((tempstr = gftpui_prompt_username (uidata, request)) != NULL)
            {
              gftp_set_username (request, tempstr);
              gftp_set_password (request, NULL);
              g_free (tempstr);
            }
        }

      if (request->username != NULL &&
          strcmp (request->username, "anonymous") != 0 &&
          (request->password == NULL || *request->password == '\0'))
        {
          if ((tempstr = gftpui_prompt_password (uidata, request)) != NULL)
            {
              gftp_set_password (request, tempstr);
              g_free (tempstr);               
            }
        }
    }

  gftp_lookup_request_option (request, "retries", &retries);

  cdata = g_malloc0 (sizeof (*cdata));
  cdata->request = request;
  cdata->uidata = uidata;
  cdata->run_function = gftpui_common_run_connect;
  cdata->retries = retries;

  gftpui_common_run_callback_function (cdata);

  g_free (cdata);

  return (1);
}


static int
gftpui_common_cmd_set (void *uidata, gftp_request * request, char *command)
{
  gftp_config_vars * cv, newcv;
  gftp_logging_func logfunc;
  char *pos, *backpos;
  GList * templist;
  int i;
  
  logfunc = _gftpui_common_log (request);

  if (command == NULL || *command == '\0')
    {
      for (templist = gftp_options_list;
           templist != NULL; 
           templist = templist->next)
        {
          cv = templist->data;

          for (i=0; cv[i].key != NULL; i++)
            { 
              if (!(cv[i].ports_shown & GFTP_PORT_TEXT))
                continue;

              if (*cv[i].key == '\0' ||
                  gftp_option_types[cv[i].otype].write_function == NULL)
                continue;

              printf ("%s = ", cv[i].key);
              gftp_option_types[cv[i].otype].write_function (&cv[i], stdout, 0);
              printf ("\n");
            }
        }
    }
  else
    {
      if ((pos = strchr (command, '=')) == NULL)
        {
          logfunc (gftp_logging_error, request,
                   _("usage: set [variable = value]\n"));
          return (1);
        }
      *pos = '\0';

      for (backpos = pos - 1;
           (*backpos == ' ' || *backpos == '\t') && backpos > command;
           backpos--)
        *backpos = '\0';
      for (++pos; *pos == ' ' || *pos == '\t'; pos++);

      if ((cv = g_hash_table_lookup (gftp_global_options_htable, command)) == NULL)
        {
          logfunc (gftp_logging_error, request,
                   _("Error: Variable %s is not a valid configuration variable.\n"), command);
          return (1);
        }

      if (!(cv->ports_shown & GFTP_PORT_TEXT))
        {
          logfunc (gftp_logging_error, request,
                   _("Error: Variable %s is not available in the text port of gFTP\n"), command);
          return (1);
        }

      if (gftp_option_types[cv->otype].read_function != NULL)
        {
          memcpy (&newcv, cv, sizeof (newcv));
          newcv.flags &= ~GFTP_CVARS_FLAGS_DYNMEM;

          gftp_option_types[cv->otype].read_function (pos, &newcv, 1);

          gftp_set_global_option (command, newcv.value);

          if (newcv.flags & GFTP_CVARS_FLAGS_DYNMEM)
            g_free (newcv.value);
        }
    }

  return (1);
}


static int
gftpui_common_cmd_help (void *uidata, gftp_request * request, char *command)
{
  int i, j, ele, numrows, numcols = 6, handled, number_commands;
  char *pos;

  for (number_commands=0;
       gftpui_common_commands[number_commands].command != NULL;
       number_commands++);

  if (command != NULL && *command != '\0')
    {
      for (pos = command; *pos != ' ' && *pos != '\0'; pos++);
      if (*pos == ' ')
        {
          *pos++ = '\0';
          if (*pos == '\0')
            pos = NULL;
        }
      else
        pos = NULL;

      for (i=0; gftpui_common_commands[i].command != NULL; i++)
        {
          if (strcmp (gftpui_common_commands[i].command, command) == 0)
            break;
        }

      if (gftpui_common_commands[i].cmd_description != NULL)
        {
          if (pos != NULL && gftpui_common_commands[i].subhelp_func != NULL)
            handled = gftpui_common_commands[i].subhelp_func (pos);
          else
            handled = 0;

          if (!handled)
            printf ("%s\n", _(gftpui_common_commands[i].cmd_description));
        }
      else
        *command = '\0';
    }
  
  if (command == NULL || *command == '\0')
    {
      numrows = number_commands / numcols;
      if (number_commands % numcols != 0)
        numrows++;

      printf (_("Supported commands:\n\n"));
      for (i=0; i<numrows; i++)
        {
          printf ("     ");
          for (j=0; j<numcols; j++)
            {
              ele = i + j * numrows;
              if (ele >= number_commands)
                break;
              printf ("%-10s", gftpui_common_commands[ele].command);
            }
         printf ("\n");
        }

      printf ("\n");
    }
  return (1);
}


gftpui_common_methods gftpui_common_commands[] = {
        {N_("about"),   2, gftpui_common_cmd_about, gftpui_common_request_none,
         N_("Shows gFTP information"), NULL},
        {N_("ascii"),   2, gftpui_common_cmd_ascii, gftpui_common_request_remote,
         N_("Sets the current file transfer mode to Ascii (only for FTP)"), NULL},
        {N_("binary"),  1, gftpui_common_cmd_binary, gftpui_common_request_remote,
         N_("Sets the current file transfer mode to Binary (only for FTP)"), NULL},
        {N_("cd"),      2, gftpui_common_cmd_chdir, gftpui_common_request_remote,
         N_("Changes the remote working directory"), NULL},
        {N_("chdir"),   3, gftpui_common_cmd_chdir, gftpui_common_request_remote,
         N_("Changes the remote working directory"), NULL},
        {N_("chmod"),   3, gftpui_common_cmd_chmod, gftpui_common_request_remote,
         N_("Changes the permissions of a remote file"), NULL},
        {N_("clear"),   3, gftpui_common_cmd_clear, gftpui_common_request_none,
         N_("Available options: cache"), gftpui_common_clear_show_subhelp},
        {N_("close"),   3, gftpui_common_cmd_close, gftpui_common_request_remote,
         N_("Disconnects from the remote site"), NULL},
        {N_("delete"),  1, gftpui_common_cmd_delete, gftpui_common_request_remote,
         N_("Removes a remote file"), NULL},
/* FIXME
        {N_("get"),     1, gftp_text_mget_file, gftpui_common_request_none,
         N_("Downloads remote file(s)"), NULL},
*/
        {N_("help"),    1, gftpui_common_cmd_help, gftpui_common_request_none,
         N_("Shows this help screen"), NULL},
        {N_("lcd"),     3, gftpui_common_cmd_chdir, gftpui_common_request_local,
         N_("Changes the local working directory"), NULL},
        {N_("lchdir"),  4, gftpui_common_cmd_chdir, gftpui_common_request_local,
         N_("Changes the local working directory"), NULL},
        {N_("lchmod"),  4, gftpui_common_cmd_chmod,     gftpui_common_request_local,
         N_("Changes the permissions of a local file"), NULL},
        {N_("ldelete"), 2, gftpui_common_cmd_delete,    gftpui_common_request_local,
         N_("Removes a local file"), NULL},
        {N_("lls"),     2, gftpui_common_cmd_ls, gftpui_common_request_local,
         N_("Shows the directory listing for the current local directory"), NULL},
        {N_("lmkdir"),  2, gftpui_common_cmd_mkdir, gftpui_common_request_local,
         N_("Creates a local directory"), NULL},
        {N_("lpwd"),    2, gftpui_common_cmd_pwd, gftpui_common_request_local,
         N_("Show current local directory"), NULL},
        {N_("lrename"), 3, gftpui_common_cmd_rename, gftpui_common_request_local,
         N_("Rename a local file"), NULL},
        {N_("lrmdir"),  3, gftpui_common_cmd_rmdir, gftpui_common_request_local,
         N_("Remove a local directory"), NULL},
        {N_("ls"),      2, gftpui_common_cmd_ls, gftpui_common_request_remote,
         N_("Shows the directory listing for the current remote directory"), NULL},
/* FIXME
        {N_("mget"),    2, gftp_text_mget_file, gftpui_common_request_none,
         N_("Downloads remote file(s)"), NULL},
*/
        {N_("mkdir"),   2, gftpui_common_cmd_mkdir,     gftpui_common_request_remote,
         N_("Creates a remote directory"), NULL},
/* FIXME
        {N_("mput"),    2, gftp_text_mput_file, gftpui_common_request_none,
         N_("Uploads local file(s)"), NULL},
*/
        {N_("open"),    1, gftpui_common_cmd_open, gftpui_common_request_remote,
         N_("Opens a connection to a remote site"), NULL},
/* FIXME
        {N_("put"),     2, gftp_text_mput_file, gftpui_common_request_none,
         N_("Uploads local file(s)"), NULL},
*/
        {N_("pwd"),     2, gftpui_common_cmd_pwd, gftpui_common_request_remote,
         N_("Show current remote directory"), NULL},
        {N_("quit"),    1, gftpui_common_cmd_quit, gftpui_common_request_none,
         N_("Exit from gFTP"), NULL},
        {N_("rename"),  2, gftpui_common_cmd_rename, gftpui_common_request_remote,
         N_("Rename a remote file"), NULL},
        {N_("rmdir"),   2, gftpui_common_cmd_rmdir, gftpui_common_request_remote,
         N_("Remove a remote directory"), NULL},
        {N_("set"),     1, gftpui_common_cmd_set, gftpui_common_request_none,
         N_("Show configuration file variables. You can also set variables by set var=val"), gftpui_common_set_show_subhelp},
        {N_("site"),    2, gftpui_common_cmd_site, gftpui_common_request_remote,
         N_("Run a site specific command"), NULL},
        {NULL,          0, NULL,                gftpui_common_request_none,
	 NULL, NULL}};


int
gftpui_common_init (void *locui, gftp_request * locreq,
                    void *remui, gftp_request * remreq)
{
  gftpui_common_local_uidata = locui;
  gftpui_common_local_request = locreq;

  gftpui_common_remote_uidata = remui;
  gftpui_common_remote_request = remreq;

  return (0);
}


int
gftpui_common_process_command (const char *command)
{
  const char *stpos;
  char *pos, *newstr;
  gftp_request * request;
  size_t cmdlen;
  void *uidata;
  int ret, i;
  size_t len;

  for (stpos = command; *stpos == ' ' || *stpos == '\t'; stpos++);

  newstr = g_strdup (stpos);
  len = strlen (newstr);

  if (len > 0 && newstr[len - 1] == '\n')
    newstr[--len] = '\0';
  if (len > 0 && newstr[len - 1] == '\r')
    newstr[--len] = '\0';

  for (pos = newstr + len - 1;
       (*pos == ' ' || *pos == '\t') && pos > newstr;
       pos--)
    *pos = '\0';

  if (*stpos == '\0')
    {
      g_free (newstr);
      return (1);
    }

  if ((pos = strchr (newstr, ' ')) != NULL)
    *pos = '\0'; 

  cmdlen = strlen (newstr);
  for (i=0; gftpui_common_commands[i].command != NULL; i++)
    {
      if (strcmp (gftpui_common_commands[i].command, newstr) == 0)
        break;
      else if (cmdlen >= gftpui_common_commands[i].minlen &&
               strncmp (gftpui_common_commands[i].command, newstr, cmdlen) == 0)
        break; 
    }

  if (pos != NULL)
    pos++;
  else
    pos = "";
  
  if (gftpui_common_commands[i].command != NULL)
    {
      if (gftpui_common_commands[i].reqtype == gftpui_common_request_local)
        {
          request = gftpui_common_local_request;
          uidata = gftpui_common_local_uidata;
        }
      else if (gftpui_common_commands[i].reqtype == gftpui_common_request_remote)
        {
          request = gftpui_common_remote_request;
          uidata = gftpui_common_remote_uidata;
        }
      else
        {
          request = NULL;
          uidata = NULL;
        }
     
      ret = gftpui_common_commands[i].func (uidata, request, pos);
    }
  else
    {
      gftpui_common_local_request->logging_function (gftp_logging_error,
                                       gftpui_common_local_request,
                                       _("Error: Command not recognized\n"));
      ret = 1;
    }

  g_free (newstr);
  return (ret);
}


gftp_transfer *
gftpui_common_add_file_transfer (gftp_request * fromreq, gftp_request * toreq,
                                 void *fromuidata, void *touidata,
                                 GList * files)
{
  intptr_t append_transfers, one_transfer;
  GList * templist, *curfle;
  gftp_transfer * tdata;
  gftp_file * tempfle;
  int show_dialog;
  char *pos;
  
  for (templist = files; templist != NULL; templist = templist->next)
    { 
      tempfle = templist->data;
      if (tempfle->startsize > 0)
        break;
    }
  show_dialog = templist != NULL;
  
  gftp_lookup_request_option (fromreq, "append_transfers",
                              &append_transfers);
  gftp_lookup_request_option (fromreq, "one_transfer",
                              &one_transfer);

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

              if (tempfle->isdir)
                tdata->numdirs++;
              else
                tdata->numfiles++;

              if (tempfle->transfer_action != GFTP_TRANS_ACTION_SKIP)
                tdata->total_bytes += tempfle->size;

              if ((pos = strrchr (tempfle->file, '/')) == NULL)
                pos = tempfle->file;
              else
                pos++;

              gftpui_add_file_to_transfer (tdata, curfle, pos);
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
          if (tempfle->isdir)
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


static void
_gftpui_common_setup_fds (gftp_transfer * tdata, gftp_file * curfle,
                          int *fromfd, int *tofd)
{
  *tofd = -1;
  *fromfd = -1;

  if (curfle->is_fd)
    {
      if (tdata->toreq->protonum == GFTP_LOCAL_NUM)
        *tofd = curfle->fd;
      else if (tdata->fromreq->protonum == GFTP_LOCAL_NUM)
        *fromfd = curfle->fd;
    }
}


static void
_gftpui_common_done_with_fds (gftp_transfer * tdata, gftp_file * curfle)
{
  if (curfle->is_fd)
    {
      if (tdata->toreq->protonum == GFTP_LOCAL_NUM)
        tdata->toreq->datafd = -1;
      else
        tdata->fromreq->datafd = -1;
    }
}


int
gftpui_common_transfer_files (gftp_transfer * tdata)
{
  int i, mode, tofd, fromfd;
  intptr_t preserve_permissions;
  char buf[8192];
  off_t fromsize, total;
  gftp_file * curfle; 
  ssize_t num_read, ret;

  tdata->curfle = tdata->files;
  gettimeofday (&tdata->starttime, NULL);
  memcpy (&tdata->lasttime, &tdata->starttime,
          sizeof (tdata->lasttime));

  gftp_lookup_request_option (tdata->fromreq, "preserve_permissions",
                              &preserve_permissions);

  while (tdata->curfle != NULL)
    {
      num_read = -1;

      if (g_thread_supported ())
        g_static_mutex_lock (&tdata->structmutex);

      curfle = tdata->curfle->data;
      tdata->current_file_number++;

      if (g_thread_supported ())
        g_static_mutex_unlock (&tdata->structmutex);

      if (curfle->transfer_action == GFTP_TRANS_ACTION_SKIP)
        {
          if (g_thread_supported ())
            g_static_mutex_lock (&tdata->structmutex);

          tdata->next_file = 1;
          tdata->curfle = tdata->curfle->next;

          if (g_thread_supported ())
            g_static_mutex_unlock (&tdata->structmutex);
          continue;
        }

      fromsize = -1;
      if (gftp_connect (tdata->fromreq) == 0 &&
          gftp_connect (tdata->toreq) == 0)
        {
          if (curfle->isdir)
            {
              if (tdata->toreq->mkdir != NULL)
                {
                  tdata->toreq->mkdir (tdata->toreq, curfle->destfile);
                  if (!GFTP_IS_CONNECTED (tdata->toreq))
                    break;
                }

              if (g_thread_supported ())
                g_static_mutex_lock (&tdata->structmutex);

              tdata->next_file = 1;
              tdata->curfle = tdata->curfle->next;

              if (g_thread_supported ())
                g_static_mutex_unlock (&tdata->structmutex);
              continue;
            }

          _gftpui_common_setup_fds (tdata, curfle, &fromfd, &tofd);

          if (curfle->size == 0)
            {
              curfle->size = gftp_get_file_size (tdata->fromreq, curfle->file);
              tdata->total_bytes += curfle->size;
            }

          if (GFTP_IS_CONNECTED (tdata->fromreq) &&
              GFTP_IS_CONNECTED (tdata->toreq))
            {
              fromsize = gftp_transfer_file (tdata->fromreq, curfle->file,
                          fromfd,
                          curfle->transfer_action == GFTP_TRANS_ACTION_RESUME ?
                                                    curfle->startsize : 0,
                          tdata->toreq, curfle->destfile, tofd,
                          curfle->transfer_action == GFTP_TRANS_ACTION_RESUME ?
                                                    curfle->startsize : 0);
            }
        }

      if (!GFTP_IS_CONNECTED (tdata->fromreq) ||
          !GFTP_IS_CONNECTED (tdata->toreq))
        {
          tdata->fromreq->logging_function (gftp_logging_misc,
                         tdata->fromreq,
                         _("Error: Remote site disconnected after trying to tdata file\n"));
        }
      else if (fromsize < 0)
        {
          if (g_thread_supported ())
            g_static_mutex_lock (&tdata->structmutex);

          curfle->transfer_action = GFTP_TRANS_ACTION_SKIP;
          tdata->next_file = 1;
          tdata->curfle = tdata->curfle->next;

          if (g_thread_supported ())
          g_static_mutex_unlock (&tdata->structmutex);
          continue;
        }
      else
        {
          if (g_thread_supported ())
            g_static_mutex_lock (&tdata->structmutex);

          tdata->curtrans = 0;
          tdata->curresumed = curfle->transfer_action == GFTP_TRANS_ACTION_RESUME ? curfle->startsize : 0;
          tdata->resumed_bytes += tdata->curresumed;

          if (g_thread_supported ())
            g_static_mutex_unlock (&tdata->structmutex);

          total = 0;
          i = 0;
          while (!tdata->cancel &&
                 (num_read = gftp_get_next_file_chunk (tdata->fromreq,
                                                       buf, sizeof (buf))) > 0)
            {
              total += num_read;
              gftp_calc_kbs (tdata, num_read);

              if ((ret = gftp_put_next_file_chunk (tdata->toreq, buf,
                                                   num_read)) < 0)
                {
                  num_read = (int) ret;
                  break;
                }
            }
        }

      if (tdata->cancel)
        {
          if (gftp_abort_transfer (tdata->fromreq) != 0)
            gftp_disconnect (tdata->fromreq);

          if (gftp_abort_transfer (tdata->toreq) != 0)
            gftp_disconnect (tdata->toreq);
        }
      else if (num_read < 0)
        {
          tdata->fromreq->logging_function (gftp_logging_misc,
                                        tdata->fromreq,
                                        _("Could not download %s from %s\n"),
                                        curfle->file,
                                        tdata->fromreq->hostname);

          if (gftp_get_transfer_status (tdata, num_read) == GFTP_ERETRYABLE)
            continue;

          break;
        }
      else
        {
          _gftpui_common_done_with_fds (tdata, curfle);
          if (gftp_end_transfer (tdata->fromreq) != 0)
            {
              if (gftp_get_transfer_status (tdata, -1) == GFTP_ERETRYABLE)
                continue;

              break;
            }
          gftp_end_transfer (tdata->toreq);

          tdata->fromreq->logging_function (gftp_logging_misc,
                         tdata->fromreq,
                         _("Successfully tdatared %s at %.2f KB/s\n"),
                         curfle->file, tdata->kbs);
        }

      if (!curfle->is_fd && preserve_permissions)
        {
          if (curfle->attribs)
            {
              mode = gftp_parse_attribs (curfle->attribs);
              if (mode != 0)
                gftp_chmod (tdata->toreq, curfle->destfile, mode);
            }

          if (curfle->datetime != 0)
            gftp_set_file_time (tdata->toreq, curfle->destfile,
                                curfle->datetime);
        }

      if (g_thread_supported ())
        g_static_mutex_lock (&tdata->structmutex);

      tdata->curtrans = 0;
      tdata->next_file = 1;
      curfle->transfer_done = 1;
      tdata->curfle = tdata->curfle->next;

      if (g_thread_supported ())
        g_static_mutex_unlock (&tdata->structmutex);

      if (tdata->cancel && !tdata->skip_file)
        break;
      tdata->cancel = 0;
      tdata->fromreq->cancel = 0;
      tdata->toreq->cancel = 0;
    }
  tdata->done = 1;

  return (1); /* FIXME */
}
