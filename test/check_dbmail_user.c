/*
 *   Copyright (c) 2005-2013 NFG Net Facilities Group BV support@nfg.nl
 *   Copyright (c) 2014-2019 Paul J Stevens, The Netherlands, support@nfg.nl
 *   Copyright (c) 2020-2026 Alan Hicks, Persistent Objects Ltd support@p-o.co.uk
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License
 *   as published by the Free Software Foundation; either
 *   version 2 of the License, or (at your option) any later
 *   version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *
 *   
 *
 *
 *  
 *
 *   Basic unit-test framework for dbmail (www.dbmail.org)
 *
 *   See http://check.sf.net for details and docs.
 *
 *
 *   Run 'make check' to see some action.
 *
 */ 

#include <check.h>
#include "check_dbmail.h"

extern char configFile[PATH_MAX];
extern int quiet;
extern int reallyquiet;

/*
 *
 * the test fixtures
 *
 */
void setup(void)
{
	reallyquiet = 1;
	config_get_file();
	config_read(configFile);
	configure_debug(NULL,255,0);
	GetDBParams();
	db_connect();
	auth_connect();
	do_add("testfailuser","testpass","md5-hash",0,0,NULL,NULL);
}

void teardown(void)
{
	db_disconnect();
}

//int do_add(const char * const user,
//           const char * const password,
//           const char * const enctype,
//           const uint64_t maxmail, const uint64_t clientid,
//	   GList * alias_add,
//	   GList * alias_del);

START_TEST(test_do_add)
{
	int result;
	result = do_add("testfailuser","testpass","md5-hash",0,0,NULL,NULL);
	fail_unless(result!=0,"do_add succeeded when it should have failed");
	result = do_add("testadduser-md5-hash","testpass","md5-hash",0,0,NULL,NULL);
	fail_unless(result==0,"do_add failed");
	result = do_add("testadduser-md5-digest","testpass","md5-digest",0,0,NULL,NULL);
	fail_unless(result==0,"do_add failed");
	result = do_add("testadduser-crypt-raw","testpass","crypt-raw",0,0,NULL,NULL);
	fail_unless(result==0,"do_add failed");
	result = do_add("testadduser-md5-hash-raw","testpass","md5-hash-raw",0,0,NULL,NULL);
	fail_unless(result==0,"do_add failed");
	result = do_add("testadduser-md5-digest-raw","testpass","md5-digest-raw",0,0,NULL,NULL);
	fail_unless(result==0,"do_add failed");
	result = do_add("testadduser-md5-base64","testpass","md5-base64",0,0,NULL,NULL);
	fail_unless(result==0,"do_add failed");
	result = do_add("testadduser-md5-base64-raw","testpass","md5-base64-raw",0,0,NULL,NULL);
	fail_unless(result==0,"do_add failed");
	result = do_add("testadduser-whirlpool","testpass","whirlpool",0,0,NULL,NULL);
	fail_unless(result==0,"do_add failed");
	result = do_add("testadduser-sha512","testpass","sha512",0,0,NULL,NULL);
	fail_unless(result==0,"do_add failed");
	result = do_add("testadduser-sha256","testpass","sha256",0,0,NULL,NULL);
	fail_unless(result==0,"do_add failed");
	result = do_add("testadduser-sha1","testpass","sha1",0,0,NULL,NULL);
	fail_unless(result==0,"do_add failed");
	result = do_add("testadduser-tiger","testpass","tiger",0,0,NULL,NULL);
	fail_unless(result==0,"do_add failed");
}
END_TEST


//int do_show(const char * const user);
START_TEST(test_do_show)
{
	fail_unless(do_show("testfailuser")==0,"test_do_show failed");
	fail_unless(do_show("nosuchuser"),"do_show should have failed");
}
END_TEST

//int do_empty(const uint64_t useridnr);
START_TEST(test_do_empty)
{
	uint64_t user_idnr;
	auth_user_exists("nosuchuser",&user_idnr);
	//fail_unless(do_empty(user_idnr),"do_empty should have failed");
}
END_TEST

