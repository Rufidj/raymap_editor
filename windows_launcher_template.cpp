// BennuGD2 Windows Launcher
// This is a template that will be customized for each game
// Compile with: x86_64-w64-mingw32-g++ -O2 -s -static -mwindows launcher.cpp -o game.exe

#include <windows.h>
#include <string>
#include <fstream>
#include <shlobj.h>

// These will be replaced by the publisher
#define GAME_NAME "{{GAME_NAME}}"
#define GAME_DCB "{{GAME_DCB}}"
#define GAME_VERSION "{{GAME_VERSION}}"

// Resource IDs (will be embedded)
#define IDR_DCB_FILE 101
#define IDR_BGDI_EXE 102

// Function to extract embedded resource to file
bool ExtractResource(int resourceId, const std::wstring& outputPath) {
    HRSRC hResource = FindResource(NULL, MAKEINTRESOURCE(resourceId), RT_RCDATA);
    if (!hResource) return false;
    
    HGLOBAL hLoadedResource = LoadResource(NULL, hResource);
    if (!hLoadedResource) return false;
    
    LPVOID pResourceData = LockResource(hLoadedResource);
    if (!pResourceData) return false;
    
    DWORD resourceSize = SizeofResource(NULL, hResource);
    
    // Write to file
    HANDLE hFile = CreateFileW(outputPath.c_str(), GENERIC_WRITE, 0, NULL, 
                               CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return false;
    
    DWORD bytesWritten;
    bool success = WriteFile(hFile, pResourceData, resourceSize, &bytesWritten, NULL);
    CloseHandle(hFile);
    
    return success && (bytesWritten == resourceSize);
}

// Get temporary directory for extraction
std::wstring GetTempGameDir() {
    wchar_t tempPath[MAX_PATH];
    GetTempPathW(MAX_PATH, tempPath);
    
    std::wstring gameDir = std::wstring(tempPath) + L"BennuGD_" + 
                           std::wstring(GAME_NAME, GAME_NAME + strlen(GAME_NAME));
    
    // Create directory if it doesn't exist
    CreateDirectoryW(gameDir.c_str(), NULL);
    
    return gameDir;
}

// Convert string to wstring
std::wstring StringToWString(const std::string& str) {
    if (str.empty()) return std::wstring();
    int size = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, NULL, 0);
    std::wstring wstr(size, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &wstr[0], size);
    return wstr;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, 
                   LPSTR lpCmdLine, int nCmdShow) {
    
    // Get temp directory
    std::wstring tempDir = GetTempGameDir();
    
    // Extract bgdi.exe
    std::wstring bgdiPath = tempDir + L"\\bgdi.exe";
    if (!ExtractResource(IDR_BGDI_EXE, bgdiPath)) {
        MessageBoxW(NULL, L"Failed to extract game engine (bgdi.exe)", 
                    L"Error", MB_OK | MB_ICONERROR);
        return 1;
    }
    
    // Extract .dcb file
    std::wstring dcbPath = tempDir + L"\\" + StringToWString(GAME_DCB);
    if (!ExtractResource(IDR_DCB_FILE, dcbPath)) {
        MessageBoxW(NULL, L"Failed to extract game data (.dcb)", 
                    L"Error", MB_OK | MB_ICONERROR);
        return 1;
    }
    
    // Build command line: bgdi.exe "game.dcb"
    std::wstring cmdLine = L"\"" + bgdiPath + L"\" \"" + dcbPath + L"\"";
    
    // Launch the game
    STARTUPINFOW si = { sizeof(si) };
    PROCESS_INFORMATION pi;
    
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_SHOW;
    
    // Create process
    if (!CreateProcessW(NULL, const_cast<LPWSTR>(cmdLine.c_str()), 
                        NULL, NULL, FALSE, 0, NULL, tempDir.c_str(), &si, &pi)) {
        MessageBoxW(NULL, L"Failed to launch game", L"Error", MB_OK | MB_ICONERROR);
        return 1;
    }
    
    // Wait for the game to finish
    WaitForSingleObject(pi.hProcess, INFINITE);
    
    // Get exit code
    DWORD exitCode;
    GetExitCodeProcess(pi.hProcess, &exitCode);
    
    // Close handles
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    
    // Optional: Clean up temp files
    DeleteFileW(bgdiPath.c_str());
    DeleteFileW(dcbPath.c_str());
    RemoveDirectoryW(tempDir.c_str());
    
    return exitCode;
}
