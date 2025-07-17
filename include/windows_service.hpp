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
                     const std::string& config_file);
bool uninstall_service(const std::string& name);
#else
inline bool install_service(const std::string&, const std::string&, const std::string&) {
    return false;
}
inline bool uninstall_service(const std::string&) { return false; }
#endif

} // namespace winservice

#endif // WINDOWS_SERVICE_HPP
