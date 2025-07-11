/*
 Copyright (C) 1999-2004 IC & S  dbmail@ic-s.nl
 Copyright (c) 2004-2013 NFG Net Facilities Group BV support@nfg.nl
 Copyright (C) 2007 Aaron Stone aaron@serendity.cx
 Copyright (c) 2014-2019 Paul J Stevens, The Netherlands, support@nfg.nl
 Copyright (c) 2020-2025 Alan Hicks, Persistent Objects Ltd support@p-o.co.uk

 This program is free software; you can redistribute it and/or 
 modify it under the terms of the GNU General Public License 
 as published by the Free Software Foundation; either 
 version 2 of the License, or (at your option) any later 
 version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

/* 
 * This is the dbmail-export program to dump mailboxes out to mbox files.
 */

#include "dbmail.h"

extern char configFile[PATH_MAX];

#define PNAME "dbmail/export"
#define THIS_MODULE "export"

extern DBParam_T db_params;
extern Mempool_T small_pool;

#define DBPFX db_params.pfx

void do_showhelp(void)
{
	printf(
//	Try to stay under the standard 80 column width
//	0........10........20........30........40........50........60........70........80
	"*** dbmail-export ***\n"
	"Use this program to export your DBMail mailboxes.\n"
	"See the man page for more info. Summary:\n"
	"     -u, --user username   specify a user, wildcards ? and * accepted, but please be\n"
	"                           careful that you may need to escape these from your shell\n"
	"     -m, --mailbox mailbox specify a mailbox (default: export all mailboxes recursively)\n"
	"     -b, --basedir basedir specify the destination base dir (default: current directory)\n"
	"                           note that files are always opened in append mode\n"
	"     -o, --out outfile     specify the output file (default: stdout)\n"
	"                           note that files are always opened in append mode\n"
	"     -s, --search search   use an IMAP SEARCH string to select messages (default: 1:*)\n"
	"                           for example, to export all messages received in May:\n"
	"                           \"1:* SINCE 1-May-2007 BEFORE 1-Jun-2007\"\n"
	"     -d, --set-del-flag    set \\Deleted flag on exported messages\n"
	"     -D, --set-del-status  set delete status on exported messages\n"
	"                           note that dbmail-util can be used to set deleted status for\n"
	"                           \\Deleted messages, and to purge messages with deleted status\n"
	"     -r, --recursive       export mailboxes recursively\n"
	"                           (default: true unless -m option is specified)\n"
	"\nCommon options for all DBMail utilities:\n"
	"     -f, --config file     specify an alternative config file\n"
	"                           Default: %s\n"
	"     -q, --quiet           quietly skip interactive prompts\n"
	"                           use twice to suppress error messages\n"
	"     -n, --no              show the intended action but do not perform it,\n"
	"                           no to all\n"
	"     -y, --yes             perform all proposed actions, yes to all\n"
	"     -v, --verbose         verbose details\n"
	"     -V, --version         show the version\n"
	"     -h, --help            show this help message\n"
	, configFile);
}

