#ifndef PARSE_UTILS_HPP
#define PARSE_UTILS_HPP

#include <cstddef>
#include <string>
#include <chrono>
#include "arg_parser.hpp"

// Parse an integer flag from the parser.
// Format: decimal with optional '+' or '-'.
// Bounds: inclusive [min, max].
// Invalid input: missing flag, non-integer, or out-of-range sets ok=false and returns 0.
int parse_int(const ArgParser& parser, const std::string& flag, int min, int max, bool& ok);

// Parse an integer from a string.
// Format: decimal with optional '+' or '-' (std::stoi rules; trailing characters are ignored).
// Bounds: inclusive [min, max].
// Invalid input: conversion failure or out-of-range sets ok=false and returns 0.
int parse_int(const std::string& value, int min, int max, bool& ok);

// Parse a floating-point flag from the parser.
// Format: decimal with at most one digit after the decimal point.
// Bounds: inclusive [min, max].
// Invalid input: missing flag, parse failure, or out-of-range sets ok=false and returns 0.0.
double parse_double(const ArgParser& parser, const std::string& flag, double min, double max,
                    bool& ok);

// Parse a floating-point number from a string.
// Format: decimal with at most one fractional digit; no extra characters allowed.
// Bounds: inclusive [min, max].
// Invalid input: conversion failure or out-of-range sets ok=false and returns 0.0.
double parse_double(const std::string& value, double min, double max, bool& ok);

// Parse an unsigned integer flag from the parser.
// Format: decimal digits only.
// Bounds: inclusive [min, max].
// Invalid input: missing flag, non-numeric, or out-of-range sets ok=false and returns 0.
unsigned int parse_uint(const ArgParser& parser, const std::string& flag, unsigned int min,
                        unsigned int max, bool& ok);

// Parse an unsigned integer from a string.
// Format: decimal digits only.
// Bounds: inclusive [min, max].
// Invalid input: conversion failure or out-of-range sets ok=false and returns 0.
unsigned int parse_uint(const std::string& value, unsigned int min, unsigned int max, bool& ok);

// Parse a size_t flag from the parser.
// Format: decimal digits only.
// Bounds: inclusive [min, max].
// Invalid input: missing flag, non-numeric, or out-of-range sets ok=false and returns 0.
size_t parse_size_t(const ArgParser& parser, const std::string& flag, size_t min, size_t max,
                    bool& ok);

// Parse a size_t from a string.
// Format: decimal digits only.
// Bounds: inclusive [min, max].
// Invalid input: conversion failure or out-of-range sets ok=false and returns 0.
size_t parse_size_t(const std::string& value, size_t min, size_t max, bool& ok);

// Parse byte size from parser with optional unit suffix.
// Format: unsigned integer followed by optional units B, KB, MB, GB, TB, or PB (case-insensitive).
// Bounds: inclusive [min, max] bytes.
// Invalid input: missing flag, bad unit, parse failure, or out-of-range sets ok=false and returns
// 0.
size_t parse_bytes(const ArgParser& parser, const std::string& flag, size_t min, size_t max,
                   bool& ok);

// Parse byte size from a string with optional unit suffix.
// Format: unsigned integer followed by optional units B, KB, MB, GB, TB, or PB (case-insensitive).
// Bounds: inclusive [min, max] bytes.
// Invalid input: bad unit, parse failure, or out-of-range sets ok=false and returns 0.
size_t parse_bytes(const std::string& value, size_t min, size_t max, bool& ok);

// Parse byte size with optional unit suffix and no explicit bounds.
// Format: unsigned integer followed by optional units B, KB, MB, GB, TB, or PB (case-insensitive).
// Bounds: [0, SIZE_MAX].
// Invalid input: missing flag, bad unit, or parse failure sets ok=false and returns 0.
size_t parse_bytes(const ArgParser& parser, const std::string& flag, bool& ok);

// Parse byte size from a string with optional unit suffix and no explicit bounds.
// Format: unsigned integer followed by optional units B, KB, MB, GB, TB, or PB (case-insensitive).
// Bounds: [0, SIZE_MAX].
// Invalid input: bad unit or parse failure sets ok=false and returns 0.
size_t parse_bytes(const std::string& value, bool& ok);

// Parse an unsigned long long flag from the parser.
// Format: integer literal in base 10, 8, or 16 (0x/0 prefix supported).
// Bounds: inclusive [min, max].
// Invalid input: missing flag, conversion failure, or out-of-range sets ok=false and returns 0.
unsigned long long parse_ull(const ArgParser& parser, const std::string& flag,
                             unsigned long long min, unsigned long long max, bool& ok);

// Parse an unsigned long long from a string.
// Format: integer literal in base 10, 8, or 16 (0x/0 prefix supported).
// Bounds: inclusive [min, max].
// Invalid input: conversion failure or out-of-range sets ok=false and returns 0.
unsigned long long parse_ull(const std::string& value, unsigned long long min,
                             unsigned long long max, bool& ok);

// Parse a duration string like "30m" or "2h".
// Format: non-negative integer followed by s, m, h, d, w, M (~30 days), or Y (~365 days).
// Bounds: no upper bound beyond integer range; negative values are invalid.
// Invalid input: missing flag or parse failure sets ok=false and returns 0s.
std::chrono::seconds parse_duration(const ArgParser& parser, const std::string& flag, bool& ok);

// Parse a duration from a string.
// Format: non-negative integer followed by s, m, h, d, w, M (~30 days), or Y (~365 days).
// Bounds: no upper bound beyond integer range; negative values are invalid.
// Invalid input: parse failure sets ok=false and returns 0s.
std::chrono::seconds parse_duration(const std::string& value, bool& ok);

// Parse milliseconds with optional unit suffix.
// Format: non-negative integer optionally suffixed by ms (default), s, or m.
// Bounds: no upper bound beyond integer range; negative values are invalid.
// Invalid input: missing flag or parse failure sets ok=false and returns 0ms.
std::chrono::milliseconds parse_time_ms(const ArgParser& parser, const std::string& flag, bool& ok);

// Parse milliseconds from a string with optional unit suffix.
// Format: non-negative integer optionally suffixed by ms (default), s, or m.
// Bounds: no upper bound beyond integer range; negative values are invalid.
// Invalid input: parse failure sets ok=false and returns 0ms.
std::chrono::milliseconds parse_time_ms(const std::string& value, bool& ok);

#endif // PARSE_UTILS_HPP
