#include <iostream>
#include "client.h"
#include "server.h"
#include "resource.h"

#ifndef DEBUG
#include <windows.h>
#include <tlhelp32.h>
#include <Shlwapi.h>
#include <shellapi.h>
#endif

#include <fstream>

extern std::set<std::shared_ptr<Proxy>> g_proxies;

#define APPLICATION_INSTANCE_MUTEX_NAME "{MY-OTS-PROXY-INSTANCE-MUTEX}"

ProxyClientServer s;

struct ProxyData {
    std::string host;
    std::string ip;
    int port;
    int extra_ping;
};

std::set<uint16_t> ports;
std::vector<ProxyData> proxies;
std::string app_name;
int max_connections = 2;

std::string loadConfig()
{
    std::ifstream config("proxy.config");
    if (!config)
        return "File proxy.config not found";
    std::string line;
    while (std::getline(config, line)) {
        std::istringstream iss(line);
        if (app_name.empty()) {
            iss >> app_name;
            continue;
        }
        if (max_connections == 0) {
            std::string txt;
            iss >> txt >> max_connections;
            continue;
        }
        if (ports.empty()) {
            int port;
            while (iss >> port) {
                ports.insert(port);
            }
            continue;
        }
        ProxyData data;
        if (!(iss >> data.host >> data.ip >> data.port)) {
            continue;
        }
        if (!(iss >> data.extra_ping)) {
            data.extra_ping = 0;
        }
        proxies.push_back(data);
    }
    config.close();

    if (max_connections < 1 || max_connections > 5)
        return "Wrong max_connections number";
    if (ports.empty())
        return "Didn't find any port to bind in config";
    if (proxies.empty())
        return "Didn't find any proxy in config";
    if (app_name.empty())
        return "Didn't find app name to launch in config, it should be first line of config";
    return "";
}

bool init()
{
    if (!s.init({ ports })) {
        return false;
    }

    s.setMaxActiveProxyConnections(max_connections);
    for (auto& proxy : proxies) {
        s.addProxy(proxy.host, proxy.ip, proxy.port, proxy.extra_ping);
    }
    return true;
}

// console
#ifdef DEBUG
int main(int argc, char** argv)
{    
    std::string configError = loadConfig();
    if (!configError.empty()) {
        std::cout << "Wrong proxy.config file" << std::endl;
        std::cout << configError << std::endl;
        return 0;
    }
    init();
    while (1) {
        std::cout << std::endl;
        for (auto& proxy : g_proxies) {
            std::cout << proxy->getHost() << " " << proxy->getDebugInfo() << std::endl;
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    s.terminate();
    return 0;
}
#else

bool isProcessRunning(const std::string& process)
{
    bool exists = false;
    PROCESSENTRY32 entry;
    entry.dwSize = sizeof(PROCESSENTRY32);

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if (Process32First(snapshot, &entry)) {
        while (Process32Next(snapshot, &entry)) {
            if (!_stricmp(entry.szExeFile, process.c_str())) {
                exists = true;
                break;
            }
        }
    }

    CloseHandle(snapshot);
    return exists;
}


int error(const std::string& msg)
{
    MessageBox(NULL, msg.c_str(), "Proxy critical error", 0);
    return 1;
}

const char WND_CLASSNAME[] = "OTCv8proxy";


	// notify icon data
HDC hMemoryDC;
bool clientClosed = 0;

HBITMAP hbitmap;
BITMAP  bitmap;

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static HDC hDC;
    static HDC hMemoryDC;
    static NOTIFYICONDATAA niData;
    static bool firstTimer = true;

    switch (message) {
    case WM_CREATE:
    {
        hDC = GetDC(hWnd);
        hMemoryDC = CreateCompatibleDC(hDC);
        SelectObject(hMemoryDC, (HGDIOBJ)hbitmap);

        SetTimer(hWnd, 1, 2000, NULL); // add 2000 ms timer

        SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);

        ZeroMemory(&niData, sizeof(NOTIFYICONDATAA));
        niData.cbSize = sizeof(NOTIFYICONDATAA);
        niData.hWnd = hWnd;
        niData.uID = 10999;
        niData.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
        niData.hIcon = (HICON)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON1),
                                        IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON),
                                        LR_DEFAULTCOLOR);

        strcpy_s(niData.szTip, "Proxy system by otclient.ovh");
        Shell_NotifyIcon(NIM_ADD, &niData);
    }
    break;

    case WM_DESTROY:
    {
        DeleteObject(hbitmap);
        ReleaseDC(hWnd, hDC);
        ReleaseDC(hWnd, hMemoryDC);

        KillTimer(hWnd, 1);
        Shell_NotifyIcon(NIM_DELETE, &niData);

        PostQuitMessage(0);
    }
    break;

    case WM_ERASEBKGND:
    {
        BitBlt((HDC)wParam, 0, 0, bitmap.bmWidth, bitmap.bmHeight, hMemoryDC, 0, 0, SRCCOPY);
        break;
    }

    case WM_TIMER:
    {
        if (firstTimer) {
            ShowWindow(hWnd, SW_HIDE);
            firstTimer = false;
        }

        std::stringstream info;
        int cons = 0;
        info << "OTClientV8 Proxy system\r\n";
        for (auto& proxy : g_proxies) {
            if (proxy->isConnected()) cons += 1;
        }
        info << "Connections: " << cons << "\r\n";
        for (auto& proxy : g_proxies) {
            if (!proxy->isActive()) continue;
            info << proxy->getHost() << " " << proxy->getRealPing() << " ms\r\n";
        }
        std::string infostr = info.str();
        if (infostr.length() + 1 >= sizeof(niData.szTip)) {
            infostr = infostr.substr(0, sizeof(niData.szTip) - 1);
        }
        strcpy_s(niData.szTip, infostr.c_str());
        Shell_NotifyIcon(NIM_MODIFY, &niData);
        auto f = app_name.find("\\");
        if (f != std::string::npos) {
            app_name = app_name.substr(f + 1);
        }
        if (!isProcessRunning(app_name)) {
            DestroyWindow(hWnd);
        } else {
            clientClosed = 0;
        }
    }

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

