#ifndef PROCESS_MONITOR_HPP
#define PROCESS_MONITOR_HPP
#include <functional>
#include "options.hpp"

int run_with_monitor(const Options& opts);
void set_monitor_worker(int (*func)(Options));

#endif // PROCESS_MONITOR_HPP
