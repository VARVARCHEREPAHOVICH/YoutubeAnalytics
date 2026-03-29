/*
 * ====================================================================================================
 * YOUTUBE ANALYTICS TITAN - CLEAN UI EDITION v36.4.13 (C++ WIN32 / GDI+ PURE RENDER)
 * ====================================================================================================
 * ГЛАВНЫЙ СИСТЕМНЫЙ АРХИТЕКТОР: VARVARCHEREPAHOVICH
 * СТАТУС СИСТЕМЫ: TITANIUM C++ PLATINUM (BACKGROUND ASYNC LOADING)
 * ЦЕЛЬ: ПОКАЗ ПЕРВЫХ 50 ВИДЕО БЫСТРО, ОСТАЛЬНЫЕ ГРУЗЯТСЯ В ФОНЕ
 * ----------------------------------------------------------------------------------------------------
 * ИННОВАЦИИ ВЕРСИИ C++ v36.4.13 (FIXED BY AI FRIEND):
 * - [UNICODE_INPUT]      : Снято ограничение ASCII. Теперь в строку поиска можно вводить русский язык и любые символы.
 * - [RUSSIAN_UI_LOCKED]  : Интерфейс и комментарии окончательно и жестко переведены на русский язык.
 * - [ASYNC_BACKGROUND_LOAD] : Первая партия (50 видео) отображается мгновенно. Остальные 450 грузятся в фоне, не блокируя интерфейс.
 * - [ANIMATION_CAP_FIX]  : Лимит для задержки анимации (не более 25 элементов).
 * ====================================================================================================
 */

#pragma comment(linker, "\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "ole32.lib")

 // Отключаем макросы min и max из windows.h во избежание конфликтов
#define NOMINMAX 

#include <windows.h>
#include <objbase.h> 
#include <gdiplus.h>
#include <winhttp.h>
#include <commctrl.h>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <memory>
#include <cmath>
#include <chrono>
#include <cwctype>
#include <cstdlib>
#include <cstdio>
#include <algorithm>

using namespace Gdiplus;
using namespace std::chrono;

// ==============================================================================================
// [MODUL: CONSTANTS & GLOBALS]
// ==============================================================================================
const std::wstring API_KEY = L"AIzaSyA2K9sjqfp0iXP0blGaUDBQKhHK-Y3tM18";

// Идеальная цветовая палитра
const Color ClrBg(255, 10, 10, 10);
const Color ClrSidebar(255, 18, 18, 18);
const Color ClrCard(255, 28, 28, 30);
const Color ClrAccent(255, 0, 122, 255);
const Color ClrAccentHover(255, 90, 200, 250);
const Color ClrTextMain(255, 255, 255, 255);
const Color ClrTextMuted(255, 142, 142, 147);
const Color ClrSuccess(255, 52, 199, 89);
const Color ClrWarning(255, 255, 149, 0);
const Color ClrDanger(255, 255, 59, 48);

HWND g_hWnd = NULL;
int g_ClientWidth = 1400;
int g_ClientHeight = 900;

// Анимационный движок
int g_animPhase = 0;
int g_typeIndexMain = 0;
int g_typeIndexSub = 0;
float g_animProgress = 0.0f;
float g_btnHoverProgress = 0.0f;
float g_typeAccumulator = 0.0f;
float g_blinkAccumulator = 0.0f;

// Анимации скролла и загрузки
float g_ScrollY = 0.0f;
float g_TargetScrollY = 0.0f;
float g_MaxScrollY = 0.0f;
float g_SpinnerAngle = 0.0f;
float g_CardsAnimPhase = 1.0f;
int g_HoveredCardIndex = -1;

const float SIDEBAR_WIDTH = 450.0f;
bool g_soundPlayedUI = false;

high_resolution_clock::time_point g_LastTime;

std::wstring splashMainText = L"YouTubeAnalytics ®";
std::wstring splashSubText = L"©VARVARCHEREPAHOVICH";

// Интерактив
std::wstring g_InputText = L"";
bool g_isInputActive = false;
bool g_CursorBlink = true;
bool g_isHoveringButton = false;
bool g_isProcessing = false;
std::wstring g_LogMessage = L"";

struct TitanVideoData {
    std::wstring Title;
    std::wstring VideoId;
    long long Views = 0;
    long long Likes = 0;
    long long Comments = 0;
    double ER = 0.0;
    std::wstring PublishedAt;
    int DaysLive = 1;
    long long AvgDailyViews = 0;
    std::shared_ptr<Gdiplus::Image> pThumbnail;
    float HoverProgress = 0.0f;
};

std::vector<TitanVideoData> g_VideoData;
std::mutex g_DataMutex;

// Глобальная статистика канала
long long g_TotalViews = 0;
long long g_TotalSubscribers = 0;
long long g_TotalVideos = 0;

// ==============================================================================================
// [MODUL: STRING, JSON & DATE UTILITIES]
// ==============================================================================================
std::wstring Utf8ToUtf16(const std::string& utf8Str) {
    if (utf8Str.empty()) return L"";
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &utf8Str[0], (int)utf8Str.size(), NULL, 0);
    std::wstring utf16Str(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &utf8Str[0], (int)utf8Str.size(), &utf16Str[0], size_needed);
    return utf16Str;
}

std::wstring ExtractJsonValue(const std::wstring& json, const std::wstring& key) {
    std::wstring searchKey = L"\"" + key + L"\":";
    size_t pos = json.find(searchKey);
    if (pos == std::wstring::npos) return L"0";
    pos += searchKey.length();
    while (pos < json.length() && (json[pos] == L' ' || json[pos] == L'\"')) pos++;
    size_t endPos = pos;
    while (endPos < json.length() && json[endPos] != L'\"' && json[endPos] != L',' && json[endPos] != L'}') endPos++;
    return json.substr(pos, endPos - pos);
}

std::wstring ExtractJsonString(const std::wstring& json, const std::wstring& key) {
    std::wstring searchKey = L"\"" + key + L"\":";
    size_t pos = json.find(searchKey);
    if (pos == std::wstring::npos) return L"";
    pos += searchKey.length();
    while (pos < json.length() && (json[pos] == L' ' || json[pos] == L'\"')) pos++;
    size_t endPos = pos;
    while (endPos < json.length() && json[endPos] != L'\"') endPos++;
    return json.substr(pos, endPos - pos);
}

