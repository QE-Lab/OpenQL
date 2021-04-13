/** \file
 * Provides utilities for working with strings that the STL fails to
 * satisfactorily provide.
 */

#include "ql/utils/str.h"

#include <algorithm>
#include <cctype>
#include "ql/utils/exception.h"
#include "ql/utils/list.h"

namespace ql {
namespace utils {

/**
 * Parses the given string as an unsigned integer. Throws an exception if the
 * conversion fails.
 */
UInt parse_uint(const Str &str) {
    try {
        return std::stoull(str);
    } catch (std::invalid_argument &e) {
        (void)e;
        throw Exception("failed to parse \"" + str + "\" as an unsigned integer");
    } catch (std::range_error &e) {
        (void)e;
        throw Exception("failed to parse \"" + str + "\" as an unsigned integer (out of range)");
    }
}

/**
 * Parses the given string as a signed integer. Throws an exception if the
 * conversion fails.
 */
Int parse_int(const Str &str) {
    try {
        return std::stoll(str);
    } catch (std::invalid_argument &e) {
        (void)e;
        throw Exception("failed to parse \"" + str + "\" as a signed integer");
    } catch (std::range_error &e) {
        (void)e;
        throw Exception("failed to parse \"" + str + "\" as a signed integer (out of range)");
    }
}

/**
 * Parses the given string as a real number. Throws an exception if the
 * conversion fails.
 */
Real parse_real(const Str &str) {
    try {
        return std::stod(str);
    } catch (std::invalid_argument &e) {
        (void)e;
        throw Exception("failed to parse \"" + str + "\" as a real number");
    } catch (std::range_error &e) {
        (void)e;
        throw Exception("failed to parse \"" + str + "\" as a real number (out of range)");
    }
}

/**
 * Parses the given string as an unsigned integer. Returns the given default
 * value if the conversion fails.
 */
UInt parse_uint(const Str &str, UInt dflt, bool *success) {
    try {
        auto x = std::stoull(str);
        if (success) *success = true;
        return x;
    } catch (std::invalid_argument &e) {
        (void)e;
        if (success) *success = false;
        return dflt;
    } catch (std::range_error &e) {
        (void)e;
        if (success) *success = false;
        return dflt;
    }
}

/**
 * Parses the given string as a signed integer. Returns the given default value
 * if the conversion fails.
 */
Int parse_int(const Str &str, Int dflt, bool *success) {
    try {
        auto x = std::stoll(str);
        if (success) *success = true;
        return x;
    } catch (std::invalid_argument &e) {
        (void)e;
        if (success) *success = false;
        return dflt;
    } catch (std::range_error &e) {
        (void)e;
        if (success) *success = false;
        return dflt;
    }
}

/**
 * Parses the given string as a real number. Returns the given default value if
 * the conversion fails.
 */
Real parse_real(const Str &str, Real dflt, bool *success) {
    try {
        auto x = std::stod(str);
        if (success) *success = true;
        return x;
    } catch (std::invalid_argument &e) {
        (void)e;
        if (success) *success = false;
        return dflt;
    } catch (std::range_error &e) {
        (void)e;
        if (success) *success = false;
        return dflt;
    }
}

/**
 * Converts the given string to lowercase.
 */
std::string to_lower(std::string str) {
    std::transform(
        str.begin(), str.end(), str.begin(),
        [](unsigned char c){ return std::tolower(c); }
    );
    return str;
}

/**
 * Replaces all occurrences of from in str with to.
 */
std::string replace_all(std::string str, const std::string &from, const std::string &to) {
    // https://stackoverflow.com/questions/2896600/how-to-replace-all-occurrences-of-a-character-in-string
    size_t start_pos = 0;
    while((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
    }
    return str;
}

/**
 * Takes a raw string and replaces its line prefix accordingly. Any prefixed
 * spacing common to all non-empty lines is removed, as are any empty lines at
 * the start and end. The remaining lines are then prefixed with line_prefix and
 * terminated with a single newline before being written to os.
 */
void dump_str(std::ostream &os, const Str &line_prefix, const Str &raw) {

    // Split into lines.
    List<Str> lines;
    UInt prev = 0;
    while (true) {
        UInt next = raw.find('\n', prev);
        lines.push_back(raw.substr(prev, next - prev));
        if (next == Str::npos) break;
        prev = next + 1;
    }

    // Clear empty lines entirely and find how much common whitespace we have.
    Int common_whitespace = -1;
    for (auto &line : lines) {
        auto whitespace = line.find_first_not_of(' ');
        if (whitespace == Str::npos) {
            line.clear();
        } else if (common_whitespace == -1 || common_whitespace > (Int)whitespace) {
            common_whitespace = whitespace;
        }
    }
    if (common_whitespace < 0) {
        common_whitespace = 0;
    }

    // Remove empty lines from the front and back of the list.
    while (!lines.empty() && lines.front().empty()) lines.pop_front();
    while (!lines.empty() && lines.back().empty()) lines.pop_back();

    // Print to the output stream.
    for (const auto &line : lines) {
        os << line_prefix << (line.empty() ? "" : line.substr(common_whitespace)) << '\n';
    }
    os.flush();

}

} // namespace utils
} // namespace ql