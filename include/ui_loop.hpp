#ifndef UI_LOOP_HPP
#define UI_LOOP_HPP

#include "options.hpp"

/** Print the command line help text. */
void print_help(const char* prog);

int run_event_loop(const Options& opts);

extern bool debugMemory;
extern bool dumpState;
extern size_t dumpThreshold;

#endif // UI_LOOP_HPP