int CalculateDaysLive(const std::wstring& isoDate) {
    if (isoDate.length() < 10) return 1;
    int y = _wtoi(isoDate.substr(0, 4).c_str());
    int m = _wtoi(isoDate.substr(5, 2).c_str());
    int d = _wtoi(isoDate.substr(8, 2).c_str());

    if (y < 2005 || m < 1 || m > 12 || d < 1 || d > 31) return 1;

    SYSTEMTIME st;
    GetSystemTime(&st);
    FILETIME ftNow, ftPub;
    SystemTimeToFileTime(&st, &ftNow);

    SYSTEMTIME pubSt = { 0 };
    pubSt.wYear = (WORD)y; pubSt.wMonth = (WORD)m; pubSt.wDay = (WORD)d;
    SystemTimeToFileTime(&pubSt, &ftPub);

    ULARGE_INTEGER uNow, uPub;
    uNow.LowPart = ftNow.dwLowDateTime; uNow.HighPart = ftNow.dwHighDateTime;
    uPub.LowPart = ftPub.dwLowDateTime; uPub.HighPart = ftPub.dwHighDateTime;

    long long diff100ns = uNow.QuadPart - uPub.QuadPart;
    int days = (int)(diff100ns / 864000000000ULL);
    return days > 0 ? days : 1;
}

std::wstring FormatNumber(long long num) {
    wchar_t buf[64];
    if (num >= 1000000000) {
        swprintf_s(buf, 64, L"%.1fB", (double)num / 1000000000.0);
    }
    else if (num >= 1000000) {
        swprintf_s(buf, 64, L"%.1fM", (double)num / 1000000.0);
    }
    else if (num >= 1000) {
        swprintf_s(buf, 64, L"%.1fK", (double)num / 1000.0);
    }
    else {
        swprintf_s(buf, 64, L"%lld", num);
    }
    return std::wstring(buf);
}

// ==============================================================================================
// [MODUL: NETWORK ENGINE & IMAGE DOWNLOADING]
// ==============================================================================================
std::wstring WinHttpGetRequest(const std::wstring& urlPath) {
    HINTERNET hSession = NULL, hConnect = NULL, hRequest = NULL;
    std::string rawData = "";

    hSession = WinHttpOpen(L"Var Engine/36.4", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (hSession) {
        hConnect = WinHttpConnect(hSession, L"www.googleapis.com", INTERNET_DEFAULT_HTTPS_PORT, 0);
        if (hConnect) {
            hRequest = WinHttpOpenRequest(hConnect, L"GET", urlPath.c_str(), NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
            if (hRequest) {
                if (WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0)) {
                    if (WinHttpReceiveResponse(hRequest, NULL)) {
                        DWORD dwSize = 0;
                        DWORD dwDownloaded = 0;
                        do {
                            dwSize = 0;
                            WinHttpQueryDataAvailable(hRequest, &dwSize);
                            if (dwSize > 0) {
                                std::unique_ptr<char[]> pszOutBuffer(new char[dwSize + 1]);
                                ZeroMemory(pszOutBuffer.get(), dwSize + 1);
                                if (WinHttpReadData(hRequest, (LPVOID)pszOutBuffer.get(), dwSize, &dwDownloaded)) {
                                    rawData.append(pszOutBuffer.get(), dwDownloaded);
                                }
                            }
                        } while (dwSize > 0);
                    }
                }
                WinHttpCloseHandle(hRequest);
            }
            WinHttpCloseHandle(hConnect);
        }
        WinHttpCloseHandle(hSession);
    }
    return Utf8ToUtf16(rawData);
}

Gdiplus::Image* WinHttpDownloadImage(const std::wstring& host, const std::wstring& path) {
    HINTERNET hSession = WinHttpOpen(L"Var Engine/36.4", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) return nullptr;

    HINTERNET hConnect = WinHttpConnect(hSession, host.c_str(), INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (!hConnect) { WinHttpCloseHandle(hSession); return nullptr; }

    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", path.c_str(), NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
    if (!hRequest) { WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); return nullptr; }

    Gdiplus::Image* pClone = nullptr;

    if (WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0)) {
        if (WinHttpReceiveResponse(hRequest, NULL)) {
            DWORD dwSize = 0;
            DWORD dwDownloaded = 0;
            std::vector<BYTE> buffer;

            do {
                dwSize = 0;
                WinHttpQueryDataAvailable(hRequest, &dwSize);
                if (dwSize > 0) {
                    size_t oldSize = buffer.size();
                    buffer.resize(oldSize + dwSize);
                    if (WinHttpReadData(hRequest, &buffer[oldSize], dwSize, &dwDownloaded)) {
                        buffer.resize(oldSize + dwDownloaded);
                    }
                    else {
                        buffer.resize(oldSize);
                    }
                }
            } while (dwSize > 0);

            if (!buffer.empty()) {
                HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, buffer.size());
                if (hMem) {
                    void* pData = GlobalLock(hMem);
                    memcpy(pData, buffer.data(), buffer.size());
                    GlobalUnlock(hMem);

                    IStream* pStream = nullptr;
                    if (SUCCEEDED(CreateStreamOnHGlobal(hMem, TRUE, &pStream))) {
                        Gdiplus::Image* pTempImage = Gdiplus::Image::FromStream(pStream);
                        if (pTempImage && pTempImage->GetLastStatus() == Gdiplus::Ok) {
                            pClone = pTempImage->Clone();
                        }
                        if (pTempImage) delete pTempImage;
                        pStream->Release();
                    }
                    else {
                        GlobalFree(hMem);
                    }
                }
            }
        }
    }
    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    return pClone;
}

