#ifndef MUTANT_MODE_HPP
#define MUTANT_MODE_HPP

#include <filesystem>
#include <chrono>

#include "options.hpp"
#include "repo.hpp"

// Load configuration for mutant mode, apply adaptive settings,
// and persist any updated parameters.
void apply_mutant_mode(Options& opts);

// Verify whether a repository has recent updates and hasn't been processed
// already in this mutant session.
bool mutant_should_pull(const std::filesystem::path& repo, RepoInfo& ri, const std::string& remote,
                        bool include_private, std::chrono::seconds updated_since);

// Record the result of a pull operation and adjust timeouts accordingly.
void mutant_record_result(const std::filesystem::path& repo, RepoStatus status,
                          std::chrono::seconds duration);

#endif // MUTANT_MODE_HPP
