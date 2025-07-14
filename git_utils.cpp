#include "git_utils.hpp"
#include <git2.h>

using namespace std;

namespace git {

bool is_git_repo(const fs::path& p) {
    return fs::exists(p / ".git") && fs::is_directory(p / ".git");
}

static string oid_to_hex(const git_oid& oid) {
    char buf[GIT_OID_HEXSZ + 1];
    git_oid_tostr(buf, sizeof(buf), &oid);
    return string(buf);
}

string get_local_hash(const fs::path& repo) {
    git_libgit2_init();
    git_repository* r = nullptr;
    if (git_repository_open(&r, repo.string().c_str()) != 0) {
        git_libgit2_shutdown();
        return "";
    }
    git_oid oid;
    if (git_reference_name_to_id(&oid, r, "HEAD") != 0) {
        git_repository_free(r);
        git_libgit2_shutdown();
        return "";
    }
    string hash = oid_to_hex(oid);
    git_repository_free(r);
    git_libgit2_shutdown();
    return hash;
}

string get_current_branch(const fs::path& repo) {
    git_libgit2_init();
    git_repository* r = nullptr;
    if (git_repository_open(&r, repo.string().c_str()) != 0) {
        git_libgit2_shutdown();
        return "";
    }
    git_reference* head = nullptr;
    if (git_repository_head(&head, r) != 0) {
        git_repository_free(r);
        git_libgit2_shutdown();
        return "";
    }
    const char* name = git_reference_shorthand(head);
    string branch = name ? name : "";
    git_reference_free(head);
    git_repository_free(r);
    git_libgit2_shutdown();
    return branch;
}

string get_remote_hash(const fs::path& repo, const string& branch) {
    git_libgit2_init();
    git_repository* r = nullptr;
    if (git_repository_open(&r, repo.string().c_str()) != 0) {
        git_libgit2_shutdown();
        return "";
    }
    git_remote* remote = nullptr;
    if (git_remote_lookup(&remote, r, "origin") != 0) {
        git_repository_free(r);
        git_libgit2_shutdown();
        return "";
    }
    git_fetch_options fetch_opts = GIT_FETCH_OPTIONS_INIT;
    git_remote_fetch(remote, nullptr, &fetch_opts, nullptr);
    git_remote_free(remote);
    git_oid oid;
    string refname = string("refs/remotes/origin/") + branch;
    if (git_reference_name_to_id(&oid, r, refname.c_str()) != 0) {
        git_repository_free(r);
        git_libgit2_shutdown();
        return "";
    }
    string hash = oid_to_hex(oid);
    git_repository_free(r);
    git_libgit2_shutdown();
    return hash;
}

string get_origin_url(const fs::path& repo) {
    git_libgit2_init();
    git_repository* r = nullptr;
    if (git_repository_open(&r, repo.string().c_str()) != 0) {
        git_libgit2_shutdown();
        return "";
    }
    git_remote* remote = nullptr;
    if (git_remote_lookup(&remote, r, "origin") != 0) {
        git_repository_free(r);
        git_libgit2_shutdown();
        return "";
    }
    const char* url = git_remote_url(remote);
    string result = url ? url : "";
    git_remote_free(remote);
    git_repository_free(r);
    git_libgit2_shutdown();
    return result;
}

bool is_github_url(const string& url) {
    return url.find("github.com") != string::npos;
}

bool remote_accessible(const fs::path& repo) {
    git_libgit2_init();
    git_repository* r = nullptr;
    if (git_repository_open(&r, repo.string().c_str()) != 0) {
        git_libgit2_shutdown();
        return false;
    }
    git_remote* remote = nullptr;
    if (git_remote_lookup(&remote, r, "origin") != 0) {
        git_repository_free(r);
        git_libgit2_shutdown();
        return false;
    }
    int err = git_remote_connect(remote, GIT_DIRECTION_FETCH, nullptr, nullptr, nullptr);
    bool ok = err == 0;
    if (ok)
        git_remote_disconnect(remote);
    git_remote_free(remote);
    git_repository_free(r);
    git_libgit2_shutdown();
    return ok;
}

int try_pull(const fs::path& repo, string& out_pull_log) {
    git_libgit2_init();
    git_repository* r = nullptr;
    if (git_repository_open(&r, repo.string().c_str()) != 0) {
        git_libgit2_shutdown();
        out_pull_log = "Failed to open repository";
        return 2;
    }
    string branch = get_current_branch(repo);
    git_remote* remote = nullptr;
    if (git_remote_lookup(&remote, r, "origin") != 0) {
        git_repository_free(r);
        git_libgit2_shutdown();
        out_pull_log = "No origin remote";
        return 2;
    }
    git_fetch_options fetch_opts = GIT_FETCH_OPTIONS_INIT;
    if (git_remote_fetch(remote, nullptr, &fetch_opts, nullptr) != 0) {
        git_remote_free(remote);
        git_repository_free(r);
        git_libgit2_shutdown();
        out_pull_log = "Fetch failed";
        return 2;
    }
    git_oid remote_oid;
    string refname = string("refs/remotes/origin/") + branch;
    if (git_reference_name_to_id(&remote_oid, r, refname.c_str()) != 0) {
        git_remote_free(remote);
        git_repository_free(r);
        git_libgit2_shutdown();
        out_pull_log = "Remote branch not found";
        return 2;
    }
    git_oid local_oid;
    if (git_reference_name_to_id(&local_oid, r, "HEAD") != 0) {
        git_remote_free(remote);
        git_repository_free(r);
        git_libgit2_shutdown();
        out_pull_log = "Local HEAD not found";
        return 2;
    }
    if (git_oid_cmp(&local_oid, &remote_oid) == 0) {
        out_pull_log = "Already up to date";
        git_remote_free(remote);
        git_repository_free(r);
        git_libgit2_shutdown();
        return 0;
    }
    git_object* target = nullptr;
    if (git_object_lookup(&target, r, &remote_oid, GIT_OBJECT_COMMIT) != 0) {
        git_remote_free(remote);
        git_repository_free(r);
        git_libgit2_shutdown();
        out_pull_log = "Lookup failed";
        return 2;
    }
    if (git_reset(r, target, GIT_RESET_HARD, nullptr) != 0) {
        git_object_free(target);
        git_remote_free(remote);
        git_repository_free(r);
        git_libgit2_shutdown();
        out_pull_log = "Reset failed";
        return 2;
    }
    git_object_free(target);
    git_remote_free(remote);
    git_repository_free(r);
    git_libgit2_shutdown();
    out_pull_log = "Fast-forwarded";
    return 0;
}

} // namespace git
