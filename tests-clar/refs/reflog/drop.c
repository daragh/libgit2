#include "clar_libgit2.h"

#include "reflog.h"

static git_repository *g_repo;
static git_reflog *g_reflog;
static unsigned int entrycount;

void test_refs_reflog_drop__initialize(void)
{
	git_reference *ref;
	
	g_repo = cl_git_sandbox_init("testrepo.git");
	cl_git_pass(git_reference_lookup(&ref, g_repo, "HEAD"));
	
	git_reflog_read(&g_reflog, ref);
	entrycount = git_reflog_entrycount(g_reflog);

	git_reference_free(ref);
}

void test_refs_reflog_drop__cleanup(void)
{
	git_reflog_free(g_reflog);
	g_reflog = NULL;

	cl_git_sandbox_cleanup();
}

void test_refs_reflog_drop__dropping_a_non_exisiting_entry_from_the_log_returns_ENOTFOUND(void)
{
	cl_assert_equal_i(GIT_ENOTFOUND, git_reflog_drop(g_reflog, entrycount, 0));

	cl_assert_equal_i(entrycount, git_reflog_entrycount(g_reflog));
}

void test_refs_reflog_drop__can_drop_an_entry(void)
{
	cl_assert(entrycount > 4);

	cl_git_pass(git_reflog_drop(g_reflog, 2, 0));
	cl_assert_equal_i(entrycount - 1, git_reflog_entrycount(g_reflog));
}

void test_refs_reflog_drop__can_drop_an_entry_and_rewrite_the_log_history(void)
{
	const git_reflog_entry *before_current;
	const git_reflog_entry *after_current;
	git_oid before_current_old_oid, before_current_cur_oid;

	cl_assert(entrycount > 4);

	before_current = git_reflog_entry_byindex(g_reflog, 1);

	git_oid_cpy(&before_current_old_oid, &before_current->oid_old);
	git_oid_cpy(&before_current_cur_oid, &before_current->oid_cur);

	cl_git_pass(git_reflog_drop(g_reflog, 1, 1));

	cl_assert_equal_i(entrycount - 1, git_reflog_entrycount(g_reflog));

	after_current = git_reflog_entry_byindex(g_reflog, 0);

	cl_assert_equal_i(0, git_oid_cmp(&before_current_old_oid, &after_current->oid_old));
	cl_assert(0 != git_oid_cmp(&before_current_cur_oid, &after_current->oid_cur));
}

void test_refs_reflog_drop__can_drop_the_oldest_entry(void)
{
	const git_reflog_entry *entry;

	cl_assert(entrycount > 2);

	cl_git_pass(git_reflog_drop(g_reflog, entrycount - 1, 0));
	cl_assert_equal_i(entrycount - 1, git_reflog_entrycount(g_reflog));

	entry = git_reflog_entry_byindex(g_reflog, entrycount - 2);
	cl_assert(git_oid_streq(&entry->oid_old, GIT_OID_HEX_ZERO) != 0);
}

void test_refs_reflog_drop__can_drop_the_oldest_entry_and_rewrite_the_log_history(void)
{
	const git_reflog_entry *entry;

	cl_assert(entrycount > 2);

	cl_git_pass(git_reflog_drop(g_reflog, entrycount - 1, 1));
	cl_assert_equal_i(entrycount - 1, git_reflog_entrycount(g_reflog));

	entry = git_reflog_entry_byindex(g_reflog, entrycount - 2);
	cl_assert(git_oid_streq(&entry->oid_old, GIT_OID_HEX_ZERO) == 0);
}

void test_refs_reflog_drop__can_drop_all_the_entries(void)
{
	cl_assert(--entrycount > 0);

	do 	{
		cl_git_pass(git_reflog_drop(g_reflog, 0, 1));
	} while (--entrycount > 0);

	cl_git_pass(git_reflog_drop(g_reflog, 0, 1));

	cl_assert_equal_i(0, git_reflog_entrycount(g_reflog));
}

void test_refs_reflog_drop__can_persist_deletion_on_disk(void)
{
	git_reference *ref;

	cl_assert(entrycount > 2);

	cl_git_pass(git_reference_lookup(&ref, g_repo, g_reflog->ref_name));
	cl_git_pass(git_reflog_drop(g_reflog, 0, 1));
	cl_assert_equal_i(entrycount - 1, git_reflog_entrycount(g_reflog));
	cl_git_pass(git_reflog_write(g_reflog));

	git_reflog_free(g_reflog);

	git_reflog_read(&g_reflog, ref);
	git_reference_free(ref);

	cl_assert_equal_i(entrycount - 1, git_reflog_entrycount(g_reflog));
}
