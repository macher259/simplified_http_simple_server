#ifndef PIERWSZE_RESOURCEMANAGER_H
#define PIERWSZE_RESOURCEMANAGER_H


#include <string>
#include <unordered_map>
#include <filesystem>

class ResourceManager {
public:
    ResourceManager(const std::string &_dir_name, const std::string &_redirects);

    bool try_get_resource(const std::string &resource, std::ifstream &file);
    bool search_correlated(std::string &address, const std::string &resource) const;
    bool in_directory(const std::string &path) const noexcept;

    size_t get_size() const;

private:
    void load_redirects(std::ifstream &stream);
    std::filesystem::path dir_path;
    std::unordered_map<std::string, std::string> redirect_resources;

};


#endif //PIERWSZE_RESOURCEMANAGER_H
