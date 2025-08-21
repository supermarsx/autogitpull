#ifndef WEBHOOK_NOTIFIER_HPP
#define WEBHOOK_NOTIFIER_HPP

#include <optional>
#include <string>

#include "repo.hpp"

class WebhookNotifier {
  public:
    explicit WebhookNotifier(std::string url, std::optional<std::string> secret = std::nullopt);
    void notify(const RepoInfo& info) const;
    const std::string& url() const { return url_; }

  private:
    std::string url_;
    std::optional<std::string> secret_;
};

#endif // WEBHOOK_NOTIFIER_HPP