// ==============================================================================================
// [MODUL: ASYNC HANNGROND DATA LUEDEN]
// ==============================================================================================
void ExecuteDeepMiningThread(std::wstring handle) {
    CoInitializeEx(NULL, COINIT_MULTITHREADED);

    g_LogMessage = L"СИНХРОНИЗАЦИЯ С GOOGLE API...";
    InvalidateRect(g_hWnd, NULL, FALSE);

    try {
        std::wstring cleanHandle = handle;
        if (!cleanHandle.empty() && cleanHandle[0] == L'@') {
            cleanHandle = cleanHandle.substr(1);
        }

        std::wstring chUrl = L"/youtube/v3/channels?key=" + API_KEY + L"&forHandle=%40" + cleanHandle + L"&part=contentDetails,statistics";
        std::wstring chRes = WinHttpGetRequest(chUrl);

        if (chRes.find(L"\"totalResults\": 0") != std::wstring::npos || chRes.empty()) {
            g_LogMessage = L"ОШИБКА: КАНАЛ НЕ НАЙДЕН";
            g_isProcessing = false;
            InvalidateRect(g_hWnd, NULL, FALSE);
            CoUninitialize();
            return;
        }

        {
            std::lock_guard<std::mutex> lock(g_DataMutex);
            g_TotalViews = _wtoi64(ExtractJsonValue(chRes, L"viewCount").c_str());
            g_TotalSubscribers = _wtoi64(ExtractJsonValue(chRes, L"subscriberCount").c_str());
            g_TotalVideos = _wtoi64(ExtractJsonValue(chRes, L"videoCount").c_str());
        }

        std::wstring uploadsId = ExtractJsonValue(chRes, L"uploads");
        std::wstring nextPageToken = L"";

        bool isFirstBatch = true;
        int totalLoaded = 0;

        // ПАГИНАЦИЯ И ФОНОВАЯ ЗАГРУЗКА
        do {
            std::wstring plUrl = L"/youtube/v3/playlistItems?key=" + API_KEY + L"&playlistId=" + uploadsId + L"&part=snippet&maxResults=50";
            if (!nextPageToken.empty()) {
                plUrl += L"&pageToken=" + nextPageToken;
            }
            std::wstring plRes = WinHttpGetRequest(plUrl);

            std::vector<std::wstring> videoIds;
            size_t pos = 0;
            while ((pos = plRes.find(L"\"videoId\": \"", pos)) != std::wstring::npos) {
                pos += 12;
                size_t endPos = plRes.find(L"\"", pos);
                videoIds.push_back(plRes.substr(pos, endPos - pos));
            }

            nextPageToken = ExtractJsonString(plRes, L"nextPageToken");
            if (videoIds.empty()) break;

            std::wstring idsQuery = L"";
            for (const auto& id : videoIds) idsQuery += id + L",";
            if (!idsQuery.empty()) idsQuery.pop_back();

            std::wstring stUrl = L"/youtube/v3/videos?key=" + API_KEY + L"&id=" + idsQuery + L"&part=snippet,statistics";
            std::wstring stRes = WinHttpGetRequest(stUrl);

            size_t itemPos = 0;
            int vidIndex = 0;
            std::vector<TitanVideoData> tempBatch;

            while ((itemPos = stRes.find(L"\"kind\": \"youtube#video\"", itemPos)) != std::wstring::npos) {
                size_t nextItem = stRes.find(L"\"kind\": \"youtube#video\"", itemPos + 10);
                std::wstring itemJson = stRes.substr(itemPos, (nextItem == std::wstring::npos) ? stRes.length() - itemPos : nextItem - itemPos);

                TitanVideoData v;
                v.Title = ExtractJsonString(itemJson, L"title");
                v.PublishedAt = ExtractJsonString(itemJson, L"publishedAt");
                v.Views = _wtoi64(ExtractJsonValue(itemJson, L"viewCount").c_str());
                v.Likes = _wtoi64(ExtractJsonValue(itemJson, L"likeCount").c_str());
                v.Comments = _wtoi64(ExtractJsonValue(itemJson, L"commentCount").c_str());

                if (v.Views > 0) v.ER = (double)(v.Likes + v.Comments) / v.Views * 100.0;

                v.DaysLive = CalculateDaysLive(v.PublishedAt);
                v.AvgDailyViews = v.Views / v.DaysLive;
                v.HoverProgress = 0.0f;

                v.VideoId = ExtractJsonString(itemJson, L"id");
                if (v.VideoId.empty() && vidIndex < (int)videoIds.size()) {
                    v.VideoId = videoIds[vidIndex];
                }

                if (!v.VideoId.empty()) {
                    std::wstring thumbPath = L"/vi/" + v.VideoId + L"/mqdefault.jpg";
                    Gdiplus::Image* img = WinHttpDownloadImage(L"i.ytimg.com", thumbPath);
                    if (img) {
                        v.pThumbnail.reset(img);
                    }
                }

                tempBatch.push_back(v);
                itemPos += 10;
                vidIndex++;
            }

            {
                std::lock_guard<std::mutex> lock(g_DataMutex);
                for (const auto& v : tempBatch) {
                    g_VideoData.push_back(v);
                }
            }

            totalLoaded += (int)tempBatch.size();

            // Триггер первой партии (Сразу показываем результат)
            if (isFirstBatch) {
                isFirstBatch = false;
                g_isProcessing = false;
                g_LogMessage = L"ПЕРВЫЕ ВИДЕО ЗАГРУЖЕНЫ. ОСТАЛЬНЫЕ В ФОНЕ...";
            }
            InvalidateRect(g_hWnd, NULL, FALSE);

            if (totalLoaded >= 500) break; // Максимум 500 видео

        } while (!nextPageToken.empty());

        if (isFirstBatch) {
            g_isProcessing = false;
        }

        g_LogMessage = L"АНАЛИЗ УСПЕШНО ЗАВЕРШЕН.";
        InvalidateRect(g_hWnd, NULL, FALSE);
    }
    catch (...) {
        g_LogMessage = L"ФАТАЛЬНАЯ ОШИБКА СЕТИ";
        g_isProcessing = false;
        InvalidateRect(g_hWnd, NULL, FALSE);
    }

    CoUninitialize();
}

