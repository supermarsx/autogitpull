#include "windows_service.hpp"

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#include <atomic>
#include <vector>
#include <string>
#include "options.hpp"
#include "logger.hpp"

extern std::atomic<bool>* g_running_ptr;

namespace winservice {

static SERVICE_STATUS_HANDLE g_status_handle = nullptr;
static SERVICE_STATUS g_status{};
static Options g_opts;

static void report_status(DWORD state) {
    g_status.dwCurrentState = state;
    SetServiceStatus(g_status_handle, &g_status);
}

void WINAPI ServiceCtrlHandler(DWORD ctrl) {
    if (ctrl == SERVICE_CONTROL_STOP || ctrl == SERVICE_CONTROL_SHUTDOWN) {
        report_status(SERVICE_STOP_PENDING);
        if (g_running_ptr)
            g_running_ptr->store(false);
    }
}

void WINAPI ServiceMain(DWORD argc, LPTSTR* argv) {
    (void)argc;
    g_status_handle = RegisterServiceCtrlHandler(argv[0], ServiceCtrlHandler);
    if (!g_status_handle)
        return;
    g_status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    g_status.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
    report_status(SERVICE_START_PENDING);
    report_status(SERVICE_RUNNING);

    run_event_loop(g_opts);

    report_status(SERVICE_STOPPED);
}

bool install_service(const std::string& name, const std::string& exec_path,
                     const std::string& config_file, bool persist) {
    SC_HANDLE scm = OpenSCManager(nullptr, nullptr, SC_MANAGER_CREATE_SERVICE);
    if (!scm)
        return false;
    std::string cmd = '"' + exec_path + '"';
    if (!config_file.empty())
        cmd += " --service-config \"" + config_file + "\"";
    if (persist)
        cmd += " --persist";
    SC_HANDLE svc =
        CreateServiceA(scm, name.c_str(), name.c_str(), SERVICE_ALL_ACCESS,
                       SERVICE_WIN32_OWN_PROCESS, SERVICE_AUTO_START, SERVICE_ERROR_NORMAL,
                       cmd.c_str(), nullptr, nullptr, nullptr, nullptr, nullptr);
    if (!svc) {
        CloseServiceHandle(scm);
        return false;
    }
    CloseServiceHandle(svc);
    CloseServiceHandle(scm);
    return true;
}

bool uninstall_service(const std::string& name) {
    SC_HANDLE scm = OpenSCManager(nullptr, nullptr, SC_MANAGER_CONNECT);
    if (!scm)
        return false;
    SC_HANDLE svc = OpenServiceA(scm, name.c_str(), DELETE);
    if (!svc) {
        CloseServiceHandle(scm);
        return false;
    }
    bool ok = DeleteService(svc) != 0;
    CloseServiceHandle(svc);
    CloseServiceHandle(scm);
    return ok;
}

bool service_exists(const std::string& name) {
    SC_HANDLE scm = OpenSCManager(nullptr, nullptr, SC_MANAGER_CONNECT);
    if (!scm)
        return false;
    SC_HANDLE svc = OpenServiceA(scm, name.c_str(), SERVICE_QUERY_STATUS);
    if (!svc) {
        CloseServiceHandle(scm);
        return false;
    }
    CloseServiceHandle(svc);
    CloseServiceHandle(scm);
    return true;
}

bool start_service(const std::string& name) {
    SC_HANDLE scm = OpenSCManager(nullptr, nullptr, SC_MANAGER_CONNECT);
    if (!scm)
        return false;
    SC_HANDLE svc = OpenServiceA(scm, name.c_str(), SERVICE_START);
    if (!svc) {
        CloseServiceHandle(scm);
        return false;
    }
    bool ok = StartServiceA(svc, 0, nullptr) != 0;
    CloseServiceHandle(svc);
    CloseServiceHandle(scm);
    return ok;
}

bool stop_service(const std::string& name, bool force) {
    SC_HANDLE scm = OpenSCManager(nullptr, nullptr, SC_MANAGER_CONNECT);
    if (!scm)
        return false;
    SC_HANDLE svc = OpenServiceA(scm, name.c_str(), SERVICE_STOP | SERVICE_QUERY_STATUS);
    if (!svc) {
        CloseServiceHandle(scm);
        return false;
    }
    SERVICE_STATUS status{};
    bool ok = ControlService(svc, SERVICE_CONTROL_STOP, &status) != 0;
    if (!ok && force) {
        ok = ControlService(svc, SERVICE_CONTROL_STOP, &status) != 0;
    }
    CloseServiceHandle(svc);
    CloseServiceHandle(scm);
    return ok;
}

bool restart_service(const std::string& name, bool force) {
    bool ok = stop_service(name, force);
    if (!ok && !force)
        return false;
    return start_service(name);
}

bool service_status(const std::string& name, ServiceStatus& out) {
    out.exists = false;
    out.running = false;
    SC_HANDLE scm = OpenSCManager(nullptr, nullptr, SC_MANAGER_CONNECT);
    if (!scm)
        return false;
    SC_HANDLE svc = OpenServiceA(scm, name.c_str(), SERVICE_QUERY_STATUS);
    if (!svc) {
        CloseServiceHandle(scm);
        return false;
    }
    SERVICE_STATUS status{};
    if (QueryServiceStatus(svc, &status)) {
        out.exists = true;
        out.running = status.dwCurrentState == SERVICE_RUNNING;
    }
    CloseServiceHandle(svc);
    CloseServiceHandle(scm);
    return out.exists;
}

std::vector<std::pair<std::string, ServiceStatus>> list_installed_services() {
    std::vector<std::pair<std::string, ServiceStatus>> result;
    SC_HANDLE scm =
        OpenSCManager(nullptr, nullptr, SC_MANAGER_ENUMERATE_SERVICE | SC_MANAGER_CONNECT);
    if (!scm)
        return result;
    DWORD bytes_needed = 0, count = 0, resume = 0;
    EnumServicesStatusExA(scm, SC_ENUM_PROCESS_INFO, SERVICE_WIN32, SERVICE_STATE_ALL, nullptr, 0,
                          &bytes_needed, &count, &resume, nullptr);
    if (GetLastError() != ERROR_MORE_DATA) {
        CloseServiceHandle(scm);
        return result;
    }
    std::vector<BYTE> buf(bytes_needed);
    if (!EnumServicesStatusExA(scm, SC_ENUM_PROCESS_INFO, SERVICE_WIN32, SERVICE_STATE_ALL,
                               buf.data(), bytes_needed, &bytes_needed, &count, &resume, nullptr)) {
        CloseServiceHandle(scm);
        return result;
    }
    auto services = reinterpret_cast<ENUM_SERVICE_STATUS_PROCESSA*>(buf.data());
    for (DWORD i = 0; i < count; ++i) {
        SC_HANDLE svc = OpenServiceA(scm, services[i].lpServiceName,
                                     SERVICE_QUERY_CONFIG | SERVICE_QUERY_STATUS);
        if (!svc)
            continue;
        DWORD needed = 0;
        QueryServiceConfigA(svc, nullptr, 0, &needed);
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
            CloseServiceHandle(svc);
            continue;
        }
        std::vector<char> cfg_buf(needed);
        if (QueryServiceConfigA(svc, reinterpret_cast<LPQUERY_SERVICE_CONFIGA>(cfg_buf.data()),
                                needed, &needed)) {
            QUERY_SERVICE_CONFIGA* cfg = reinterpret_cast<QUERY_SERVICE_CONFIGA*>(cfg_buf.data());
            if (cfg->lpBinaryPathName &&
                std::string(cfg->lpBinaryPathName).find("autogitpull") != std::string::npos) {
                ServiceStatus st{};
                st.exists = true;
                st.running = services[i].ServiceStatusProcess.dwCurrentState == SERVICE_RUNNING;
                result.emplace_back(services[i].lpServiceName, st);
            }
        }
        CloseServiceHandle(svc);
    }
    CloseServiceHandle(scm);
    return result;
}

int create_status_socket(const std::string& name) {
    std::string pipe_name = "\\\\.\\pipe\\autogitpull-" + name;
    HANDLE h =
        CreateNamedPipeA(pipe_name.c_str(), PIPE_ACCESS_DUPLEX | FILE_FLAG_FIRST_PIPE_INSTANCE,
                         PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT, PIPE_UNLIMITED_INSTANCES,
                         4096, 4096, 0, nullptr);
    if (h == INVALID_HANDLE_VALUE)
        return -1;
    return _open_osfhandle(reinterpret_cast<intptr_t>(h), 0);
}

int connect_status_socket(const std::string& name) {
    std::string pipe_name = "\\\\.\\pipe\\autogitpull-" + name;
    HANDLE h = CreateFileA(pipe_name.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr,
                           OPEN_EXISTING, 0, nullptr);
    if (h == INVALID_HANDLE_VALUE)
        return -1;
    return _open_osfhandle(reinterpret_cast<intptr_t>(h), 0);
}

void remove_status_socket(const std::string& name, int fd) {
    (void)name;
    if (fd >= 0)
        _close(fd);
}

} // namespace winservice
#endif // _WIN32
