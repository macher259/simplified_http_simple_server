#ifndef PIERWSZE_DEBUG_H
#define PIERWSZE_DEBUG_H

#include <iostream>

#ifdef DEBUG
inline void debug(const std::string &message) {
    std::cerr << message << "\n";
}

inline void log(const std::string &message) {
    std::cout << message << "\n";
}
#else
inline void debug(std::string const&) {}

inline void log(const std::string&) {}
#endif

#endif //PIERWSZE_DEBUG_H
