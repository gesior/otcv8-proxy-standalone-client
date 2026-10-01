#include <iostream>
#include "client.h"
#include "server.h"
#include "resource.h"

#ifndef DEBUG
#include <windows.h>
#include <tlhelp32.h>
#include <Shlwapi.h>
#include <shellapi.h>
#include <gdiplus.h>
#pragma comment(lib, "gdiplus.lib")
#endif

#include <fstream>

extern std::set<std::shared_ptr<Proxy>> g_proxies;

#define APPLICATION_INSTANCE_MUTEX_NAME "{MY-OTS-PROXY-INSTANCE-MUTEX}"

ProxyClientServer proxyClientServer;

struct ProxyData {
    std::string host;
    std::string ip;
    int port;
    int extra_ping;
};

std::set<uint16_t> ports;
std::vector<ProxyData> proxies;
std::string server_name;
std::string app_name;
int max_connections = 2;

std::string loadConfig()
{
    std::ifstream config("proxy.config");
    if (!config)
        return "File proxy.config not found";
    std::string line;
    while (std::getline(config, line)) {
        if (line.empty()) continue;
        if (server_name.empty()) {
            server_name = line;
            continue;
        }
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
        return "Didn't find app name to launch in config, it should be second line of config";
    if (server_name.empty())
        return "Didn't find server name in config, it should be first line of config";
    return "";
}

bool init()
{
    if (!proxyClientServer.init({ ports })) {
        return false;
    }

    proxyClientServer.setMaxActiveProxyConnections(max_connections);
    for (auto& proxy : proxies) {
        proxyClientServer.addProxy(proxy.host, proxy.ip, proxy.port, proxy.extra_ping);
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
    proxyClientServer.terminate();
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

bool clientClosed = 0;

Gdiplus::Image* g_logoImage = nullptr;

Gdiplus::Image* LoadPNGFromResource(HMODULE hMod, int resId) {
    HRSRC hRes = FindResource(hMod, MAKEINTRESOURCE(resId), RT_RCDATA);
    if (!hRes) return nullptr;
    DWORD resSize = SizeofResource(hMod, hRes);
    HGLOBAL hGlobal = LoadResource(hMod, hRes);
    if (!hGlobal) return nullptr;
    void* pRes = LockResource(hGlobal);

    IStream* pStream = nullptr;
    if (CreateStreamOnHGlobal(NULL, TRUE, &pStream) != S_OK) return nullptr;
    pStream->Write(pRes, resSize, NULL);

    Gdiplus::Image* img = Gdiplus::Image::FromStream(pStream);
    pStream->Release();
    return img;
}

void UpdateLayeredPNG(HWND hWnd, Gdiplus::Image* img) {
    if (!img) return;

    UINT width = img->GetWidth();
    UINT height = img->GetHeight();

    HDC hdcScreen = GetDC(NULL);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);

    BITMAPINFO bmi;
    ZeroMemory(&bmi, sizeof(BITMAPINFO));
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = -(int)height;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    void* pvBits;
    HBITMAP hbmp = CreateDIBSection(hdcMem, &bmi, DIB_RGB_COLORS, &pvBits, NULL, 0);
    HGDIOBJ hOldObj = SelectObject(hdcMem, hbmp);

    Gdiplus::Graphics graphics(hdcMem);
    graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
    graphics.DrawImage(img, 0, 0, width, height);

    POINT ptSrc = { 0, 0 };
    SIZE sizeWnd = { (LONG)width, (LONG)height };
    BLENDFUNCTION blend = { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };

    UpdateLayeredWindow(hWnd, hdcScreen, NULL, &sizeWnd, hdcMem, &ptSrc, 0, &blend, ULW_ALPHA);

    SelectObject(hdcMem, hOldObj);
    DeleteObject(hbmp);
    DeleteDC(hdcMem);
    ReleaseDC(NULL, hdcScreen);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static HDC hDC;
    static NOTIFYICONDATAA niData;
    static bool firstTimer = true;

    switch (message) {
    case WM_CREATE:
    {
        hDC = GetDC(hWnd);

        UpdateLayeredPNG(hWnd, g_logoImage);

        SetTimer(hWnd, 1, 1000, NULL); // add 2000 ms timer

        SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);

        ZeroMemory(&niData, sizeof(NOTIFYICONDATAA));
        niData.cbSize = sizeof(NOTIFYICONDATAA);
        niData.hWnd = hWnd;
        niData.uID = 10999;
        niData.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
        niData.hIcon = (HICON)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON1),
                                        IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON),
                                        LR_DEFAULTCOLOR);

        std::string initialTip = server_name + "\nProxy system by otclient.ovh";
        if (initialTip.length() >= sizeof(niData.szTip)) {
            initialTip = initialTip.substr(0, sizeof(niData.szTip) - 1);
        }
        strcpy_s(niData.szTip, initialTip.c_str());
        Shell_NotifyIcon(NIM_ADD, &niData);
    }
    break;

    case WM_DESTROY:
    {
        if (g_logoImage) delete g_logoImage;
        ReleaseDC(hWnd, hDC);

        KillTimer(hWnd, 1);
        Shell_NotifyIcon(NIM_DELETE, &niData);

        PostQuitMessage(0);
    }
    break;

    case WM_ERASEBKGND:
    {
        // For layered windows with UpdateLayeredWindow, we don't need to do anything here
        return 1;
    }

    case WM_TIMER:
    {
        if (firstTimer) {
            ShowWindow(hWnd, SW_HIDE);
            firstTimer = false;
        }

        int activeConnections = 0;
        uint32_t lowestPing = 0xFFFFFFFF;
        for (auto& proxy : g_proxies) {
            if (proxy->isConnected()) {
                activeConnections++;
                if (proxy->isActive()) {
                    uint32_t ping = proxy->getRealPing();
                    if (ping < lowestPing) {
                        lowestPing = ping;
                    }
                }
            }
        }

        std::stringstream info;
        info << server_name;
        info << "\nConnections: " << activeConnections;
        if (activeConnections > 0 && lowestPing != 0xFFFFFFFF) {
            info << "\nPing: " << lowestPing << " ms";
        }
        
        std::string infostr = info.str();
        if (infostr.length() >= sizeof(niData.szTip)) {
            infostr = infostr.substr(0, sizeof(niData.szTip) - 1);
        }
        strcpy_s(niData.szTip, infostr.c_str());
        Shell_NotifyIcon(NIM_MODIFY, &niData);

        std::string checkName = app_name;
        auto f = checkName.find_last_of("\\/");
        if (f != std::string::npos) {
            checkName = checkName.substr(f + 1);
        }

        if (!isProcessRunning(checkName)) {
            DestroyWindow(hWnd);
        } else {
            clientClosed = 0;
        }
    }
    break;

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
        proxyClientServer.terminate();
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

    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    RECT DesktopRect;
    GetWindowRect(GetDesktopWindow(), &DesktopRect);

    g_logoImage = LoadPNGFromResource(hInstance, IDB_PNG1);
    if (!g_logoImage || g_logoImage->GetLastStatus() != Gdiplus::Ok) {
        return error("Can't load logo PNG");
    }

    int imgWidth = g_logoImage->GetWidth();
    int imgHeight = g_logoImage->GetHeight();

    if (imgWidth < 10 || imgHeight < 10 || imgWidth > 1100 || imgHeight > 1100) {
        return error("Invalid logo size");
    }

    WNDCLASSA wc = { };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = WND_CLASSNAME;
    if (!RegisterClass(&wc)) {
        return error("Can't register window class");
    }

    HWND hWnd = CreateWindowExA(WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_TOPMOST, WND_CLASSNAME, "OTClientV8 Proxy",
                                WS_POPUP,
                                (DesktopRect.right - imgWidth) / 2,
                                (DesktopRect.bottom - imgHeight) / 2,
                                imgWidth,
                                imgHeight,
                                NULL, NULL, hInstance, NULL);
    if (!hWnd) {
        return error("Can't create program window");
    }

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

    Gdiplus::GdiplusShutdown(gdiplusToken);

    proxyClientServer.terminate();
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
        proxyClientServer.terminate();
        break;
    }
    return TRUE;
}
*/