// ==============================================================================================
// [MODUL: AI CHAT & POPUP WINDOW]
// ==============================================================================================
LRESULT CALLBACK ChatInputSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
    if (uMsg == WM_CHAR && wParam == VK_RETURN) return 0;
    if (uMsg == WM_KEYDOWN && wParam == VK_RETURN) {
        HWND hParent = GetParent(hWnd);
        SendMessage(hParent, WM_APP + 1, 0, 0);
        return 0;
    }
    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK VideoPopupProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    TitanVideoData* pVideo = (TitanVideoData*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

    switch (uMsg) {
    case WM_CREATE: {
        CREATESTRUCT* pCreate = (CREATESTRUCT*)lParam;
        pVideo = (TitanVideoData*)pCreate->lpCreateParams;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pVideo);

        HWND hChatLog = CreateWindowEx(0, L"EDIT", L"ИИ-Ассистент: Привет, VARVARCHEREPAHOVICH! Я готов обсудить это видео. Спроси про просмотры или вовлеченность...\r\n\r\n",
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL,
            20, 250, 740, 230, hwnd, (HMENU)101, NULL, NULL);

        static HFONT hChatFont = CreateFont(18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, L"Segoe UI");
        SendMessage(hChatLog, WM_SETFONT, (WPARAM)hChatFont, TRUE);

        HWND hChatInput = CreateWindowEx(0, L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
            20, 500, 740, 30, hwnd, (HMENU)102, NULL, NULL);
        SendMessage(hChatInput, WM_SETFONT, (WPARAM)hChatFont, TRUE);
        SetWindowSubclass(hChatInput, ChatInputSubclassProc, 1, 0);

        return 0;
    }
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLOREDIT: {
        HDC hdcEdit = (HDC)wParam;
        SetTextColor(hdcEdit, RGB(0, 122, 255));
        SetBkColor(hdcEdit, RGB(28, 28, 30));
        static HBRUSH hBrush = CreateSolidBrush(RGB(28, 28, 30));
        return (LRESULT)hBrush;
    }
    case WM_APP + 1: {
        if (!pVideo) return 0;

        HWND hChatInput = GetDlgItem(hwnd, 102);
        HWND hChatLog = GetDlgItem(hwnd, 101);
        wchar_t buf[1024];
        GetWindowText(hChatInput, buf, 1024);

        if (wcslen(buf) > 0) {
            std::wstring msg = L"Вы: ";
            msg += buf;
            msg += L"\r\n\r\n";

            int len = GetWindowTextLength(hChatLog);
            SendMessage(hChatLog, EM_SETSEL, len, len);
            SendMessage(hChatLog, EM_REPLACESEL, FALSE, (LPARAM)msg.c_str());
            SetWindowText(hChatInput, L"");

            std::wstring inputLower = buf;
            for (auto& c : inputLower) c = towlower(c);

            std::wstring aiMsg = L"VarAI: ";

            if (inputLower.find(L"просмотр") != std::wstring::npos) {
                aiMsg += L"У этого ролика отличный темп: " + FormatNumber(pVideo->AvgDailyViews) + L" просмотров в день.\r\n\r\n";
            }
            else if (inputLower.find(L"лайк") != std::wstring::npos) {
                aiMsg += L"Активность аудитории высокая. Всего " + FormatNumber(pVideo->Likes) + L" лайков.\r\n\r\n";
            }
            else {
                aiMsg += L"Я проанализировал данные. Это видео успешно удерживает интерес уже " + std::to_wstring(pVideo->DaysLive) + L" дней.\r\n\r\n";
            }

            len = GetWindowTextLength(hChatLog);
            SendMessage(hChatLog, EM_SETSEL, len, len);
            SendMessage(hChatLog, EM_REPLACESEL, FALSE, (LPARAM)aiMsg.c_str());
        }
        return 0;
    }
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        Graphics g(hdc);
        g.SetSmoothingMode(SmoothingModeAntiAlias);
        g.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);
        g.Clear(Color(255, 10, 10, 15));

        if (pVideo) {
            FontFamily fontFamily(L"Segoe UI");
            Font fontTitle(&fontFamily, 22, FontStyleBold, UnitPixel);
            Font fontStats(&fontFamily, 16, FontStyleRegular, UnitPixel);
            Font fontNumbers(&fontFamily, 32, FontStyleBold, UnitPixel);

            SolidBrush textBrush(Color(255, 255, 255, 255));
            SolidBrush mutedBrush(Color(255, 142, 142, 147));
            SolidBrush successBrush(Color(255, 52, 199, 89));

            g.DrawString(L"📊 ИНСПЕКТОР ОБЪЕКТА", -1, &fontTitle, PointF(20.0f, 20.0f), &textBrush);
            std::wstring titleDisp = pVideo->Title.length() > 60 ? pVideo->Title.substr(0, 57) + L"..." : pVideo->Title;
            g.DrawString(titleDisp.c_str(), -1, &fontStats, RectF(20.0f, 65.0f, 750.0f, 60.0f), NULL, &mutedBrush);

            g.DrawString(L"⏳ ДНЕЙ В СЕТИ:", -1, &fontStats, PointF(20.0f, 130.0f), &mutedBrush);
            g.DrawString(std::to_wstring(pVideo->DaysLive).c_str(), -1, &fontNumbers, PointF(20.0f, 155.0f), &textBrush);

            Pen p(Color(255, 0, 122, 255), 2.0f);
            g.DrawLine(&p, 20.0f, 230.0f, 780.0f, 230.0f);
        }
        EndPaint(hwnd, &ps);
        return 0;
    }
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

// ==============================================================================================
// [MODUL: GDI+ GRAPHICS RENDERING]
// ==============================================================================================
void DrawRoundedRect(Graphics& g, const RectF& rect, REAL radius, const Color& fillColor, const Color& borderColor = Color::Transparent) {
    REAL safeRadius = (std::min)(radius, (std::min)(rect.Width / 2.0f, rect.Height / 2.0f));
    if (safeRadius <= 0.0f) safeRadius = 0.1f;
    REAL r2 = safeRadius * 2.0f;

    GraphicsPath path;
    path.AddArc(rect.X, rect.Y, r2, r2, 180.0f, 90.0f);
    path.AddArc(rect.X + rect.Width - r2, rect.Y, r2, r2, 270.0f, 90.0f);
    path.AddArc(rect.X + rect.Width - r2, rect.Y + rect.Height - r2, r2, r2, 0.0f, 90.0f);
    path.AddArc(rect.X, rect.Y + rect.Height - r2, r2, r2, 90.0f, 90.0f);
    path.CloseFigure();

    SolidBrush brush(fillColor);
    g.FillPath(&brush, &path);

    if (borderColor.GetAlpha() > 0) {
        Pen pen(borderColor, 1.5f);
        g.DrawPath(&pen, &path);
    }
}

void DrawProgressBar(Graphics& g, const RectF& rect, double percentage, const Color& color) {
    DrawRoundedRect(g, rect, rect.Height / 2.0f, Color(255, 30, 30, 35));
    if (percentage > 0) {
        REAL w = (REAL)(rect.Width * (percentage / 100.0));
        if (w > rect.Width) w = rect.Width;
        if (w > rect.Height) {
            DrawRoundedRect(g, RectF(rect.X, rect.Y, w, rect.Height), rect.Height / 2.0f, color);
        }
    }
}