static int mailbox_dump(uint64_t mailbox_idnr, const char *dumpfile,
		const char *search, int delete_after_dump)
{
	FILE *ostream;
	DbmailMailbox *mb = NULL;
	ImapSession *s = NULL;
	int result = 0;

	if (! search)
		search = "1:*";

	/* 
	 * For dbmail the usual filesystem semantics don't really 
	 * apply. Mailboxes can contain other mailboxes as well as
	 * messages. For now however, this is solved by appending
	 * the mailboxname with .mbox
	 *
	 * TODO: facilitate maildir type exports
	 */
	Mempool_T pool = mempool_open();
	mb = dbmail_mailbox_new(pool, mailbox_idnr);
	client_sock *c;
	s = dbmail_imap_session_new(mb->pool);
	c = mempool_pop(s->pool, sizeof(client_sock));
	c->pool = s->pool;
	s->ci = client_init(c);
	if (! (imap4_tokenizer_main(s, search))) {
		qprintf("error parsing search string\n");
		TRACE(TRACE_ERR, "[%p] Error parsing search string", s);
		dbmail_mailbox_free(mb);
		dbmail_imap_session_delete(&s);
		return 1;
	}

	if (dbmail_mailbox_build_imap_search(mb, s->args, &(s->args_idx), SEARCH_UNORDERED) < 0) {
		qprintf("Invalid search string\n");
		TRACE(TRACE_INFO, "[%p] Invalid search string", s);
		dbmail_mailbox_free(mb);
		dbmail_imap_session_delete(&s);
		return 1;
	}
	dbmail_mailbox_search(mb);

	if (strcmp(dumpfile, "-") == 0) {
		ostream = stdout;
	} else if (! (ostream = fopen(dumpfile, "a"))) {
		int err = errno;
		qprintf("Opening [%s] failed [%s]\n", dumpfile, strerror(err));
		TRACE(TRACE_ERR, "[%p] Opening [%s] failed [%s]", s, dumpfile, strerror(err));
		result = -1;
		goto cleanup;
	}

	if (dbmail_mailbox_dump(mb, ostream) < 0) {
		qprintf("Export failed\n");
		TRACE(TRACE_ERR, "[%p] Export failed", s);
		result = -1;
		goto cleanup;
	}

	if (delete_after_dump) {
		int count = 0;
		int affected = 0;
		int deleted_flag[IMAP_NFLAGS];
		memset(deleted_flag, 0, sizeof(deleted_flag));
		deleted_flag[IMAP_FLAG_DELETED] = 1;

		GList *ids = g_tree_keys(MailboxState_getIds(mb->mbstate));

                while (ids) {
			affected = 0;
			// Flag the selected messages \\Deleted
			// Following this, dbmail-util -d sets deleted status
			if (delete_after_dump & 1) {
				if (db_set_msgflag(*(uint64_t *)ids->data, deleted_flag, NULL, IMAPFA_ADD, 0, NULL) < 0) {
					qprintf("Error setting flags for message [%" PRIu64 "]\n", *(uint64_t *)ids->data);
					TRACE(TRACE_ERR, "[%p] Error setting flags for message [%" PRIu64 "]", s, *(uint64_t *)ids->data);
					result = -1;
				} else {
					affected = 1;
				}
			}

			// Set deleted status on each message
			// Following this, dbmail-util -p sets purge status
			if (delete_after_dump & 2) {
				if (! db_set_message_status(*(uint64_t *)ids->data, MESSAGE_STATUS_DELETE)) {
					qprintf("Error setting status for message [%" PRIu64 "]\n", *(uint64_t *)ids->data);
					TRACE(TRACE_ERR, "[%p] Error setting status for message [%" PRIu64 "]\n", s, *(uint64_t *)ids->data);
					result = -1;
				} else {
					affected = 1;
				}
			}
			if (affected)
				count++;

			if (!g_list_next(ids))
				break;
			ids = g_list_next(ids);
		}
		if (count)
			db_mailbox_seq_update(MailboxState_getId(mb->mbstate), 0);

		g_list_free(g_list_first(ids));
	}

cleanup:
	if (mb)
		dbmail_mailbox_free(mb);
	dbmail_imap_session_delete(&s);
	if ((ostream) && (ostream != stdout))
		fclose(ostream);

	return result;
}
	
