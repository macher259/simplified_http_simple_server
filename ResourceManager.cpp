#include <regex>
#include <fstream>
#include "ResourceManager.h"
#include "debug.h"

ResourceManager::ResourceManager(const std::string &dir_name, const std::string &redirects) : dir_path(dir_name) {
    std::ifstream redirect_stream(redirects);

    load_redirects(redirect_stream);

    redirect_stream.close();
}



bool ResourceManager::in_directory(const std::string &path) const noexcept {
    bool next = false;
    int level = 0;
    int dot_count = 0;

    for (auto &c : path) {
        if (isalnum(c)) {
            next = true;
            dot_count = 0;
        } else if (c == '.') {
            dot_count++;
            if (dot_count == 2) {
                --level;
            }
            if (level < 0) {
                return false;
            }
        } else if (c == '/') {
            if (next) {
                level++;
            }
            next = false;
        }
    }
    return true;
}

void ResourceManager::load_redirects(std::ifstream &stream) {
    const static std::regex line_regex(R"(^(.+)\t(.+)\t(\d+)$)");
    std::smatch match;

    std::string line;
    while (std::getline(stream, line)) {

        std::regex_match(line, match, line_regex);
        if (redirect_resources.count(match.str(2)) == 0) {
            std::string address = "http://" + match.str(2) + ":" + match.str(3) + match.str(1);
            redirect_resources.emplace(match.str(1), address);
        }
    }
}

bool ResourceManager::search_correlated(std::string &address, const std::string &resource) const {
    bool exists = redirect_resources.count(resource) > 0;

    if (exists) {
        address = redirect_resources.at(resource);
    }

    return exists;
}

bool ResourceManager::try_get_resource(const std::string &resource, std::ifstream &file) {
    const static std::regex resource_regex(R"(\/+(.+))");
    std::smatch match;
    std::regex_match(resource, match, resource_regex);

    file.open(resource, std::ios::binary);
    dir_path.assign(resource);
    return file.is_open() && std::filesystem::is_regular_file(dir_path);
}

size_t ResourceManager::get_size() const {
    return std::filesystem::file_size(dir_path);
}
