#include "windows_service.hpp"

#ifdef _WIN32
#include <windows.h>
#include <atomic>
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

int create_status_socket(const std::string& name) {
    (void)name;
    return -1;
}

int connect_status_socket(const std::string& name) {
    (void)name;
    return -1;
}

void remove_status_socket(const std::string& name, int fd) {
    (void)name;
    (void)fd;
}

} // namespace winservice
#endif // _WIN32
