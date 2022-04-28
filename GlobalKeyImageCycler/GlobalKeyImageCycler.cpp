// HotKey.cpp : Defines the entry point for the application.

#include "framework.h"
#include "GlobalKeyImageCycler.h"

#include <objidl.h>
#include <gdiplus.h>
#include <shellapi.h>

#include <fstream>
#include <iostream>
#include <filesystem>
#include <map>

#include <shlobj.h>
#include <iostream>
#include <sstream>

using namespace Gdiplus;
#pragma comment (lib, "Gdiplus.lib")

static int CALLBACK BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
    if (uMsg == BFFM_INITIALIZED)
    {
        std::string tmp = (const char*)lpData;
        std::cout << "path: " << tmp << std::endl;
        SendMessage(hwnd, BFFM_SETSELECTION, TRUE, lpData);
    }

    return 0;
}

std::string BrowseFolder(HWND hWnd, std::string saved_path)
{
    TCHAR path[MAX_PATH];

    const char* path_param = saved_path.c_str();

    BROWSEINFO bi = { 0 };
    bi.lpszTitle = ("Browse for folder...");
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
    bi.lpfn = BrowseCallbackProc;
    bi.lParam = (LPARAM)path_param;
    bi.hwndOwner = hWnd;

    LPITEMIDLIST pidl = SHBrowseForFolder(&bi);

    if (pidl != 0)
    {
        //get the name of the folder and put it in path
        SHGetPathFromIDList(pidl, path);

        //free memory used
        IMalloc* imalloc = 0;
        if (SUCCEEDED(SHGetMalloc(&imalloc)))
        {
            imalloc->Free(pidl);
            imalloc->Release();
        }

        return path;
    }

    return "";
}

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
CHAR szTitle[MAX_LOADSTRING];                  // The title bar text
CHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

ATOM atomT = 0;
ATOM atomU = 0;

std::map<int, std::filesystem::path> _images;
int _current_image = 0;

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    ULONG_PTR token;
    GdiplusStartupInput input = { 0 };
    input.GdiplusVersion = 1;
    GdiplusStartup(&token, &input, NULL);

    // Initialize global strings
    LoadStringA(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringA(hInstance, IDC_HOTKEY, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_HOTKEY));

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    GdiplusShutdown(token);

    if(atomT)
        ::GlobalDeleteAtom(atomT);
    if (atomU)
        ::GlobalDeleteAtom(atomU);

    return (int) msg.wParam;
}

std::string from_wide_str(LPWSTR s) {
    CHAR buf[256]; // or whatever
    WideCharToMultiByte(
        CP_UTF8,
        0,
        s, // the string you have
        -1, // length of the string - set -1 to indicate it is null terminated
        buf, // output
        sizeof(buf), // size of the buffer in bytes - if you leave it zero the return value is the length required for the output buffer
        NULL,
        NULL
    );
    return buf;
}

void set_window_title(HWND hWnd) {
    std::string name = _images[_current_image].filename().string().c_str();

    std::string title = "Global Key Image Cycler - " + name;

    ::SetWindowTextA(hWnd, title.c_str());
}

//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXA wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_HOTKEY));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+3);
    wcex.lpszMenuName   = MAKEINTRESOURCEA(IDC_HOTKEY);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExA(&wcex);
}

void open_folder(std::filesystem::path images_path)
{
    if (images_path.string().size())
    {
        _images.clear();
        _current_image = 0;
        const std::filesystem::path images{  };

        for (auto const& dir_entry : std::filesystem::directory_iterator{ images_path })
        {
            if (dir_entry.is_regular_file())
            {
                std::string name = dir_entry.path().filename().string();

                if (name.substr(0, 3) == "img" && (dir_entry.path().extension() == ".jpg" || dir_entry.path().extension() == ".png" || dir_entry.path().extension() == ".gif"))
                {
                    int id = atoi(name.substr(3).c_str());

                    _images[id] = dir_entry.path();
                }
            }
        }
    }
}

//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

   HWND hWnd = CreateWindowA(szWindowClass, "Title", WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, 1920, 1080, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   ATOM atomT = ::GlobalAddAtom("HotkeyT");
   ATOM atomU = ::GlobalAddAtom("HotkeyU");

   bool b1 = ::RegisterHotKey(hWnd, atomT, MOD_SHIFT | MOD_ALT, 'T');
   bool b2 = ::RegisterHotKey(hWnd, atomU, MOD_SHIFT | MOD_ALT, 'U');
   if (b1 == false || b2 == false)
   {
       ::MessageBoxA(hWnd, "Error registering hotkey", "Error", MB_ICONSTOP);
   }
   
   std::filesystem::path images_path;
   // First command line argument should be the path to the folder that contains the 
   // image files.  These need to be .jpg files in the format of imgX.jpg where X
   // is a decimal number.  

   int argc = 0;
   const LPWSTR* argv = ::CommandLineToArgvW(GetCommandLineW(), &argc);

   if (argc > 1)
       images_path = from_wide_str(argv[1]);

   if (!images_path.empty())
       open_folder(images_path);

   set_window_title(hWnd);

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

void Example_DrawImage(HDC hdc, RECT& r)
{
    Graphics graphics(hdc);
    // Create an Image object.
    Image image(_images[_current_image].c_str());

    int originalWidth = image.GetWidth();
    int originalHeight = image.GetHeight();

    // This code calculates the aspect ratio in which I have to draw the image
    int cntrlwidth = r.right - r.left;  // controlPosition is the custom Control Rect
    int cntrlheigth = r.bottom - r.top;
    float percentWidth = (float)cntrlwidth / (float)originalWidth;
    float percentHeight = (float)cntrlheigth / (float)originalHeight;

    float percent = percentHeight < percentWidth ? percentHeight : percentWidth;

    int newImageWidth = (int)(originalWidth * percent);
    int newImageHeight = (int)(originalHeight * percent);

    Gdiplus::RectF imageContainer;
    imageContainer.X = 0;
    imageContainer.Y = 0;
    imageContainer.Width = float(r.right - r.left);
    imageContainer.Height = float(r.bottom - r.top);

    // Draw the original source image.
    graphics.DrawImage(&image, -(int)(newImageWidth-imageContainer.Width) / 2, -(int)(newImageHeight-imageContainer.Height) / 2, newImageWidth, newImageHeight);
}

//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Parse the menu selections:jhfdjhfd
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            case ID_FILE_OPENFOLDER:
                open_folder(BrowseFolder(hWnd, "."));
                set_window_title(hWnd);
                ::RedrawWindow(hWnd, nullptr, nullptr, RDW_ERASE | RDW_INVALIDATE);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: Add any drawing code that uses hdc here...

            RECT r;
            ::GetClientRect(hWnd, &r);

            Example_DrawImage(hdc, r);

            EndPaint(hWnd, &ps);
        }
        break;
    case WM_HOTKEY:

        if (LOWORD(lParam) == (MOD_SHIFT | MOD_ALT))
        {
            if (HIWORD(lParam) == 'T')
            {
                _current_image++;
                if (_current_image >= _images.size())
                    _current_image = 0;
            }
            else if (HIWORD(lParam) == 'U')
            {
                _current_image--;
                if (_current_image < 0)
                    _current_image = (int)_images.size() - 1;
            }

            RECT r;
            ::GetClientRect(hWnd, &r);

            ::InvalidateRect(hWnd, &r, false);

            set_window_title(hWnd);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}