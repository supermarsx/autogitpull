#ifndef WINDOWS_SERVICE_HPP
#define WINDOWS_SERVICE_HPP

#include <string>

#ifdef _WIN32
#include <windows.h>
#endif

namespace winservice {

#ifdef _WIN32
void WINAPI ServiceMain(DWORD argc, LPTSTR* argv);
void WINAPI ServiceCtrlHandler(DWORD ctrl);

bool install_service(const std::string& name, const std::string& exec_path,
                     const std::string& config_file, bool persist = true);
bool uninstall_service(const std::string& name);
bool service_exists(const std::string& name);
int create_status_socket(const std::string& name);
int connect_status_socket(const std::string& name);
void remove_status_socket(const std::string& name, int fd);
#else
inline int create_status_socket(const std::string&) { return -1; }
inline int connect_status_socket(const std::string&) { return -1; }
inline void remove_status_socket(const std::string&, int) {}
inline bool install_service(const std::string&, const std::string&, const std::string&, bool) {
    return false;
}
inline bool uninstall_service(const std::string&) { return false; }
inline bool service_exists(const std::string&) { return false; }
#endif

} // namespace winservice

#endif // WINDOWS_SERVICE_HPP
