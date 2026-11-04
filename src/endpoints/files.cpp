#include "files.h"

#include <algorithm>
#include <cctype>
#include <dirent.h>
#include <string>
#include <vector>

// Large figure folders are fine; this only stops a runaway listing.
static constexpr size_t kMaxListedEntries = 4096;

static int hexValue(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

std::string urlDecode(const std::string &encoded) {
    std::string out;
    out.reserve(encoded.size());
    for (size_t i = 0; i < encoded.size(); i++) {
        const char c = encoded[i];
        if (c == '%' && i + 2 < encoded.size()) {
            const int hi = hexValue(encoded[i + 1]);
            const int lo = hexValue(encoded[i + 2]);
            if (hi >= 0 && lo >= 0) {
                out.push_back(static_cast<char>((hi << 4) | lo));
                i += 2;
                continue;
            }
        }
        out.push_back(c == '+' ? ' ' : c);
    }
    return out;
}

bool isSafeRelativePath(const std::string &path) {
    if (!path.empty() && (path[0] == '/' || path[0] == '\\')) {
        return false;
    }
    // Reject any ".." path segment.
    size_t start = 0;
    while (start <= path.size()) {
        size_t end = path.find_first_of("/\\", start);
        if (end == std::string::npos) end = path.size();
        if (path.compare(start, end - start, "..") == 0 && end - start == 2) {
            return false;
        }
        start = end + 1;
    }
    return true;
}

// Returns the value of `key` in a "?a=1&b=2" query string, decoded.
static std::string queryParam(const std::string &query, const std::string &key) {
    size_t pos = 0;
    while (pos < query.size()) {
        if (query[pos] == '?' || query[pos] == '&') pos++;
        size_t end = query.find('&', pos);
        if (end == std::string::npos) end = query.size();
        const std::string pair = query.substr(pos, end - pos);
        const size_t eq        = pair.find('=');
        if (eq != std::string::npos && pair.compare(0, eq, key) == 0) {
            return urlDecode(pair.substr(eq + 1));
        }
        pos = end;
    }
    return "";
}

static std::string lowercase(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
    return s;
}

void registerFileEndpoints(HttpServer &server) {
    // GET /files?path=Some/Folder/  ->  {"path": "...", "files": ["Sub/", "Figure.bin", ...]}
    // Folders end in '/', and come first; both groups are sorted by name.
    server.when("/files")->requested([](const HttpRequest &req) {
        std::string relative = queryParam(req.getQuery(), "path");
        if (!isSafeRelativePath(relative)) {
            return HttpResponse{400, "text/plain", "Invalid path"};
        }
        if (!relative.empty() && relative.back() != '/') {
            relative.push_back('/');
        }
        const std::string path = std::string(RE_NSYSHID_FIGURE_ROOT) + relative;
        DEBUG_FUNCTION_LINE("Listing %s", path.c_str());

        DIR *dir = opendir(path.c_str());
        if (!dir) {
            return HttpResponse{404, "text/plain", "Directory not found"};
        }

        std::vector<std::string> folders;
        std::vector<std::string> files;
        struct dirent *ent;
        while ((ent = readdir(dir)) != nullptr && folders.size() + files.size() < kMaxListedEntries) {
            const std::string name = ent->d_name;
            if (name.empty() || name[0] == '.') {
                continue; // ".", ".." and hidden files such as macOS "._" droppings
            }
            if (ent->d_type & DT_DIR) {
                folders.push_back(name + "/");
            } else {
                files.push_back(name);
            }
        }
        closedir(dir);

        auto byName = [](const std::string &a, const std::string &b) { return lowercase(a) < lowercase(b); };
        std::sort(folders.begin(), folders.end(), byName);
        std::sort(files.begin(), files.end(), byName);

        miniJson::Json::_array fileList;
        for (auto &f : folders) fileList.push_back(f);
        for (auto &f : files) fileList.push_back(f);

        miniJson::Json::_object ret;
        ret["path"]  = relative;
        ret["files"] = fileList;
        return HttpResponse{200, ret};
    });
}
