#ifndef UI_LOOP_HPP
#define UI_LOOP_HPP

#include <chrono>
#include "options.hpp"

/** Print the command line help text. */
void print_help(const char* prog);

int run_event_loop(Options opts);

bool poll_timed_out(const Options& opts, std::chrono::steady_clock::time_point start,
                    std::chrono::steady_clock::time_point now);

extern bool debugMemory;
extern bool dumpState;
extern size_t dumpThreshold;

#endif // UI_LOOP_HPP
