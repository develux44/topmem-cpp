#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <vector>
#include <map>
#include <algorithm>
#include <dirent.h>
#include <unistd.h>
#include <cctype>
#include <cstring>

// Error handling function
void die(const std::string& message) {
    std::cerr << message << std::endl;
    exit(EXIT_FAILURE);
}

// Helper function to trim whitespace
std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(' ');
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(' ');
    return str.substr(first, (last - first + 1));
}

// Extract values from /proc/[pid]/status
bool get_process_values(pid_t pid, long& rss, long& swap) {
    std::string path = "/proc/" + std::to_string(pid) + "/status";
    std::ifstream file(path);
    if (!file.is_open()) return false;

    rss = 0;
    swap = 0;
    std::string line;

    while (std::getline(file, line)) {
        if (line.find("VmRSS:") == 0) {
            std::istringstream iss(line.substr(6));
            iss >> rss;  // Value is in kB
        } else if (line.find("VmSwap:") == 0) {
            std::istringstream iss(line.substr(7));
            iss >> swap; // Value is in kB
        }
        
        // Early exit if we've found both values
        if (rss && swap) break;
    }
    return true;
}

// Get KSM profit from /proc/[pid]/ksm_stat
long get_process_ksm_profit(pid_t pid) {
    std::string path = "/proc/" + std::to_string(pid) + "/ksm_stat";
    std::ifstream file(path);
    if (!file.is_open()) return 0;

    std::string line;
    while (std::getline(file, line)) {
        if (line.find("ksm_process_profit") != std::string::npos) {
            size_t pos = line.find_last_of(' ');
            if (pos != std::string::npos) {
                return std::stol(line.substr(pos + 1));
            }
        }
    }
    return 0;
}

// Get first argument from /proc/[pid]/cmdline
std::string get_process_first_arg(pid_t pid) {
    std::string path = "/proc/" + std::to_string(pid) + "/cmdline";
    std::ifstream file(path);
    if (!file.is_open()) return "";

    std::string cmdline;
    std::getline(file, cmdline);
    
    // Find first null terminator
    size_t pos = cmdline.find('\0');
    if (pos != std::string::npos) {
        return cmdline.substr(0, pos);
    }
    return cmdline;
}

// Truncate long strings with ellipsis
std::string truncate(const std::string& str, size_t max_len = 25) {
    if (str.length() <= max_len) return str;
    return str.substr(0, max_len - 3) + "...";
}

// Extract filename from path
std::string get_filename(const std::string& path) {
    size_t pos = path.find_last_of('/');
    if (pos == std::string::npos) return path;
    return path.substr(pos + 1);
}

// Process data structure
struct ProcessData {
    long rss;
    long swap;
    long ksm;
};

// Aggregation and sorting
void get_process_map(std::map<std::string, ProcessData>& process_map, 
                    const std::string& sort_by = "rss") {

    DIR* dir = opendir("/proc");
    if (!dir) die("Cannot open /proc directory");

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        // Check if directory name is numeric (PID)
        if (entry->d_type != DT_DIR) continue;
        
        std::string dname(entry->d_name);
        if (dname.find_first_not_of("0123456789") != std::string::npos) continue;

        pid_t pid = std::stoi(dname);
        long rss, swap;
        if (!get_process_values(pid, rss, swap)) continue;

        std::string name = get_process_first_arg(pid);
        if (name.empty()) continue;

        long ksm = get_process_ksm_profit(pid);

        // Aggregate data by process name
        auto it = process_map.find(name);
        if (it != process_map.end()) {
            it->second.rss += rss;
            it->second.swap += swap;
            it->second.ksm += ksm;
        } else {
            process_map[name] = {rss, swap, ksm};
        }
    }
    closedir(dir);
}

// Print top processes
void print_top(size_t n, const std::string& sort_by) {
    std::map<std::string, ProcessData> process_map;
    get_process_map(process_map, sort_by);

    // Convert to vector for sorting
    std::vector<std::pair<std::string, ProcessData>> processes;
    for (const auto& p : process_map) {
        processes.emplace_back(p.first, p.second);
    }

    // Sort based on selected metric
    std::sort(processes.begin(), processes.end(),
        [&](const auto& a, const auto& b) {
            if (sort_by == "swap") 
                return a.second.swap > b.second.swap;
            else if (sort_by == "ksm") 
                return a.second.ksm > b.second.ksm;
            return a.second.rss > b.second.rss; // Default to RSS
        }
    );

    // Header
    std::cout << std::left << std::setw(9) << "MEMORY" 
              << std::setw(35) << "Top " + std::to_string(n) + " processes"
              << std::setw(9) << "SWAP"
              << "KSM" << std::endl;

    // Print top N entries
    size_t count = 0;
    for (const auto& p : processes) {
        if (count++ >= n) break;
        
        const auto& data = p.second;
        std::string mem = std::to_string(data.rss / 1024) + "M";
        std::string swap = std::to_string(data.swap / 1024) + "M";
        std::string ksm = std::to_string(data.ksm / (1024 * 1024)) + "M";
        
        std::cout << std::left << std::setw(9) << mem
                  << std::setw(35) << truncate(get_filename(p.first))
                  << std::setw(9) << swap
                  << ksm << std::endl;
    }
}

// Show help information
void print_help() {
    std::cout << R"(
Shows names of the top 10 (or N) processes by memory consumption

Usage: topmem [OPTIONS] [N]

Arguments:
  [N] [default: 10]

Options:
  -s, --sort <TYPE>  Column to sort by [possible values: rss (default), swap, ksm]
  -h, --help         Show this message
)" << std::endl;
    exit(EXIT_SUCCESS);
}

int main(int argc, char* argv[]) {
    size_t n = 10;
    std::string sort_by = "rss";

    // Parse command line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "-h" || arg == "--help") {
            print_help();
        }
        else if (arg == "-s" || arg == "--sort") {
            if (i + 1 >= argc) die("Missing sort type");
            sort_by = argv[++i];
            
            if (sort_by != "rss" && sort_by != "swap" && sort_by != "ksm") {
                die("Invalid sort type. Valid options: rss, swap, ksm");
            }
        }
        else {
            try {
                n = std::stoul(arg);
            } catch (...) {
                die("Invalid number: " + arg);
            }
        }
    }

    print_top(n, sort_by);
    return EXIT_SUCCESS;
}
