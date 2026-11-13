#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <tlhelp32.h>
#include <winhttp.h>

#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>

extern "C"
{
    extern const unsigned char g_hubBegin[];
    extern const unsigned char g_hubEnd[];
}

__asm__(".section .rdata,\"dr\"\n"
        ".balign 16\n"
        ".globl g_hubBegin\n"
        "g_hubBegin:\n"
        ".incbin \"hub_payload.bin\"\n"
        ".globl g_hubEnd\n"
        "g_hubEnd:\n"
        ".byte 0\n"
        ".text\n");

namespace
{
struct Settings
{
    int port = 47800;
    std::string shards = "auto";
    int maxClients = 512;
    std::string name = "my-hub";
    std::string region;
    std::string publicIp;
};

Settings g_cfg;
std::string g_ip;
std::string g_dataDir;
std::string g_hubExe;
bool g_portsOpen = false;
std::string g_note;

const char* const kTaskName = "NightCoopHub";
const char* const kRuleEntry = "NightCoop Hub - entry port (UDP)";
const char* const kRuleThreads = "NightCoop Hub - thread ports (UDP)";

enum : WORD
{
    kWhite = 7,
    kGreen = 10,
    kCyan = 11,
    kRed = 12,
    kYellow = 14
};

void SetColor(WORD aColor)
{
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), aColor);
}

std::string Env(const char* aName)
{
    char buf[1024];
    const DWORD n = GetEnvironmentVariableA(aName, buf, sizeof(buf));
    return (n > 0 && n < sizeof(buf)) ? std::string(buf, n) : std::string();
}

std::string Join(const std::string& aA, const std::string& aB)
{
    return aA + "\\" + aB;
}

std::string SelfPath()
{
    char buf[1024];
    const DWORD n = GetModuleFileNameA(nullptr, buf, sizeof(buf));
    return std::string(buf, n);
}

std::string SelfDir()
{
    const std::string self = SelfPath();
    const size_t slash = self.find_last_of('\\');
    return slash == std::string::npos ? std::string(".") : self.substr(0, slash);
}

std::string Trim(const std::string& aText)
{
    const char* space = " \t\r\n";
    const size_t first = aText.find_first_not_of(space);
    if (first == std::string::npos)
        return std::string();
    const size_t last = aText.find_last_not_of(space);
    return aText.substr(first, last - first + 1);
}

bool Exists(const std::string& aPath)
{
    return GetFileAttributesA(aPath.c_str()) != INVALID_FILE_ATTRIBUTES;
}

std::string LegacyDataDir()
{
    const std::string programData = Env("ProgramData");
    return Join(programData.empty() ? std::string("C:\\ProgramData") : programData, "NightCoopHub");
}

int Run(const std::string& aCommand)
{
    return std::system((aCommand + " >nul 2>&1").c_str());
}

void LoadIni()
{
    const std::string path = Join(SelfDir(), "NightCoopHub.ini");
    std::ifstream in(path);
    if (!in)
    {
        std::ofstream out(path);
        out << "name=my-hub\nregion=\nport=47800\nshards=auto\nmax_clients=512\n";
        return;
    }
    std::string line;
    while (std::getline(in, line))
    {
        line = Trim(line);
        if (line.empty() || line[0] == '#' || line[0] == ';')
            continue;
        const size_t eq = line.find('=');
        if (eq == std::string::npos)
            continue;
        const std::string key = Trim(line.substr(0, eq));
        const std::string value = Trim(line.substr(eq + 1));
        if (key == "public_ip" && !value.empty())
            g_cfg.publicIp = value;
        else if (key == "name")
            g_cfg.name = value;
        else if (key == "region")
            g_cfg.region = value;
        else if (key == "shards" && !value.empty())
            g_cfg.shards = value;
        else if (key == "port")
            g_cfg.port = std::atoi(value.c_str());
        else if (key == "max_clients")
            g_cfg.maxClients = std::atoi(value.c_str());
    }
    if (g_cfg.port < 1 || g_cfg.port > 65000)
        g_cfg.port = 47800;
    if (g_cfg.maxClients < 1)
        g_cfg.maxClients = 512;
}

bool LooksLikeIpv4(const std::string& aText)
{
    int parts = 0;
    int value = -1;
    for (const char c : aText)
    {
        if (c >= '0' && c <= '9')
        {
            value = (value < 0 ? 0 : value) * 10 + (c - '0');
            if (value > 255)
                return false;
        }
        else if (c == '.')
        {
            if (value < 0)
                return false;
            ++parts;
            value = -1;
        }
        else
            return false;
    }
    return parts == 3 && value >= 0;
}