void PaintInterface(HDC hdc, int width, int height) {
    HDC memDC = CreateCompatibleDC(hdc);
    HBITMAP memBitmap = CreateCompatibleBitmap(hdc, width, height);
    HBITMAP oldBitmap = (HBITMAP)SelectObject(memDC, memBitmap);

    Graphics g(memDC);
    g.SetSmoothingMode(SmoothingModeAntiAlias);
    g.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);
    g.Clear(ClrBg);

    FontFamily fontFamily(L"Segoe UI");
    Font fontSplashMain(&fontFamily, 60, FontStyleBold, UnitPixel);
    Font fontSplashSub(&fontFamily, 18, FontStyleRegular, UnitPixel);

    Font fontTitle(&fontFamily, 36, FontStyleBold, UnitPixel);
    Font fontSub(&fontFamily, 12, FontStyleBold, UnitPixel);
    Font fontInput(&fontFamily, 18, FontStyleRegular, UnitPixel);
    Font fontBtn(&fontFamily, 16, FontStyleBold, UnitPixel);
    Font fontStats(&fontFamily, 48, FontStyleBold, UnitPixel);
    Font fontLog(&fontFamily, 12, FontStyleBold, UnitPixel);

    SolidBrush textBrush(ClrTextMain);
    SolidBrush accentBrush(ClrAccent);
    SolidBrush mutedBrush(ClrTextMuted);

    REAL centerX = (REAL)width / 2.0f;
    REAL centerY = (REAL)height / 2.0f;

    if (g_animPhase < 3) {
        if (g_animPhase == 0 || g_animPhase == 1) {
            int lenMain = (g_typeIndexMain < (int)splashMainText.length()) ? g_typeIndexMain : (int)splashMainText.length();
            std::wstring typedMain = splashMainText.substr(0, lenMain);
            if (g_typeIndexMain <= splashMainText.length()) typedMain += L"|";

            StringFormat format; format.SetAlignment(StringAlignmentCenter); format.SetLineAlignment(StringAlignmentCenter);
            g.DrawString(typedMain.c_str(), -1, &fontSplashMain, PointF(centerX, centerY - 20.0f), &format, &textBrush);

            if (g_animPhase == 1) {
                int lenSub = (g_typeIndexSub < (int)splashSubText.length()) ? g_typeIndexSub : (int)splashSubText.length();
                std::wstring typedSub = splashSubText.substr(0, lenSub);
                if (g_typeIndexSub <= splashSubText.length()) typedSub += L"|";
                g.DrawString(typedSub.c_str(), -1, &fontSplashSub, PointF(centerX, centerY + 40.0f), &format, &accentBrush);
            }
        }
        else if (g_animPhase == 2) {
            float ease = std::pow(g_animProgress, 4.0f);
            float yOff = ease * 1200.0f;
            StringFormat format; format.SetAlignment(StringAlignmentCenter); format.SetLineAlignment(StringAlignmentCenter);

            for (int i = 0; i < 10; i++) {
                int alpha = 200 - (i * 20);
                if (alpha < 0) alpha = 0;
                float ghostY = centerY - yOff + (yOff * (i / 10.0f) * 0.1f);
                SolidBrush ghostBrush(Color(alpha, 255, 255, 255));
                g.DrawString(splashMainText.c_str(), -1, &fontSplashMain, PointF(centerX, ghostY - 20.0f), &format, &ghostBrush);
            }
            float subY = centerY + 40.0f - yOff;
            SolidBrush subBrush(Color(255, 0, 122, 255));
            g.DrawString(splashSubText.c_str(), -1, &fontSplashSub, PointF(centerX, subY), &format, &subBrush);
        }
    }
    else {
        float ease = 1.0f - std::pow(1.0f - g_animProgress, 4.0f);
        REAL sidebarX = (REAL)(-SIDEBAR_WIDTH + (SIDEBAR_WIDTH * ease));
        REAL viewY = (REAL)height - ((REAL)height * ease);

        // --- БОКОВАЯ ПАНЕЛЬ ---
        RectF sidebarRect(sidebarX, 0.0f, SIDEBAR_WIDTH, (REAL)height);
        DrawRoundedRect(g, sidebarRect, 0.0f, ClrSidebar);

        g.DrawString(L"YouTubeAnalytics ®", -1, &fontTitle, PointF(sidebarX + 35.0f, 45.0f), &textBrush);
        g.DrawString(L"СБОРКА: C++ Classic", -1, &fontSub, PointF(sidebarX + 40.0f, 95.0f), &mutedBrush);

        // ПОЛЕ ВВОДА
        RectF inputBorder(sidebarX + 35.0f, 185.0f, 380.0f, 50.0f);
        DrawRoundedRect(g, inputBorder, 10.0f, Color(255, 30, 30, 35), g_isInputActive ? ClrAccent : Color::Transparent);

        StringFormat vertCenter;
        vertCenter.SetLineAlignment(StringAlignmentCenter);
        vertCenter.SetFormatFlags(StringFormatFlagsNoWrap);
        vertCenter.SetTrimming(StringTrimmingEllipsisCharacter);

        std::wstring displayText = g_InputText;
        if (g_InputText.empty() && !g_isInputActive) {
            g.DrawString(L"Введите @channel...", -1, &fontInput, RectF(sidebarX + 50.0f, 185.0f, 350.0f, 50.0f), &vertCenter, &mutedBrush);
        }
        else {
            if (g_CursorBlink && g_isInputActive) displayText += L"|";
            g.DrawString(displayText.c_str(), -1, &fontInput, RectF(sidebarX + 50.0f, 185.0f, 350.0f, 50.0f), &vertCenter, &textBrush);
        }

        // КНОПКА ЗАПУСКА
        RectF btnRect(sidebarX + 35.0f, 260.0f, 380.0f, 65.0f);
        int r = ClrAccent.GetR() + (int)((ClrAccentHover.GetR() - ClrAccent.GetR()) * g_btnHoverProgress);
        int gr = ClrAccent.GetG() + (int)((ClrAccentHover.GetG() - ClrAccent.GetG()) * g_btnHoverProgress);
        int b = ClrAccent.GetB() + (int)((ClrAccentHover.GetB() - ClrAccent.GetB()) * g_btnHoverProgress);
        Color currentBtnClr = g_isProcessing ? ClrSidebar : Color(255, r, gr, b);

        DrawRoundedRect(g, btnRect, 15.0f, currentBtnClr);
        StringFormat centerFmt; centerFmt.SetAlignment(StringAlignmentCenter); centerFmt.SetLineAlignment(StringAlignmentCenter);
        g.DrawString(g_isProcessing ? L"СИНХРОНИЗАЦИЯ..." : L"ГЛУБОКИЙ АНАЛИЗ", -1, &fontBtn, btnRect, &centerFmt, &textBrush);

        SolidBrush statusBrush(g_isProcessing ? ClrWarning : ClrSuccess);
        if (g_LogMessage.find(L"ОШИБКА") != std::wstring::npos || g_LogMessage.find(L"ФАТАЛЬН") != std::wstring::npos) statusBrush.SetColor(ClrDanger);
        g.DrawString(g_LogMessage.c_str(), -1, &fontLog, RectF(sidebarX + 35.0f, 360.0f, 380.0f, 100.0f), NULL, &statusBrush);

        g.DrawString(L"СОБСТВЕННОСТЬ VARVARCHEREPAHOVICH\nСИСТЕМНЫЙ ПРОГРАММИСТ C++,C,C#,Python,Go,Rust", -1, &fontSub, PointF(sidebarX + 35.0f, (REAL)height - 70.0f), &mutedBrush);

        // --- РАБОЧАЯ ОБЛАСТЬ ---
        REAL vpX = sidebarX + SIDEBAR_WIDTH + 40.0f;
        REAL vpY = viewY + 35.0f;
        REAL vpW = (REAL)width - vpX - 40.0f;

        if (vpW > 100.0f) {
            // Анимация верхней статистики
            float topStatsAnim = std::clamp(g_CardsAnimPhase * 2.0f, 0.0f, 1.0f);
            float topEaseOut = 1.0f - std::pow(1.0f - topStatsAnim, 3.0f);
            REAL statY = vpY + 50.0f * (1.0f - topEaseOut);

            if (topEaseOut > 0.0f) {
                REAL tileW = (vpW - 40.0f) / 3.0f;
                RectF tileV(vpX, statY, tileW, 160.0f);
                RectF tileL(vpX + tileW + 20.0f, statY, tileW, 160.0f);
                RectF tileC(vpX + (tileW + 20.0f) * 2.0f, statY, tileW, 160.0f);

                DrawRoundedRect(g, tileV, 25.0f, ClrCard);
                DrawRoundedRect(g, tileL, 25.0f, ClrCard);
                DrawRoundedRect(g, tileC, 25.0f, ClrCard);

                g.DrawString(L" ВСЕ ПРОСМОТРЫ", -1, &fontSub, PointF(tileV.X + 25.0f, tileV.Y + 25.0f), &mutedBrush);
                g.DrawString(g_TotalViews == 0 ? L"---" : FormatNumber(g_TotalViews).c_str(), -1, &fontStats, PointF(tileV.X + 25.0f, tileV.Y + 65.0f), &accentBrush);

                g.DrawString(L" ПОДПИСЧИКИ", -1, &fontSub, PointF(tileL.X + 25.0f, tileL.Y + 25.0f), &mutedBrush);
                g.DrawString(g_TotalSubscribers == 0 ? L"---" : FormatNumber(g_TotalSubscribers).c_str(), -1, &fontStats, PointF(tileL.X + 25.0f, tileL.Y + 65.0f), &accentBrush);

                g.DrawString(L" ВСЕГО ВИДЕО", -1, &fontSub, PointF(tileC.X + 25.0f, tileC.Y + 25.0f), &mutedBrush);
                g.DrawString(g_TotalVideos == 0 ? L"---" : FormatNumber(g_TotalVideos).c_str(), -1, &fontStats, PointF(tileC.X + 25.0f, tileC.Y + 65.0f), &accentBrush);
            }

            // Неоновая загрузка
            if (g_isProcessing) {
                float availableHeight = height - (vpY + 200.0f) - 20.0f;
                GraphicsState spState = g.Save();
                g.TranslateTransform(vpX + vpW / 2.0f, vpY + 180.0f + availableHeight / 2.0f);

                int alphaGlow = (int)(60 + 60 * sin(g_SpinnerAngle * 2.0f * 3.14159f / 360.0f));
                SolidBrush glowBrush(Color(alphaGlow, 0, 122, 255));
                g.FillEllipse(&glowBrush, -40.0f, -40.0f, 80.0f, 80.0f);

                g.RotateTransform(g_SpinnerAngle);
                Pen spinPen1(Color(255, 0, 122, 255), 4.0f);
                spinPen1.SetLineCap(LineCapRound, LineCapRound, DashCapRound);
                g.DrawArc(&spinPen1, -30.0f, -30.0f, 60.0f, 60.0f, 0.0f, 200.0f);

                g.RotateTransform(-g_SpinnerAngle * 2.5f);
                Pen spinPen2(Color(255, 90, 200, 250), 3.0f);
                spinPen2.SetLineCap(LineCapRound, LineCapRound, DashCapRound);
                g.DrawArc(&spinPen2, -20.0f, -20.0f, 40.0f, 40.0f, 0.0f, 150.0f);

                g.Restore(spState);
            }

            std::lock_guard<std::mutex> lock(g_DataMutex);

            float totalCardsHeight = (int)g_VideoData.size() * 135.0f;
            float availableHeight = height - (vpY + 200.0f) - 20.0f;
            g_MaxScrollY = totalCardsHeight > availableHeight ? totalCardsHeight - availableHeight : 0.0f;

            if (g_TargetScrollY < -g_MaxScrollY) g_TargetScrollY = -g_MaxScrollY;
            if (g_TargetScrollY > 0.0f) g_TargetScrollY = 0.0f;

            Font fCardTitle(&fontFamily, 20, FontStyleBold, UnitPixel);
            Font fCardMeta(&fontFamily, 14, FontStyleRegular, UnitPixel);

            GraphicsState outerClip = g.Save();
            RectF clipListRect(vpX, vpY + 180.0f, vpW + 10.0f, availableHeight + 40.0f);
            g.SetClip(clipListRect);

            for (int i = 0; i < (int)g_VideoData.size(); i++) {
                const auto& v = g_VideoData[i];

                // Каскадная анимация с ограничением
                float delay = (std::min)(i, 25) * 0.03f;
                float cardProgress = (g_CardsAnimPhase - delay) * 4.0f;
                cardProgress = std::clamp(cardProgress, 0.0f, 1.0f);
                float easeOut = 1.0f - std::pow(1.0f - cardProgress, 3.0f);

                if (easeOut <= 0.0f) continue;

                REAL offsetY = 150.0f * (1.0f - easeOut);
                REAL cardY = vpY + 200.0f + g_ScrollY + (i * 135.0f) + offsetY;

                if (cardY + 135.0f > (vpY + 180.0f) && cardY < height) {
                    RectF cRect(vpX, cardY, vpW, 115.0f);

                    float hover = v.HoverProgress;
                    int cr = 28 + (int)((38 - 28) * hover);
                    int cg = 28 + (int)((45 - 28) * hover);
                    int cb = 30 + (int)((60 - 30) * hover);
                    DrawRoundedRect(g, cRect, 20.0f, Color(255, cr, cg, cb));

                    if (hover > 0.01f) {
                        DrawRoundedRect(g, cRect, 20.0f, Color(0, 0, 0, 0), Color((int)(255 * hover), 0, 122, 255));
                    }

                    RectF imgRect(cRect.X + 20.0f, cRect.Y + 20.0f, 75.0f, 75.0f);

                    if (v.pThumbnail && v.pThumbnail->GetLastStatus() == Gdiplus::Ok) {
                        GraphicsPath clipPath;
                        REAL r2 = 15.0f * 2.0f;
                        clipPath.AddArc(imgRect.X, imgRect.Y, r2, r2, 180.0f, 90.0f);
                        clipPath.AddArc(imgRect.X + imgRect.Width - r2, imgRect.Y, r2, r2, 270.0f, 90.0f);
                        clipPath.AddArc(imgRect.X + imgRect.Width - r2, imgRect.Y + imgRect.Height - r2, r2, r2, 0.0f, 90.0f);
                        clipPath.AddArc(imgRect.X, imgRect.Y + imgRect.Height - r2, r2, r2, 90.0f, 90.0f);
                        clipPath.CloseFigure();

                        GraphicsState state = g.Save();
                        g.SetClip(&clipPath, CombineModeIntersect);

                        REAL cropW = 75.0f * (320.0f / 180.0f);
                        REAL cropH = 75.0f;
                        REAL cropX = imgRect.X - (cropW - 75.0f) / 2.0f;
                        REAL cropY = imgRect.Y;

                        g.DrawImage(v.pThumbnail.get(), cropX, cropY, cropW, cropH);

                        g.Restore(state);
                    }
                    else {
                        DrawRoundedRect(g, imgRect, 15.0f, Color(255, 40, 40, 45));
                    }

                    g.DrawString(L"", -1, &fCardTitle, PointF(cRect.X + 41.0f, cRect.Y + 43.0f), &textBrush);
                    g.DrawString(v.Title.c_str(), -1, &fCardTitle, RectF(cRect.X + 115.0f, cRect.Y + 25.0f, cRect.Width - 350.0f, 35.0f), NULL, &textBrush);

                    wchar_t metaBuf[256];
                    swprintf_s(metaBuf, 256, L"Просмотры: %s   •   Лайки: %s   •   Комментарии: %s", FormatNumber(v.Views).c_str(), FormatNumber(v.Likes).c_str(), FormatNumber(v.Comments).c_str());
                    g.DrawString(metaBuf, -1, &fCardMeta, PointF(cRect.X + 115.0f, cRect.Y + 70.0f), &mutedBrush);

                    Color erClr = v.ER > 12.0 ? ClrSuccess : (v.ER > 6.0 ? ClrWarning : ClrDanger);
                    SolidBrush erBrush(erClr);
                    wchar_t erBuf[32]; swprintf_s(erBuf, 32, L"КПД: %.2f%%", v.ER);
                    g.DrawString(erBuf, -1, &fontBtn, PointF(cRect.X + cRect.Width - 220.0f, cRect.Y + 28.0f), &erBrush);
                    double barPercent = (v.ER * 4.0 > 100.0) ? 100.0 : v.ER * 4.0;
                    DrawProgressBar(g, RectF(cRect.X + cRect.Width - 220.0f, cRect.Y + 60.0f, 180.0f, 12.0f), barPercent, erClr);
                }
            }
            g.Restore(outerClip);
        }
    }

    BitBlt(hdc, 0, 0, width, height, memDC, 0, 0, SRCCOPY);
    SelectObject(memDC, oldBitmap);
    DeleteObject(memBitmap);
    DeleteDC(memDC);
}

