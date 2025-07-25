/*
 Copyright (C) 1999-2004 IC & S  dbmail@ic-s.nl
 Copyright (c) 2004-2013 NFG Net Facilities Group BV support@nfg.nl
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
 * This is the dbmail-users command line utility
 * It makes managing users easier
 * */

#include "dbmail.h"

extern char configFile[PATH_MAX];

#define SHADOWFILE "/etc/shadow"
#define THIS_MODULE "users"
#define PNAME "dbmail/users"

struct change_flags {
	unsigned int newuser         : 1;
	unsigned int newmaxmail      : 1;
	unsigned int newclientid     : 1;
	unsigned int newpasswd       : 1;
	unsigned int newpasswdfile   : 1;
	unsigned int newpasswdstdin  : 1;
	unsigned int newpasswdshadow : 1;
	unsigned int newspasswd      : 1;
	unsigned int newsaction      : 1;
	unsigned int enable          : 1;
	unsigned int disable         : 1;
};


int do_showhelp(void)
{
	printf(
//	Try to stay under the standard 80 column width
//	0........10........20........30........40........50........60........70........80
	"*** dbmail-users ***\n"
	"Use this program to manage your DBMail users.\n"
	"You are highly recommended to review the dbmail-users man page\n"
	"\nModes of operation:\n"
	"     -a, --add-user user      add a user\n"
	"     -d, --delete-user user   delete a user\n"
	"     -c, --change-user user   change details for a user\n"
	"     -e, --empty-user-mailboxes user   empty all mailboxes for a user\n"
	"     -l, --list-user user     list information for matching users\n"
	"     -L, --list-users         list information for all users\n"
	"     -x, --forward address    create an external forwarding address\n"
	"                              use with --add-forward and --delete-forward\n"
	"\nSummary of options for all modes:\n"
	"     -w, --password passwd    specify user's password on the command line\n"
	"     -W, --pw-file file       read from a file or prompt for a user's password\n"
	"     -p, --pw-type pwtype     password type may be one of the following:\n"
	"               plaintext, crypt, md5-hash, md5-digest, md5-base64,\n"
	"               whirlpool, sha512, sha256, sha1, tiger\n"
	"               each type may be given a '-raw' suffix to indicate\n"
	"               that the password argument has already been encoded.\n"
	"     -P, --pw-shadow file     pull encrypted password from the shadow file\n"
	"     -u, --user user          new username (only useful for -c, change)\n"
	"     -g, --client client      assign the user to a client\n"
	"     -m, --max-quota max      set the maximum mail quota in <bytes>B,\n"
	"                              <kbytes>K, or <mbytes>M, default in bytes\n"
	"                              specify 0 to remove any mail quota limits\n"
	"     -s, --add-alias alias    adds a list of recipient aliases\n"
	"     -S, --delete-alias alias removes a list of recipient aliases\n"
	"                              (wildcards supported)\n"
	"     -t, --add-forward fwds..\n"
	"                              adds a list of deliver-to forward\n"
	"     -T, --delete-forward fwds..\n"
	"                              removes a list of deliver-to forward\n"
	"                              (wildcards supported)\n"
	"     --security-password      specify a separate security fall-back password\n"
	"     --security-action        select the security action value (default 0)\n"
	"     --enable                 enable authentication for user\n"
	"     --disable                disable authentication for user\n"
        "\nCommon options for all DBMail utilities:\n"
	"     -f, --config file        specify an alternative config file\n"
	"                              Default: %s\n"
	"     -q, --quiet              quietly skip interactive prompts\n"
	"                              use twice to suppress error messages\n"
	"     -n, --no                 show the intended action but do not perform it,\n"
	"                              no to all\n"
	"     -y, --yes                perform all proposed actions, yes to all\n"
	"     -v, --verbose            verbose details\n"
	"     -V, --version            show the version\n"
	"     -h, --help               show this help message\n"
	, configFile);

	return 0;
}

