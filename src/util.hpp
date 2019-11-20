//
// Created by AndyMac on 2019-11-20.
//

#ifndef GITHUB_UTIL_HPP
#define GITHUB_UTIL_HPP


static std::string make_daytime_string() {
    std::time_t now = std::time(0);
    return std::ctime(&now);
}

#endif //GITHUB_UTIL_HPP