// ==============================================================================================
// [MODUL: WIN32 MESSAGES & ULTRA-SMOOTH ANIMATION TICKER]
// ==============================================================================================
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CREATE: {
        g_hWnd = hwnd;
        g_LastTime = high_resolution_clock::now();
        SetTimer(hwnd, 1, 10, NULL);
        return 0;
    }
    case WM_SIZE: {
        g_ClientWidth = LOWORD(lParam);
        g_ClientHeight = HIWORD(lParam);
        return 0;
    }
    case WM_CHAR: {
        if (g_animPhase < 3 || g_isProcessing || !g_isInputActive) return 0;

        if (wParam == VK_BACK) {
            if (!g_InputText.empty()) g_InputText.pop_back();
        }
        else if (wParam == VK_RETURN) {
            if (!g_InputText.empty()) {
                g_isProcessing = true;
                g_CardsAnimPhase = 0.0f;
                g_ScrollY = 0.0f;
                g_TargetScrollY = 0.0f;
                {
                    std::lock_guard<std::mutex> lock(g_DataMutex);
                    g_VideoData.clear();
                    g_TotalViews = 0;
                    g_TotalSubscribers = 0;
                    g_TotalVideos = 0;
                }
                std::thread(ExecuteDeepMiningThread, g_InputText).detach();
            }
        }
        // ИСПРАВЛЕНИЕ: Снято ограничение ASCII. Разрешен любой символ, исключая управляющие (wParam >= 32).
        // Лимит увеличен до 40, чтобы помещались любые названия.
        else if (wParam >= 32 && g_InputText.length() < 40) {
            g_InputText += (wchar_t)wParam;
        }
        InvalidateRect(hwnd, NULL, FALSE);
        return 0;
    }
    case WM_TIMER: {
        auto currentTime = high_resolution_clock::now();
        float deltaTime = duration<float>(currentTime - g_LastTime).count();
        g_LastTime = currentTime;

        bool needsRedraw = false;

        if (g_isProcessing) {
            g_SpinnerAngle += 360.0f * deltaTime;
            if (g_SpinnerAngle >= 360.0f) g_SpinnerAngle -= 360.0f;
            needsRedraw = true;
        }

        if (!g_isProcessing && g_CardsAnimPhase < 1.0f) {
            g_CardsAnimPhase += deltaTime * 1.2f;
            if (g_CardsAnimPhase > 1.0f) g_CardsAnimPhase = 1.0f;
            needsRedraw = true;
        }

        {
            std::lock_guard<std::mutex> lock(g_DataMutex);
            for (int i = 0; i < (int)g_VideoData.size(); i++) {
                if (i == g_HoveredCardIndex) {
                    if (g_VideoData[i].HoverProgress < 1.0f) {
                        g_VideoData[i].HoverProgress += deltaTime * 5.0f;
                        if (g_VideoData[i].HoverProgress > 1.0f) g_VideoData[i].HoverProgress = 1.0f;
                        needsRedraw = true;
                    }
                }
                else {
                    if (g_VideoData[i].HoverProgress > 0.0f) {
                        g_VideoData[i].HoverProgress -= deltaTime * 5.0f;
                        if (g_VideoData[i].HoverProgress < 0.0f) g_VideoData[i].HoverProgress = 0.0f;
                        needsRedraw = true;
                    }
                }
            }
        }

        if (g_animPhase == 3 && !g_soundPlayedUI) {
            std::thread([]() {
                int win11Notes[] = { 311, 466, 622, 784, 932 };
                int win11Durations[] = { 100, 100, 120, 150, 300 };
                for (int i = 0; i < 5; i++) {
                    Beep(win11Notes[i], win11Durations[i]);
                    std::this_thread::sleep_for(std::chrono::milliseconds(20));
                }
                }).detach();
            g_soundPlayedUI = true;
        }

        if (g_isInputActive && g_animPhase == 3) {
            g_blinkAccumulator += deltaTime;
            if (g_blinkAccumulator >= 0.5f) {
                g_CursorBlink = !g_CursorBlink;
                g_blinkAccumulator = 0.0f;
                needsRedraw = true;
            }
        }

        if (g_animPhase == 0) {
            g_typeAccumulator += deltaTime;
            if (g_typeAccumulator > 0.04f) {
                g_typeIndexMain++;
                g_typeAccumulator = 0.0f;
                if (g_typeIndexMain > (int)splashMainText.length() + 8) { g_animPhase = 1; g_typeAccumulator = 0.0f; }
            }
            needsRedraw = true;
        }
        else if (g_animPhase == 1) {
            g_typeAccumulator += deltaTime;
            if (g_typeAccumulator > 0.03f) {
                g_typeIndexSub++;
                g_typeAccumulator = 0.0f;
                if (g_typeIndexSub > (int)splashSubText.length() + 15) { g_animPhase = 2; g_animProgress = 0.0f; }
            }
            needsRedraw = true;
        }
        else if (g_animPhase == 2) {
            g_animProgress += deltaTime / 0.8f;
            if (g_animProgress >= 1.0f) { g_animPhase = 3; g_animProgress = 0.0f; }
            needsRedraw = true;
        }
        else if (g_animPhase == 3) {
            if (g_animProgress < 1.0f) {
                g_animProgress += deltaTime / 0.7f;
                if (g_animProgress > 1.0f) g_animProgress = 1.0f;
                needsRedraw = true;
            }
        }

        if (g_isHoveringButton && g_btnHoverProgress < 1.0f) {
            g_btnHoverProgress += deltaTime / 0.15f;
            if (g_btnHoverProgress > 1.0f) g_btnHoverProgress = 1.0f;
            needsRedraw = true;
        }
        else if (!g_isHoveringButton && g_btnHoverProgress > 0.0f) {
            g_btnHoverProgress -= deltaTime / 0.2f;
            if (g_btnHoverProgress < 0.0f) g_btnHoverProgress = 0.0f;
            needsRedraw = true;
        }

        if (std::abs(g_TargetScrollY - g_ScrollY) > 0.5f) {
            g_ScrollY += (g_TargetScrollY - g_ScrollY) * 12.0f * deltaTime;
            needsRedraw = true;
        }

        if (needsRedraw) InvalidateRect(hwnd, NULL, FALSE);
        return 0;
    }
    case WM_MOUSEWHEEL: {
        if (g_animPhase < 3) return 0;
        short zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
        g_TargetScrollY += (zDelta / 120.0f) * 150.0f;

        if (g_TargetScrollY > 0.0f) g_TargetScrollY = 0.0f;
        if (g_TargetScrollY < -g_MaxScrollY) g_TargetScrollY = -g_MaxScrollY;

        InvalidateRect(hwnd, NULL, FALSE);
        return 0;
    }
    case WM_MOUSEMOVE: {
        if (g_animPhase < 3) return 0;
        int x = LOWORD(lParam);
        int y = HIWORD(lParam);

        float ease = 1.0f - (float)std::pow(1.0f - g_animProgress, 4.0f);
        int sidebarX = (int)(-SIDEBAR_WIDTH + (SIDEBAR_WIDTH * ease));
        int vpX = sidebarX + (int)SIDEBAR_WIDTH + 40;
        int vpY = (int)(g_ClientHeight - (g_ClientHeight * ease)) + 35;
        int vpW = g_ClientWidth - vpX - 40;

        bool isInsideBtn = (x >= sidebarX + 35 && x <= sidebarX + 415 && y >= 260 && y <= 325);
        if (isInsideBtn != g_isHoveringButton) {
            g_isHoveringButton = isInsideBtn;
        }

        int oldHover = g_HoveredCardIndex;
        g_HoveredCardIndex = -1;

        if (x >= vpX && x <= vpX + vpW) {
            int cardStartY = vpY + 200 + (int)g_ScrollY;
            std::lock_guard<std::mutex> lock(g_DataMutex);
            for (int i = 0; i < (int)g_VideoData.size(); i++) {
                int cY = cardStartY + i * 135;
                if (y >= cY && y <= cY + 115 && y > vpY + 180) {
                    g_HoveredCardIndex = i;
                    break;
                }
            }
        }

        if (oldHover != g_HoveredCardIndex || isInsideBtn != g_isHoveringButton) {
            SetCursor(LoadCursor(NULL, (g_isHoveringButton || g_HoveredCardIndex != -1) ? IDC_HAND : IDC_ARROW));
        }

        return 0;
    }
    case WM_LBUTTONDOWN: {
        if (g_animPhase < 3) return 0;
        int x = LOWORD(lParam);
        int y = HIWORD(lParam);
        float ease = 1.0f - (float)std::pow(1.0f - g_animProgress, 4.0f);
        int sidebarX = (int)(-SIDEBAR_WIDTH + (SIDEBAR_WIDTH * ease));

        if (x >= sidebarX + 35 && x <= sidebarX + 415 && y >= 185 && y <= 235) {
            g_isInputActive = true;
        }
        else {
            g_isInputActive = false;
        }

        if (g_isHoveringButton && !g_isProcessing) {
            if (!g_InputText.empty()) {
                g_isProcessing = true;
                g_CardsAnimPhase = 0.0f;
                g_ScrollY = 0.0f;
                g_TargetScrollY = 0.0f;
                {
                    std::lock_guard<std::mutex> lock(g_DataMutex);
                    g_VideoData.clear();
                    g_TotalViews = 0;
                    g_TotalSubscribers = 0;
                    g_TotalVideos = 0;
                }
                std::thread(ExecuteDeepMiningThread, g_InputText).detach();
            }
            else {
                g_LogMessage = L"ОШИБКА: ПУСТОЙ ВВОД";
            }
        }

        if (g_HoveredCardIndex != -1) {
            std::lock_guard<std::mutex> lock(g_DataMutex);
            if (g_HoveredCardIndex < (int)g_VideoData.size()) {
                CreateWindowEx(WS_EX_TOOLWINDOW, L"VarVideoClass", L"Аналитика видео и ИИ-Ассистент",
                    WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
                    hwnd, NULL, GetModuleHandle(NULL), (LPVOID)&g_VideoData[g_HoveredCardIndex]);
            }
        }

        InvalidateRect(hwnd, NULL, FALSE);
        return 0;
    }
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        PaintInterface(hdc, g_ClientWidth, g_ClientHeight);
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_DESTROY: {
        PostQuitMessage(0);
        return 0;
    }
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

// ==============================================================================================
// [MODUL: ENTRY_POINT] (WinMain C++)
// ==============================================================================================
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    const wchar_t CLASS_NAME[] = L"VarWindowClass";
    WNDCLASS wc = { };
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = CreateSolidBrush(RGB(10, 10, 10));
    RegisterClass(&wc);

    const wchar_t POPUP_CLASS_NAME[] = L"VarVideoClass";
    WNDCLASS pwc = { };
    pwc.lpfnWndProc = VideoPopupProc;
    pwc.hInstance = hInstance;
    pwc.lpszClassName = POPUP_CLASS_NAME;
    pwc.hCursor = LoadCursor(NULL, IDC_ARROW);
    pwc.hbrBackground = CreateSolidBrush(RGB(18, 18, 18));
    RegisterClass(&pwc);

    HWND hwnd = CreateWindowEx(0, CLASS_NAME, L"YouTubeAnalytics ®",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, CW_USEDEFAULT, CW_USEDEFAULT, 1400, 900,
        NULL, NULL, hInstance, NULL);

    if (hwnd == NULL) return 0;
    ShowWindow(hwnd, nCmdShow);

    MSG msg = { };
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    GdiplusShutdown(gdiplusToken);
    return 0;
}