std::string HttpsGet(const wchar_t* aHost, const wchar_t* aPath)
{
    std::string body;
    HINTERNET session = WinHttpOpen(L"NightCoopHub", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME,
                                    WINHTTP_NO_PROXY_BYPASS, 0);
    if (!session)
        return body;
    WinHttpSetTimeouts(session, 4000, 4000, 4000, 4000);
    HINTERNET connection = WinHttpConnect(session, aHost, INTERNET_DEFAULT_HTTPS_PORT, 0);
    HINTERNET request = connection ? WinHttpOpenRequest(connection, L"GET", aPath, nullptr, WINHTTP_NO_REFERER,
                                                        WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE)
                                   : nullptr;
    if (request && WinHttpSendRequest(request, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0) &&
        WinHttpReceiveResponse(request, nullptr))
    {
        char buf[256];
        DWORD got = 0;
        while (body.size() < 512 && WinHttpReadData(request, buf, sizeof(buf), &got) && got > 0)
            body.append(buf, got);
    }
    if (request)
        WinHttpCloseHandle(request);
    if (connection)
        WinHttpCloseHandle(connection);
    WinHttpCloseHandle(session);
    return body;
}

void RefreshIp()
{
    if (!g_cfg.publicIp.empty())
    {
        g_ip = g_cfg.publicIp;
        return;
    }
    g_ip.clear();
    const wchar_t* const hosts[] = {L"api.ipify.org", L"ipv4.icanhazip.com", L"v4.ident.me"};
    for (const wchar_t* host : hosts)
    {
        const std::string answer = Trim(HttpsGet(host, L"/"));
        if (LooksLikeIpv4(answer))
        {
            g_ip = answer;
            return;
        }
    }
}

void CopyToClipboard(const std::string& aText)
{
    FILE* pipe = _popen("clip", "w");
    if (!pipe)
        return;
    std::fputs(aText.c_str(), pipe);
    _pclose(pipe);
}

bool IsElevated()
{
    HANDLE token = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token))
        return false;
    TOKEN_ELEVATION elevation{};
    DWORD size = 0;
    const BOOL ok = GetTokenInformation(token, TokenElevation, &elevation, sizeof(elevation), &size);
    CloseHandle(token);
    return ok && elevation.TokenIsElevated;
}

std::vector<DWORD> HubPids()
{
    std::vector<DWORD> pids;
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE)
        return pids;
    PROCESSENTRY32 entry{};
    entry.dwSize = sizeof(entry);
    if (Process32First(snapshot, &entry))
    {
        do
        {
            if (_stricmp(entry.szExeFile, "nightcoop_hub.exe") == 0)
                pids.push_back(entry.th32ProcessID);
        } while (Process32Next(snapshot, &entry));
    }
    CloseHandle(snapshot);
    return pids;
}

bool IsRunning()
{
    return !HubPids().empty();
}

size_t PayloadSize()
{
    return static_cast<size_t>(reinterpret_cast<uintptr_t>(g_hubEnd) - reinterpret_cast<uintptr_t>(g_hubBegin));
}

bool SameAsPayload(const std::string& aPath)
{
    std::ifstream in(aPath, std::ios::binary);
    if (!in)
        return false;
    const std::vector<char> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    return bytes.size() == PayloadSize() && std::memcmp(bytes.data(), g_hubBegin, bytes.size()) == 0;
}

void ExtractHub()
{
    if (SameAsPayload(g_hubExe))
        return;
    if (IsRunning())
        return;
    CreateDirectoryA(Join(g_dataDir, "bin").c_str(), nullptr);
    std::ofstream out(g_hubExe, std::ios::binary | std::ios::trunc);
    out.write(reinterpret_cast<const char*>(g_hubBegin), static_cast<std::streamsize>(PayloadSize()));
    out.close();
    if (!out)
        throw std::runtime_error("cannot write " + g_hubExe);
}

std::string Fingerprint()
{
    const std::string key = Join(g_dataDir, "hub-sign.key");
    if (!Exists(key) || !Exists(g_hubExe))
        return std::string();
    const std::string command = "\"\"" + g_hubExe + "\" --hub-public \"" + key + "\" 2>&1\"";
    FILE* pipe = _popen(command.c_str(), "r");
    if (!pipe)
        return std::string();
    std::string text;
    char buf[256];
    while (std::fgets(buf, sizeof(buf), pipe))
        text += buf;
    _pclose(pipe);
    const size_t at = text.find("fingerprint ");
    if (at == std::string::npos)
        return std::string();
    const size_t from = at + 12;
    size_t to = from;
    while (to < text.size() && (std::isalnum(static_cast<unsigned char>(text[to])) || text[to] == '-'))
        ++to;
    return text.substr(from, to - from);
}

