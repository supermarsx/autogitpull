#ifndef PARSE_UTILS_HPP
#define PARSE_UTILS_HPP

#include <cstddef>
#include <string>
#include "arg_parser.hpp"

int parse_int(const ArgParser& parser, const std::string& flag, int min, int max, bool& ok);
int parse_int(const std::string& value, int min, int max, bool& ok);
unsigned int parse_uint(const ArgParser& parser, const std::string& flag, unsigned int min,
                        unsigned int max, bool& ok);
unsigned int parse_uint(const std::string& value, unsigned int min, unsigned int max, bool& ok);
size_t parse_size_t(const ArgParser& parser, const std::string& flag, size_t min, size_t max,
                    bool& ok);
size_t parse_size_t(const std::string& value, size_t min, size_t max, bool& ok);
unsigned long long parse_ull(const ArgParser& parser, const std::string& flag,
                             unsigned long long min, unsigned long long max, bool& ok);
unsigned long long parse_ull(const std::string& value, unsigned long long min,
                             unsigned long long max, bool& ok);

#endif // PARSE_UTILS_HPP