static int do_export(char *user, char *base_mailbox, char *basedir, char *outfile, char *search, int delete_after_dump, int recursive)
{
	uint64_t user_idnr = 0, owner_idnr = 0, mailbox_idnr = 0;
	char *dumpfile = NULL, *mailbox = NULL, *search_mailbox = NULL, *dir = NULL;
	GList *children = NULL;
	int result = 0;

	/* Verify the existence of this user */
	if (! auth_user_exists(user, &user_idnr)) {
		qprintf("Error: user [%s] does not exist.\n", user);
		TRACE(TRACE_INFO, "User [%s] does not exist.\n", user);
		result = -1;
		goto cleanup;
	}

	mailbox = g_new0(char, IMAP_MAX_MAILBOX_NAMELEN);

	if (!base_mailbox) {
		/* Always recursive without a mailbox base */
		search_mailbox = g_strdup("*");
	} else if (recursive) {
		/* Base and everything below */
		search_mailbox = g_strdup_printf("%s*", base_mailbox);
	} else if (!recursive) {
		/* Should yield same results as plain db_findmailbox */
		search_mailbox = g_strdup_printf("%s", base_mailbox);
	}

	/* FIXME: What are the possible error conditions here? */
	db_findmailbox_by_regex(user_idnr, search_mailbox, &children, 0);

	/* Decision process for basedir vs. outfile:
	 *   If we're dumping one mailbox for one user, it goes to
	 *   stdout.  If we've been given -o -, dump everything to
	 *   stdout (e.g., one giant mbox).  If we've been given foo
	 */

	if (!outfile && !basedir) {
		/* Default is to use basedir of . */
		basedir = ".";
	} else if (outfile) {
		/* Everything goes into this one file */
		dumpfile = outfile;
	}

	children = g_list_first(children);

	qprintf("Exporting [%u] mailboxes for [%s]\n", g_list_length(children), user);

	while (children) {
		mailbox_idnr = *(uint64_t *)children->data;
		db_getmailboxname(mailbox_idnr, user_idnr, mailbox);			
		if (! db_get_mailbox_owner(mailbox_idnr, &owner_idnr)) {
			qprintf("Error checking mailbox ownership");
			TRACE(TRACE_ERR, "Error checking mailbox ownership");
			goto cleanup;
		}
		if (owner_idnr == user_idnr) {
			if (basedir) {
				/* Prepare the directory */
				dumpfile = g_strdup_printf("%s/%s/%s.mbox", basedir, user, mailbox);

				dir = g_path_get_dirname(dumpfile);
				if (g_mkdir_with_parents(dir, 0700)) {
					qprintf("Unable to create directory [%s]\n", dir);
					TRACE(TRACE_ERR, "Unable to create directory [%s]", dir);
					result = -1;
					goto cleanup;
				}
			}

			qprintf("Export mailbox %s -> %s\n", mailbox, dumpfile);
			TRACE(TRACE_INFO, "Export mailbox %s -> %s\n", mailbox, dumpfile);
			if ((result = mailbox_dump(mailbox_idnr, dumpfile, search, delete_after_dump)) != 0) {
				qprintf("Error exporting mailbox %s -> %s\n", mailbox, dumpfile);
				TRACE(TRACE_ERR, "Exporting mailbox %s -> %s\n", mailbox, dumpfile);
				goto cleanup;
			}

			if (delete_after_dump) db_update("UPDATE %smailboxes SET seq=seq+1 WHERE mailbox_idnr=%d",DBPFX,mailbox_idnr);

			if (basedir) {
				g_free(dir);
				g_free(dumpfile);
			}
		}
		if (! g_list_next(children)) break;
		children = g_list_next(children);
	}

cleanup:
	g_list_destroy(children);
	g_free(search_mailbox);
	g_free(mailbox);

	return result;
}