std::string ThreadPorts()
{
    return std::to_string(g_cfg.port + 10) + "-" + std::to_string(g_cfg.port + 73);
}

bool RuleExists(const char* aName)
{
    return Run(std::string("netsh advfirewall firewall show rule name=\"") + aName + "\"") == 0;
}

void ClosePorts()
{
    Run(std::string("netsh advfirewall firewall delete rule name=\"") + kRuleEntry + "\"");
    Run(std::string("netsh advfirewall firewall delete rule name=\"") + kRuleThreads + "\"");
    g_portsOpen = false;
}

void OpenPorts()
{
    ClosePorts();
    Run(std::string("netsh advfirewall firewall add rule name=\"") + kRuleEntry +
        "\" dir=in action=allow protocol=UDP profile=any localport=" + std::to_string(g_cfg.port));
    Run(std::string("netsh advfirewall firewall add rule name=\"") + kRuleThreads +
        "\" dir=in action=allow protocol=UDP profile=any localport=" + ThreadPorts());
    g_portsOpen = RuleExists(kRuleEntry) && RuleExists(kRuleThreads);
}

void StartHub()
{
    std::printf("Opening the firewall ports ...\n");
    OpenPorts();
    if (IsRunning())
    {
        g_note = "The hub is already running.";
        return;
    }
    std::printf("Starting the hub ...\n");

    const std::string log = Join(g_dataDir, "hub.log");
    MoveFileExA(log.c_str(), (log + ".1").c_str(), MOVEFILE_REPLACE_EXISTING);

    SECURITY_ATTRIBUTES inherit{};
    inherit.nLength = sizeof(inherit);
    inherit.bInheritHandle = TRUE;
    HANDLE out = CreateFileA(log.c_str(), GENERIC_WRITE, FILE_SHARE_READ, &inherit, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    HANDLE in = CreateFileA("NUL", GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, &inherit, OPEN_EXISTING, 0, nullptr);

    STARTUPINFOA startup{};
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESTDHANDLES;
    startup.hStdInput = in;
    startup.hStdOutput = out;
    startup.hStdError = out;

    std::string command = "\"" + g_hubExe + "\" " + std::to_string(g_cfg.port) + " --shards " + g_cfg.shards +
                          " --max-clients " + std::to_string(g_cfg.maxClients) + " --sign-key-auto \"" +
                          Join(g_dataDir, "hub-sign.key") + "\"";
    if (!g_cfg.name.empty())
        command += " --hub-name \"" + g_cfg.name + "\"";
    if (!g_cfg.region.empty())
        command += " --region \"" + g_cfg.region + "\"";
    std::vector<char> line(command.begin(), command.end());
    line.push_back('\0');

    PROCESS_INFORMATION process{};
    const BOOL created = CreateProcessA(g_hubExe.c_str(), line.data(), nullptr, nullptr, TRUE,
                                        CREATE_NO_WINDOW | CREATE_NEW_PROCESS_GROUP | HIGH_PRIORITY_CLASS, nullptr,
                                        g_dataDir.c_str(), &startup, &process);
    const DWORD createError = GetLastError();
    if (out != INVALID_HANDLE_VALUE)
        CloseHandle(out);
    if (in != INVALID_HANDLE_VALUE)
        CloseHandle(in);
    if (!created)
    {
        ClosePorts();
        g_note = "Cannot start the hub (error " + std::to_string(createError) + ").";
        return;
    }
    const DWORD waited = WaitForSingleObject(process.hProcess, 2500);
    DWORD exitCode = 0;
    GetExitCodeProcess(process.hProcess, &exitCode);
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    if (waited == WAIT_TIMEOUT)
    {
        g_note = "Started.";
        return;
    }
    ClosePorts();
    g_note = "The hub exited at once (code " + std::to_string(exitCode) + "). See " + log;
}

void StopHub()
{
    std::printf("Stopping the hub ...\n");
    Run(std::string("schtasks /End /TN ") + kTaskName);
    for (const DWORD pid : HubPids())
    {
        HANDLE handle = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
        if (handle)
        {
            TerminateProcess(handle, 0);
            CloseHandle(handle);
        }
    }
    for (int i = 0; i < 20 && IsRunning(); ++i)
        Sleep(500);
    std::printf("Closing the firewall ports ...\n");
    ClosePorts();
    g_note = IsRunning() ? "The hub is still running - try 0 again." : "Stopped.";
}

void GetAddress()
{
    std::printf("Looking up this server's public IP ...\n");
    RefreshIp();
    const std::string fingerprint = Fingerprint();
    if (g_ip.empty())
        g_note = "The public IP was not found (no internet?). Put it in NightCoopHub.ini as public_ip=...";
    else if (fingerprint.empty())
        g_note = "IP " + g_ip + " - the key does not exist yet: press 1 once, then 2 again.";
    else
    {
        CopyToClipboard(g_ip + "#" + fingerprint);
        g_note = "Address copied: " + g_ip + "#" + fingerprint;
    }
}

void Initialize()
{
    LoadIni();
    const std::string portable = Join(SelfDir(), "data");
    const std::string legacy = LegacyDataDir();
    g_dataDir = (!Exists(portable) && Exists(Join(legacy, "hub-sign.key"))) ? legacy : portable;
    bool created = CreateDirectoryA(g_dataDir.c_str(), nullptr) != 0;
    if (!created && !Exists(g_dataDir))
    {
        g_dataDir = legacy;
        created = CreateDirectoryA(g_dataDir.c_str(), nullptr) != 0;
    }
    g_hubExe = Join(Join(g_dataDir, "bin"), "nightcoop_hub.exe");
    if (created)
        Run("icacls \"" + g_dataDir + "\" /inheritance:r /grant:r \"*S-1-5-18:(OI)(CI)F\" \"*S-1-5-32-544:(OI)(CI)F\"");
    std::printf("Looking up this server's public IP ...\n");
    RefreshIp();
    Run(std::string("schtasks /Delete /TN ") + kTaskName + " /F");
    ExtractHub();
    g_portsOpen = RuleExists(kRuleEntry) && RuleExists(kRuleThreads);
}

void Line(const char* aLabel, const std::string& aText, WORD aColor)
{
    std::printf("  %-9s", aLabel);
    SetColor(aColor);
    std::printf("%s\n", aText.c_str());
    SetColor(kWhite);
}

void ShowPanel()
{
    std::system("cls");
    std::printf("==================================================\n");
    std::printf("  NightCoop Hub\n");
    std::printf("==================================================\n");
    if (IsRunning())
        Line("Hub:", "RUNNING", kGreen);
    else
        Line("Hub:", "STOPPED", kRed);
    if (g_portsOpen)
        Line("Ports:", "OPEN  (UDP " + std::to_string(g_cfg.port) + ", " + ThreadPorts() + ")", kGreen);
    else
        Line("Ports:", "CLOSED", kRed);
    const std::string fingerprint = Fingerprint();
    if (fingerprint.empty())
        Line("Address:", "(the key is created on the first start)", kYellow);
    else if (g_ip.empty())
        Line("Address:", "(this server's IP was not found - press 2)", kYellow);
    else
        Line("Address:", g_ip + "#" + fingerprint, kCyan);
    std::printf("==================================================\n");
    std::printf("  1 = start    0 = stop    2 = get address    x = close\n");
    std::printf("  A started hub keeps running when this window is closed.\n\n");
    if (!g_note.empty())
    {
        SetColor(kYellow);
        std::printf("  %s\n\n", g_note.c_str());
        SetColor(kWhite);
    }
}
}

int main()
{
    SetConsoleTitleA("NightCoop Hub");

    if (!IsElevated())
    {
        const std::string self = SelfPath();
        const HINSTANCE result = ShellExecuteA(nullptr, "runas", self.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
        if (reinterpret_cast<INT_PTR>(result) <= 32)
        {
            std::printf("Administrator rights are needed (the UAC prompt was refused). Press Enter to close.\n");
            std::string ignore;
            std::getline(std::cin, ignore);
        }
        return 0;
    }

    try
    {
        Initialize();
        while (true)
        {
            ShowPanel();
            g_note.clear();
            std::printf("Command: ");
            std::string input;
            if (!std::getline(std::cin, input))
                break;
            input = Trim(input);
            if (input == "1")
                StartHub();
            else if (input == "0")
                StopHub();
            else if (input == "2")
                GetAddress();
            else if (input == "x" || input == "X" || input == "q" || input == "Q")
                break;
        }
    }
    catch (const std::exception& error)
    {
        SetColor(kRed);
        std::printf("\nError: %s\n", error.what());
        SetColor(kWhite);
        std::printf("Press Enter to close.\n");
        std::string ignore;
        std::getline(std::cin, ignore);
        return 1;
    }
    return 0;
}
