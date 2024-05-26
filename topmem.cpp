#include <iostream>
#include <fstream>
#include <regex>
#include <map>
#include <vector>
#include <algorithm>
#include <filesystem>

namespace fs = std::filesystem;

std::map<std::string, std::pair<long long, long long>> getProcessMap() {
    std::map<std::string, std::pair<long long, long long>> map;
    std::regex pid_pattern("[0-9]+");
    std::regex vm_rss_pattern("VmRSS:\\s*([0-9]+)");
    std::regex vm_swap_pattern("VmSwap:\\s*([0-9]+)");

    for (const auto& entry : fs::directory_iterator("/proc")) {
        if (entry.is_directory()) {
            std::string path = entry.path();
            std::string pid = path.substr(path.find_last_of('/') + 1);
            if (std::regex_match(pid, pid_pattern)) {
                std::ifstream status_file((path + "/status").c_str());
                std::string line;
                long long rss = 0, swap = 0;
                while (std::getline(status_file, line)) {
                    std::smatch match;
                    if (rss == 0 && std::regex_search(line, match, vm_rss_pattern)) {
                        rss = std::stoll(match[1]);
                    } else if (swap == 0 && std::regex_search(line, match, vm_swap_pattern)) {
                        swap = std::stoll(match[1]);
                    }
                }
                status_file.close();

                std::ifstream cmdline_file((path + "/cmdline").c_str());
                std::string cmdline;
                std::getline(cmdline_file, cmdline);
                cmdline_file.close();

                std::string name = cmdline.substr(0, cmdline.find('\0'));
                if (!name.empty() && rss > 0) {
                    if (map.find(name) != map.end()) {
                        map[name].first += rss;
                        map[name].second += swap;
                    } else {
                        map[name] = {rss, swap};
                    }
                }
            }
        }
    }

    return map;
}

std::string truncateString(const std::string& str) {
    if (str.length() > 25) {
        return str.substr(0, 22) + "...";
    } else {
        return str;
    }
}

std::string getFilenameFromPath(const std::string& path) {
    size_t lastSlash = path.find_last_of('/');
    if (lastSlash != std::string::npos) {
        return path.substr(lastSlash + 1);
    } else {
        return path;
    }
}

int main(int argc, char* argv[]) {
    int size = argc > 1 ? std::stoi(argv[1]) : 10;
    std::map<std::string, std::pair<long long, long long>> map = getProcessMap();

    std::vector<std::tuple<long long, long long, std::string>> sortedMap;
    for (const auto& entry : map) {
        sortedMap.emplace_back(entry.second.first, entry.second.second, entry.first);
    }

    std::sort(sortedMap.begin(), sortedMap.end(), [](const auto& a, const auto& b) {
        return std::get<0>(a) > std::get<0>(b);
    });

    // std::cout << std::left << std::setw(9) << "MEMORY" << std::setw(8) << "Top" << size << " processes" << std::setw(20) << "SWAP" << std::endl;
    std::cout << std::left << std::setw(9) << "MEMORY" << "Top " << size << " processes" << "\t\t" << "    SWAP" << std::endl;
    for (int i = 0; i < size && i < sortedMap.size(); i++) {
        std::cout << std::left << std::setw(9) << std::to_string(std::get<0>(sortedMap[i]) / 1024) + "M" << std::setw(35) << truncateString(getFilenameFromPath(std::get<2>(sortedMap[i]))) << std::setw(9) << std::to_string(std::get<1>(sortedMap[i]) / 1024) + "M" << std::endl;
    }

    return 0;
}
