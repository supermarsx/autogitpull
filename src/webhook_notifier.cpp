#include "webhook_notifier.hpp"

#include <curl/curl.h>
#include <nlohmann/json.hpp>

namespace {
std::string status_to_string(RepoStatus status) {
    switch (status) {
    case RS_PENDING:
        return "Pending";
    case RS_CHECKING:
        return "Checking";
    case RS_UP_TO_DATE:
        return "UpToDate";
    case RS_PULLING:
        return "Pulling";
    case RS_PULL_OK:
        return "Pulled";
    case RS_PKGLOCK_FIXED:
        return "PkgLockOk";
    case RS_ERROR:
        return "Error";
    case RS_SKIPPED:
        return "Skipped";
    case RS_NOT_GIT:
        return "NotGit";
    case RS_HEAD_PROBLEM:
        return "HEAD/BR";
    case RS_DIRTY:
        return "Dirty";
    case RS_TIMEOUT:
        return "TimedOut";
    case RS_RATE_LIMIT:
        return "RateLimit";
    case RS_REMOTE_AHEAD:
        return "RemoteUp";
    case RS_TEMPFAIL:
        return "TempFail";
    }
    return "";
}
} // namespace

WebhookNotifier::WebhookNotifier(std::string url, std::optional<std::string> secret)
    : url_(std::move(url)), secret_(std::move(secret)) {}

void WebhookNotifier::notify(const RepoInfo& info) const {
    if (url_.empty())
        return;
    CURL* curl = curl_easy_init();
    if (!curl)
        return;
    nlohmann::json j{{"repository", info.path.string()},
                     {"status", status_to_string(info.status)},
                     {"message", info.message},
                     {"pulled", info.pulled},
                     {"commit", info.commit}};
    std::string payload = j.dump();
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    if (secret_) {
        std::string hdr = "X-Webhook-Secret: " + *secret_;
        headers = curl_slist_append(headers, hdr.c_str());
    }
    curl_easy_setopt(curl, CURLOPT_URL, url_.c_str());
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, payload.size());
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
    curl_easy_perform(curl);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
}