//int do_delete(const uint64_t useridnr, const char * const user);
START_TEST(test_do_delete)
{
	int result;
	uint64_t user_idnr;
	auth_user_exists("testadduser-md5-hash", &user_idnr);
	ck_assert_int_gt(user_idnr, 0);
	result = do_delete(user_idnr, "testadduser-md5-hash");
	ck_assert_int_eq(result, 0);

	auth_user_exists("testadduser-md5-digest", &user_idnr);
	ck_assert_int_gt(user_idnr, 0);
	result = do_delete(user_idnr, "testadduser-md5-digest");
	ck_assert_int_eq(result, 0);

	auth_user_exists("testadduser-crypt-raw", &user_idnr);
	ck_assert_int_gt(user_idnr, 0);
	result = do_delete(user_idnr, "testadduser-crypt-raw");
	ck_assert_int_eq(result, 0);

	auth_user_exists("testadduser-md5-hash-raw", &user_idnr);
	ck_assert_int_gt(user_idnr, 0);
	result = do_delete(user_idnr, "testadduser-md5-hash-raw");
	ck_assert_int_eq(result, 0);

	auth_user_exists("testadduser-md5-digest-raw", &user_idnr);
	ck_assert_int_gt(user_idnr, 0);
	result = do_delete(user_idnr, "testadduser-md5-digest-raw");
	ck_assert_int_eq(result, 0);

	auth_user_exists("testadduser-md5-base64", &user_idnr);
	ck_assert_int_gt(user_idnr, 0);
	result = do_delete(user_idnr, "testadduser-md5-base64");
	ck_assert_int_eq(result, 0);

	auth_user_exists("testadduser-md5-base64-raw", &user_idnr);
	ck_assert_int_gt(user_idnr, 0);
	result = do_delete(user_idnr, "testadduser-md5-base64-raw");
	ck_assert_int_eq(result, 0);

	auth_user_exists("testadduser-whirlpool", &user_idnr);
	ck_assert_int_gt(user_idnr, 0);
	result = do_delete(user_idnr, "testadduser-whirlpool");
	ck_assert_int_eq(result, 0);

	auth_user_exists("testadduser-sha512", &user_idnr);
	ck_assert_int_gt(user_idnr, 0);
	result = do_delete(user_idnr, "testadduser-sha512");
	ck_assert_int_eq(result, 0);

	auth_user_exists("testadduser-sha256", &user_idnr);
	ck_assert_int_gt(user_idnr, 0);
	result = do_delete(user_idnr, "testadduser-sha256");
	ck_assert_int_eq(result, 0);

	auth_user_exists("testadduser-sha1", &user_idnr);
	ck_assert_int_gt(user_idnr, 0);
	result = do_delete(user_idnr, "testadduser-sha1");
	ck_assert_int_eq(result, 0);

	auth_user_exists("testadduser-tiger", &user_idnr);
	ck_assert_int_gt(user_idnr, 0);
	result = do_delete(user_idnr, "testadduser-tiger");
	ck_assert_int_eq(result, 0);
}
END_TEST

START_TEST(test_dm_match)
{
	int i;
	char *candidate = "MyName is SAMMY @ and I am a SLUG!";
	char *badpatterns[] = {
		"hello", "*hello", "*hello*", "hello*",
		"*hello", "*hello*", "hello*", "?", NULL };
	char *goodpatterns[] = {
		"*", "*and*", "*SLUG!", "My*", "????My*",
		"MyName ?? *", "??Name*", "*SLUG?", NULL };

	for (i = 0; badpatterns[i] != NULL; i++) {
		fail_unless(match_glob(badpatterns[i], candidate)
			== NULL, "test_dm_match failed on a bad pattern:"
			" [%s]", badpatterns[i]);
	}

	for (i = 0; goodpatterns[i] != NULL; i++) {
		fail_unless(match_glob(goodpatterns[i], candidate)
			== candidate, "test_dm_match failed on a good pattern:"
			" [%s]", goodpatterns[i]);
	}

}
END_TEST

START_TEST(test_dm_match_list)
{
}
END_TEST

/* Change operations */
//int do_username(const uint64_t useridnr, const char *newuser);
//int do_maxmail(const uint64_t useridnr, const uint64_t maxmail);
//int do_clientid(const uint64_t useridnr, const uint64_t clientid);
//int do_password(const uint64_t useridnr,
//                const char * const password,
//                const char * const enctype);
//int do_aliases(const uint64_t useridnr,
//               GList * alias_add,
//               GList * alias_del);
/* External forwards */
//int do_forwards(const char *alias, const uint64_t clientid,
//                GList * fwds_add,
//                GList * fwds_del);

/* Helper functions */
//uint64_t strtomaxmail(const char * const str);


Suite *dbmail_common_suite(void)
{
	Suite *s = suite_create("Dbmail User");
	TCase *tc_user = tcase_create("User");
	
	suite_add_tcase(s, tc_user);
	
	tcase_add_checked_fixture(tc_user, setup, teardown);
	tcase_add_test(tc_user, test_do_add);
	tcase_add_test(tc_user, test_do_show);
	tcase_add_test(tc_user, test_do_empty);
	tcase_add_test(tc_user, test_do_delete);
	tcase_add_test(tc_user, test_dm_match);
	tcase_add_test(tc_user, test_dm_match_list);
	
	return s;
}

int main(void)
{
	int nf;
	Suite *s = dbmail_common_suite();
	SRunner *sr = srunner_create(s);
	srunner_run_all(sr, CK_NORMAL);
	nf = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (nf == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
	