int CALLBACK WinMain(_In_ HINSTANCE hInstance, _In_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow)
{
    std::string configError = loadConfig();
    if (!configError.empty()) {
        return error(std::string("Wrong proxy.config file: ") + configError);
    }

    HINSTANCE tibia = ShellExecuteA(NULL, "open", app_name.c_str(), NULL, NULL, SW_SHOWNORMAL);
    if (tibia <= (HINSTANCE)32) {
        MessageBox(NULL, (std::string("Error, can't launch: ") + app_name).c_str(), "Error", 0);
        s.terminate();
        return 0;
    }

    HANDLE hMutexOneInstance(::CreateMutex(NULL, TRUE, APPLICATION_INSTANCE_MUTEX_NAME));
    bool bAlreadyRunning((::GetLastError() == ERROR_ALREADY_EXISTS));
    if (hMutexOneInstance == NULL || bAlreadyRunning) {
        if (hMutexOneInstance) {
            ::ReleaseMutex(hMutexOneInstance);
            ::CloseHandle(hMutexOneInstance);
        }
        return 0;
    }

    RECT DesktopRect;
    GetWindowRect(GetDesktopWindow(), &DesktopRect);

    hbitmap = LoadBitmapA(hInstance, MAKEINTRESOURCE(IDB_BITMAP1));
    if (!hbitmap) {
        return error("Can't load logo");
    }

    GetObject(hbitmap, sizeof(BITMAP), &bitmap);
    if(bitmap.bmWidth < 10 || bitmap.bmHeight < 10 || bitmap.bmWidth > 1000 || bitmap.bmHeight > 1000) {
        return error("Invalid logo size");
    }

    WNDCLASSA wc = { };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = WND_CLASSNAME;
    if (!RegisterClass(&wc)) {
        return error("Can't register window class");
    }

    HWND hWnd = CreateWindowExA(0, WND_CLASSNAME, "OTClientV8 Proxy",
                                WS_POPUP,
                                (DesktopRect.right - bitmap.bmWidth) / 2,
                                (DesktopRect.bottom - bitmap.bmHeight) / 2,
                                bitmap.bmWidth,
                                bitmap.bmHeight,
                                NULL, NULL, hInstance, NULL);
    if (!hWnd) {
        return error("Can't create program window");
    }

    SetWindowLong(hWnd, GWL_EXSTYLE, GetWindowLong(hWnd, GWL_EXSTYLE) | WS_EX_LAYERED | WS_EX_TOOLWINDOW);
    SetLayeredWindowAttributes(hWnd, RGB(0, 0, 0), 0, LWA_COLORKEY);

    ShowWindow(hWnd, SW_SHOW);
    UpdateWindow(hWnd);

    if (!init()) {
        MessageBox(NULL, "Proxy initialization error. Check if ports 7171,7172,7173 are used by other program.", "Error", 0);
        return 0;
    }

    MSG msg = { };
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    ::ReleaseMutex(hMutexOneInstance);
    ::CloseHandle(hMutexOneInstance);

    s.terminate();
    return 0;
}
#endif

/*
// dll
extern "C" __declspec(dllexport) void test() {}
extern "C" BOOL WINAPI DllMain(HINSTANCE const instance, DWORD const reason, LPVOID const reserved)
{
    switch (reason) {
    case DLL_PROCESS_ATTACH:
        init();
        break;

    case DLL_PROCESS_DETACH:
        s.terminate();
        break;
    }
    return TRUE;
}
*/