int main(int argc, char *argv[])
{
	int opt = 0;
	int show_help = 0;
	int result = 0, mode = 0, mode_toomany = 0;
	char *user = NULL, *newuser = NULL, *userspec = NULL, *alias = NULL;
	char *passwd = NULL, *passwdtype = NULL, *passwdfile = NULL;
	char *password = NULL, *enctype = NULL;
	char *spasswd = NULL, *spasswd_enc = NULL;
	long int saction = 0;
	uint64_t useridnr = 0, clientid = 0, maxmail = 0;
	GList *alias_add = NULL, *alias_del = NULL, *fwds_add = NULL, *fwds_del = NULL;
	GString *tmp = NULL;
	struct change_flags change_flags;
	size_t len = 0;

	config_get_file();
	openlog(PNAME, LOG_PID, LOG_MAIL);
	setvbuf(stdout, 0, _IONBF, 0);

	/* Set all changes to false. */
	memset(&change_flags, 0, sizeof(change_flags));

	/* get options */
	//opterr = 0;		/* suppress error message from getopt() */
	static struct option long_options[] = {
		{"add-user", required_argument, NULL, 'a'},
		{"change-user", required_argument, NULL, 'c'},
		{"delete-user", required_argument, NULL, 'd'},
		{"empty-user-mailboxes", required_argument, NULL, 'e'},
		{"list-user", required_argument, NULL, 'l'},
		{"list-users", no_argument, NULL, 'L'},
		{"forward", required_argument, NULL, 'x'},

		{"password", required_argument, NULL, 'w'},
		{"pw-file", optional_argument, NULL, 'W'},
		{"pw-type", required_argument, NULL, 'p'},
		{"pw-shadow", required_argument, NULL, 'P'},
		{"user", required_argument, NULL, 'u'},
		{"client", required_argument, NULL, 'g'},
		{"max-quota", required_argument, NULL, 'm'},
		{"add-alias", required_argument, NULL, 's'},
		{"delete-alias", required_argument, NULL, 'S'},
		{"add-forward", required_argument, NULL, 't'},
		{"delete-forward", required_argument, NULL, 'T'},

		{"security-password", required_argument, 0, 0},
		{"security-action",   required_argument, 0, 0},
		{"enable",            no_argument, 0, 0},
		{"disable",           no_argument, 0, 0},

		{"config",    required_argument, NULL, 'f'},
		{"quiet",     no_argument, NULL, 'q'},
		{"no",        no_argument, NULL, 'n'},
		{"yes",       no_argument, NULL, 'y'},
		{"help",      no_argument, NULL, 'h'},
		{"verbose",   no_argument, NULL, 'v'},
		{"version",   no_argument, NULL, 'V'},
		{NULL,        0,           NULL, 0}
	};
	int option_index = 0;
	while ((opt = getopt_long(argc, argv,
				"a:d:c:e:l:Lx:" /* Major modes */
				"W::w:P::p:u:g:m:t:s:S:T:" /* Minor options */
				"i" "f:qnyvVh" /* Common options */,
				long_options, &option_index)) != -1) {
		switch (opt) {
		/* Major modes of operation
		 * (exactly one of these is required) */
		case 'a':
		case 'c':
		case 'd':
		case 'e':
			if (mode)
				mode_toomany = 1;
			mode = opt;
			if (optarg && strlen(optarg))
				user = optarg;
			break;

		case 'x':
			if (mode)
				mode_toomany = 1;
			mode = opt;
			if (optarg && strlen(optarg))
				alias = optarg;
			break;

		case 'l':
			if (mode)
				mode_toomany = 1;
			mode = opt;
			if (optarg && strlen(optarg))
				userspec = optarg;
			break;

		case 'L':
			if (mode)
				mode_toomany = 1;
			mode = opt;
			break;

		case 'i':
			printf("Interactive console is not supported in this release.\n");
			return 1;


		/* Minor options */
		case 0:
			if (MATCH(long_options[option_index].name, "security-password")) {
				change_flags.newspasswd = 1;
				spasswd = optarg;
			} else if (MATCH(long_options[option_index].name, "security-action")) {
				char *rest;
				change_flags.newsaction = 1;
				saction = strtol(optarg, &rest, 10);
				if ((rest == optarg) || (saction < 0) || (saction > 999)) {
					qerrorf("Use a security-action between 0 and 999\n");
					return 1;
				}
			} else if (MATCH(long_options[option_index].name, "enable")) {
				change_flags.enable = 1;
			} else if (MATCH(long_options[option_index].name, "disable")) {
				change_flags.disable = 1;
			}
			break;

		case 'w':
			change_flags.newpasswd = 1;
			passwd = optarg;
			break;

		case 'W':
			change_flags.newpasswd = 1;
			if (optarg && strlen(optarg)) {
				passwdfile = optarg;
				change_flags.newpasswdfile = 1;
			} else {
				change_flags.newpasswdstdin = 1;
			}
			break;

		case 'u':
			change_flags.newuser = 1;
			newuser = optarg;
			break;

		case 'p':
			if (!passwdtype)
				passwdtype = g_strdup(optarg);
			// else
				// Complain about only one type allowed.
			break;

		case 'P':
			change_flags.newpasswdshadow = 1;
			if (optarg && strlen(optarg))
				passwdfile = optarg;
			else
				passwdfile = SHADOWFILE;
			passwdtype = g_strdup("shadow");
			break;

		case 'g':
			change_flags.newclientid = 1;
			clientid = strtoull(optarg, NULL, 10);
			break;

		case 'm':
			change_flags.newmaxmail = 1;
			maxmail = strtomaxmail(optarg);
			break;

		case 's':
			// Add this item to the user's aliases.
			if (optarg && (len = strlen(optarg))) {
				tmp = g_string_new(optarg);
				alias_add = g_string_split(tmp,",");
				g_string_free(tmp, TRUE);
			}
			break;

		case 'S':
			// Delete this item from the user's aliases.
			if (optarg && (len = strlen(optarg))) {
				tmp = g_string_new(optarg);
				alias_del = g_string_split(tmp,",");
				g_string_free(tmp, TRUE);
			}
			break;

		case 't':
			// Add this item to the alias's forwards.
			if (optarg && (len = strlen(optarg))) {
				tmp = g_string_new(optarg);
				fwds_add = g_string_split(tmp,",");
				g_string_free(tmp, TRUE);
			}
			break;

		case 'T':
			// Delete this item from the alias's forwards.
			if (optarg && (len = strlen(optarg))) {
				tmp = g_string_new(optarg);
				fwds_del = g_string_split(tmp,",");
				g_string_free(tmp, TRUE);
			}
			break;

		/* Common options */
		case 'f':
			if (optarg && strlen(optarg) > 0) {
				memset(configFile, 0, sizeof(configFile));
				strncpy(configFile, optarg, sizeof(configFile)-1);
			} else {
				qprintf("dbmail-users: --config requires a filename\n\n");
				result = 1;
			}
			break;

		case 'h':
			show_help = 1;
			break;

		case 'n':
			no_to_all = 1;
			break;

		case 'y':
			yes_to_all = 1;
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
			show_help = 1;
			break;
		}

		/* If there's a non-negative return code,
		 * it's time to free memory and bail out. */
		if (result)
			goto freeall;
    }

    if (optind < argc) {
		qprintf("unrecognized option(s): ");
        while (optind < argc) {
            qprintf ("[%s] ", argv[optind++]);
        }
        qprintf ("\n");
		show_help = 1;
    }

	if (passwdtype && change_flags.newspasswd && (! passwd)) {
		printf("\nError:\nYou cannot set the security-password and encryption-type "
				"at the same time,\nwithout setting the main password as well.\n\n");
		result = -1;
		goto freeall;
	}

	if (change_flags.enable && change_flags.disable) {
		printf("\nError:\nYou cannot enable and disable an account at the same time.\n\n");
		mode_toomany = 1;
	}

	/* If nothing is happening, show the help text. */
	if (!mode || mode_toomany || show_help || (no_to_all && yes_to_all)) {
		do_showhelp();
		goto freeall;
	}

	/* read the config file */
	if (config_read(configFile) == -1) {
		printf("Failed. Unable to read config file %s\n", configFile);
		result = -1;
		goto freeall;
	}

	SetTraceLevel("USERS");
	GetDBParams();

	/* open database connection */
	if (db_connect() != 0) {
		qprintf("Failed. Could not connect to database (check log)\n");
		result = -1;
		goto freeall;
	}

	/* open authentication connection */
	if (auth_connect() != 0) {
		qprintf ("Failed. Could not connect to authentication (check log)\n");
		result = -1;
		goto freeall;
	}

	switch (mode) {
	case 'c':
	case 'd':
	case 'e':
		/* Verify the existence of this user */
		if (! auth_user_exists(user, &useridnr)) {
			qprintf("Error: user [%s] does not exist.\n", user);
			result = -1;
			goto freeall;
		}
	}

	/* Only get a password for those modes which require it. */
	switch (mode) {
	case 'a':
	case 'c':
		if (change_flags.newpasswdstdin) {
			char pw[50];
			struct termios oldattr, newattr;

			/* Get the current terminal state, then disable echo. */
			tcgetattr(fileno(stdin), &oldattr);
			newattr = oldattr;
			newattr.c_lflag &= ~ECHO;
			tcsetattr(fileno(stdin), TCSAFLUSH, &newattr);

			/* Prompt for a password and read until \n or EOF. */
			qprintf("Please enter a password (will not echo): ");
			fflush(stdout);
			if (fgets(pw, 50, stdin)) { /* ignore */ }

			/* We don't want the trailing newline. */
			len = strlen(pw);
			if (pw[len-1] == '\n')
			        pw[len-1] = '\0';
			/* fgets guarantees a nul terminated string. */
			passwd = g_strdup(pw);

			/* Restore the previous terminal state (with echo back on). */
			tcsetattr(fileno(stdin), TCSANOW, &oldattr);
			qprintf("\n");
		}

		/* If no password type was specified, and
		 * the user already exists, get their password type. */
		if (!passwdtype && useridnr)
			passwdtype = auth_getencryption(useridnr);
		/* Convert the password and password type into a
		 * fully coded format, ready for the database. */
		if (mkpassword(user, passwd, passwdtype, passwdfile, &password, &enctype)) {
			qprintf("Error: unable to create a password.\n");
			TRACE(TRACE_ERR, "Unable to create a password");
			result = -1;
			goto freeall;
		}
		if (spasswd) {
			if (mkpassword(user, spasswd, passwdtype, passwdfile, &spasswd_enc, &enctype)) {
				qprintf("Error: unable to create a security password.\n");
				TRACE(TRACE_ERR, "Unable to create a security password");
				result = -1;
				goto freeall;
			}
		}
	}

	switch (mode) {
	case 'a':
		qprintf("Adding user [%s]...\n", user);
		TRACE(TRACE_INFO, "Adding user [%s]", user);
		result = do_add(user, password, enctype, maxmail, clientid,
				alias_add, alias_del);
		break;
	case 'd':
		qprintf("Deleting user [%s]...\n", user);
		TRACE(TRACE_INFO, "Deleting user [%s]", user);
		result = do_delete(useridnr, user);
		break;
	case 'c':
		qprintf("Changing user [%s]...\n", user);
		TRACE(TRACE_INFO, "Changing user [%s]...", user);
		if (change_flags.newuser) {
			result |= do_username(useridnr, newuser);
		}
		if (change_flags.newpasswd) {
			result |= do_password(useridnr, password, enctype);
		}
		if (change_flags.newclientid) {
			result |= do_clientid(useridnr, clientid);
		}
		if (change_flags.newmaxmail) {
			result |= do_maxmail(useridnr, maxmail);
		}
		if (change_flags.newspasswd) {
			result |= do_spasswd(useridnr, spasswd_enc);
		}
		if (change_flags.newsaction) {
			result |= do_saction(useridnr, saction);
		}
		if (change_flags.enable) {
			result |= do_enable(useridnr, true);
		}
		if (change_flags.disable) {
			result |= do_enable(useridnr, false);
		}
		result |= do_aliases(useridnr, alias_add, alias_del);
		break;
	case 'e':
		qprintf("Empty all mailboxes for user [%s]...\n", user);
		TRACE(TRACE_INFO, "Empty all mailboxes for user [%s]...", user);
		result = do_empty(useridnr);
		break;
	case 'l':
		qprintf("Listing information for user [%s]...\n", userspec);
		TRACE(TRACE_INFO, "Listing information for user [%s]...", userspec);
		result = do_show(userspec);
		break;
	case 'L':
		qprintf("Listing information for all users\n");
		TRACE(TRACE_INFO, "Listing information for all users");
		result = do_show(NULL);
		break;
	case 'x':
		if (g_list_length(fwds_add) == 0 && g_list_length(fwds_del) == 0) {
			qprintf("Please add at least one of --forwards-add or --forwards-delete\n");
		} else {
			qprintf("Creating an external forwarding address for [%s]...\n", alias);
			TRACE(TRACE_INFO, "Creating an external forwarding address for [%s]...", alias);
			result = do_forwards(alias, clientid, fwds_add, fwds_del);
		}
		break;
	default:
		result = 1;
	}

	/* Here's where we free memory and quit.
	 * Be sure that all of these are NULL safe! */
freeall:

	/* Free the lists. */
	if (alias_del) g_list_destroy(alias_del);
	if (alias_add) g_list_destroy(alias_add);
	if (fwds_del) g_list_destroy(fwds_del);
	if (fwds_add) g_list_destroy(fwds_add);
	if (passwdtype) g_free(passwdtype);
	if (password) g_free(password);
	if (spasswd_enc) g_free(spasswd_enc);

	db_disconnect();
	auth_disconnect();
	config_free();

	if (result) {
		qprintf("Command failed, see logfile for details.\n");
	}
	return result;
}