int main(int argc, char *argv[])
{
	int opt = 0;
	int option_index = 0;
	int show_help = 0;
	int result = 0, delete_after_dump = 0, recursive = 0;
	char *user=NULL, *mailbox=NULL, *outfile=NULL, *basedir=NULL, *search=NULL;

	openlog(PNAME, LOG_PID, LOG_MAIL);
	setvbuf(stdout, 0, _IONBF, 0);

	small_pool = mempool_open();
	g_mime_init();

	config_get_file();
	/* get options */
	static struct option long_options[] = {
		{"user", required_argument, NULL, 'u'},
		{"mailbox", required_argument, NULL, 'm'},
		{"basedir", required_argument, NULL, 'b'},
		{"out", required_argument, NULL, 'o'},
		{"search", required_argument, NULL, 's'},
		{"set-del-flag", no_argument, NULL, 'd'},
		{"set-del-status", no_argument, NULL, 'D'},
		{"recursive", no_argument, NULL, 'r'},

		{"config",    required_argument, NULL, 'f'},
		{"quiet",     no_argument, NULL, 'q'},
		{"no",        no_argument, NULL, 'n'},
		{"yes",       no_argument, NULL, 'y'},
		{"help",      no_argument, NULL, 'h'},
		{"verbose",   no_argument, NULL, 'v'},
		{"version",   no_argument, NULL, 'V'},
		{NULL,        0,           NULL, 0}
	};
	// Alan TODO
	//opterr = 0;		/* suppress error message from getopt() */
	while ((opt = getopt_long(argc, argv,
		"-u:m:o:b:s:dDr" /* Major modes */
		"f:qvVh", /* Common options */
		long_options, &option_index)) != -1) {

		switch (opt) {
		/* export specific options */
		case 'u':
			if (optarg && strlen(optarg))
				user = optarg;
			break;

		case 'm':
			if (optarg && strlen(optarg))
				mailbox = optarg;
			break;
		case 'b':
			if (optarg && strlen(optarg))
				basedir = optarg;
			break;
		case 'o':
			if (optarg && strlen(optarg))
				outfile = optarg;
			break;
		case 'd':
			delete_after_dump |= 1;
			break;
		case 'D':
			delete_after_dump |= 2;
			break;
		case 'r':
			recursive = 1;
			break;
		case 's':
			if (optarg && strlen(optarg))
				search = optarg;
			else {
				qprintf("dbmail-mailbox: --search requires a value\n\n");
				result = 1;
			}
			break;

		/* Common options */
		case 'f':
			if (optarg && strlen(optarg) > 0) {
				memset(configFile, 0, sizeof(configFile));
				strncpy(configFile, optarg, sizeof(configFile)-1);
			} else {
				qprintf("dbmail-mailbox: -f requires a filename\n\n");
				result = 1;
			}
			break;

		case 'h':
			show_help = 1;
			break;

		case 'q':
			/* If we get q twice, be really quiet! */
			if (quiet)
				reallyquiet = 1;
			if (!verbose)
				quiet = 1;
			break;

		case 'v':
			if (!quiet)
				verbose = 1;
			break;

		case 'V':
			/* Show the version and return non-zero. */
			PRINTF_THIS_IS_DBMAIL;
			result = 1;
			break;
		default:
			/* printf("unrecognized option [%c], continuing...\n",optopt); */
			break;
		}

		/* If there's a non-negative return code,
		 * it's time to free memory and bail out. */
		if (result)
			goto freeall;
	}	

	/* If nothing is happening, show the help text. */
	if (!user || (basedir && outfile) || show_help) {
		do_showhelp();
		result = 1;
		goto freeall;
	}
 
	/* read the config file */
        if (config_read(configFile) == -1) {
                qprintf("Failed. Unable to read config file %s\n", configFile);
                result = -1;
                goto freeall;
        }
                
	SetTraceLevel("EXPORT");
	GetDBParams();

	/* open database connection */
	if (db_connect() != 0) {
		qprintf ("Failed. Could not connect to database (check log)\n");
		TRACE(TRACE_ERR, "Could not connect to database");
		result = -1;
		goto freeall;
	}

	/* open authentication connection */
	if (auth_connect() != 0) {
		qprintf("Failed. Could not connect to authentication (check log)\n");
		TRACE(TRACE_ERR, "Could not connect to authentication");
		result = -1;
		goto freeall;
	}

	/* Loop over all user accounts if there's a wildcard. */
	if (strchr(user, '?') || strchr(user, '*')) {
		GList *all_users = auth_get_known_users();
		GList *matching_users = match_glob_list(user, all_users);
		GList *users = g_list_first(matching_users);

		if (!users) {
			qprintf("Error: no users matching [%s] were found.\n", user);
			TRACE(TRACE_ERR, "No users matching [%s]", user);
			g_list_destroy(all_users);
			result = -1;
			goto freeall;
		}

		while (users) {
			result = do_export(users->data, mailbox,
				basedir, outfile, search,
				delete_after_dump, recursive);

			if (!g_list_next(users))
				break;
			users = g_list_next(users);
		}

		g_list_destroy(all_users);
		g_list_destroy(matching_users);
	} else {
		/* No globbing, just run with this one user. */
		result = do_export(user, mailbox,
			basedir, outfile, search,
			delete_after_dump, recursive);
	}

	/* Here's where we free memory and quit.
	 * Be sure that all of these are NULL safe! */
freeall:

	db_disconnect();
	auth_disconnect();
	config_free();
	g_mime_shutdown();
	mempool_close(&small_pool);

	if (result) {
		qprintf("Export failed, see logfile for details.\n");
		TRACE(TRACE_INFO, "Export failed, see logfile for details.");
	} else {
		qprintf("Export succeeded.\n");
		TRACE(TRACE_INFO, "Export succeeded.");
	}
	return result;
}
