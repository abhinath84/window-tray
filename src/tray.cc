#include "tray.h"
#include <assert.h>
#include "utils/node_async_call.h"
#include <iostream>

#define CHECK_RESULT(result)                                    \
    do                                                          \
    {                                                           \
        if (!result)                                            \
        {                                                       \
            std::cerr << "err:" << GetLastError() << std::endl; \
        }                                                       \
    } while (0)

static const UINT g_WndMsgTaskbarCreated = RegisterWindowMessageW(L"TaskbarCreated");

#define TRAY_WINDOW_MESSAGE (WM_USER + 100)
#define WAKEUP_MESSAGE (WM_USER + 101)
#define WM_SHOW_POPUP_MESSAGE (WM_USER + 102)

static UINT nextTrayIconId()
{
    static UINT next_id = 2;
    return next_id++;
}

HICON getIconHandle(const std::wstring &iconPath)
{
    return (HICON)(HICON)::LoadImage(NULL, iconPath.c_str(), IMAGE_ICON, 0, 0, LR_LOADFROMFILE);
}

int popupMenuResult = -1;
HMENU popupMenu = NULL;

//////////////////////////////////////////////////////////////////////////

LONG myFunc(LPEXCEPTION_POINTERS p)
{
     printf("Exit!!!\n");     
     return EXCEPTION_EXECUTE_HANDLER;
}

//////////////////////////////////////////////////////////////////////////

class DummyWindow
{
  public:
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        NodeTray *pThis = nullptr;

//	printf("WindowProc: %d\n", uMsg);

        if (uMsg == WM_NCCREATE)
        {
            printf("Setup exit\n");     
            SetUnhandledExceptionFilter((LPTOP_LEVEL_EXCEPTION_FILTER)&myFunc);    

            CREATESTRUCT *pCreate = (CREATESTRUCT *)lParam;
            pThis = (NodeTray *)pCreate->lpCreateParams;
            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pThis);
        }
        else
        {
            pThis = (NodeTray *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
        }
        if (pThis)
        {
            return pThis->_windowProc(hwnd, uMsg, wParam, lParam);
        }
        else
        {
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
        }
    }
};

class WindowClassRegister
{
  public:
    WindowClassRegister() { ; }
    ~WindowClassRegister()
    {
        for (std::vector<std::wstring>::iterator i = m_windowClass.begin(); i != m_windowClass.end(); ++i)
        {
            ::UnregisterClassW(i->c_str(), nullptr);
        }
    }

    static WindowClassRegister &instance()
    {
        static WindowClassRegister rr;
        return rr;
    }

    bool registerWindowClass(const std::wstring &cls)
    {
        WNDCLASSEX wc = {0};
        wc.cbSize = sizeof(WNDCLASSEX);
        wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
        wc.lpfnWndProc = DummyWindow::WindowProc;
        wc.hInstance = GetModuleHandle(nullptr);
        wc.lpszClassName = cls.c_str();
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);

        ATOM r = RegisterClassEx(&wc);
        assert(r && "registerClass");
        return !!r;
    }

    bool registerClass(const std::wstring &cls)
    {
        if (isRegistered(cls))
        {
            return true;
        }

        if (registerWindowClass(cls))
        {
            m_windowClass.push_back(cls);
            return true;
        }
        else
            return false;
    }

    bool isRegistered(const std::wstring &cls)
    {
        return std::find(m_windowClass.begin(), m_windowClass.end(), cls) != m_windowClass.end();
    }

  private:
    std::vector<std::wstring> m_windowClass;
};

//////////////////////////////////////////////////////////////////////////

Napi::FunctionReference NodeTray::constructor_;

Napi::Object NodeTray::Init(Napi::Env env, Napi::Object exports)
{
    Napi::HandleScope scope(env);

    Napi::Function func = DefineClass(env, "NodeTray", {
                                                           NAPI_METHOD_INSTANCE(NodeTray, destroy),
                                                           NAPI_METHOD_INSTANCE(NodeTray, setIcon),
                                                           NAPI_METHOD_INSTANCE(NodeTray, setToolTip),
//                                                           NAPI_METHOD_INSTANCE(NodeTray, getBounds),
                                                           NAPI_METHOD_INSTANCE(NodeTray, toggleWindow),
                                                           NAPI_METHOD_INSTANCE(NodeTray, isWindowVisible),
                                                           NAPI_METHOD_INSTANCE(NodeTray, isWindowMinimized),
                                                           NAPI_METHOD_INSTANCE(NodeTray, balloon),
                                                           NAPI_METHOD_INSTANCE(NodeTray, showPopup),
                                                       });

    constructor_ = Napi::Persistent(func);
    constructor_.SuppressDestruct();

    exports.Set("NodeTray", func);
    return exports;
}

NodeTray::NodeTray(const Napi::CallbackInfo &info)
    : Napi::ObjectWrap<NodeTray>(info)
    , id_(nextTrayIconId())
{
    wrapper_ = Napi::Weak(info.This().ToObject());
    iconPath_ = utils::fromUtf8(info[0].ToString());
    start();
}

NodeTray::~NodeTray()
{
    stop();
}

Napi::Value NodeTray::destroy(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    stop();
    return env.Undefined();
}

Napi::Value NodeTray::setIcon(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    std::string icon = info[0].ToString();

    updateIcon(icon);

    return env.Undefined();
}

Napi::Value NodeTray::setToolTip(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    std::string tip = info[0].ToString();

    updateToolTip(tip);
    return env.Undefined();
}

//Napi::Value NodeTray::getBounds(const Napi::CallbackInfo &info)
//{
//    Napi::Env env = info.Env();
//
//    RECT rect = getRect();
//
//    Napi::Object obj = Napi::Object::New(env);
//
//    obj.Set("x", Napi::Value::From(env, rect.left));
//    obj.Set("y", Napi::Value::From(env, rect.top));
//    obj.Set("width", Napi::Value::From(env, rect.right - rect.left));
//    obj.Set("height", Napi::Value::From(env, rect.bottom - rect.top));
//
//    return obj;
//}

Napi::Value NodeTray::toggleWindow(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    std::string windowTitle = info[0].ToString();

    HWND hWnd = findWindowByName(windowTitle);

    bool value = false;

	if (hWnd != NULL)
	{
        if (IsWindowVisible(hWnd) == FALSE)
        {
            value = ShowWindow(hWnd, SW_SHOWNORMAL);
            SetForegroundWindow(hWnd);
        }
        else
        {
            value = ShowWindow(hWnd, SW_HIDE);
        }
	}

    return Napi::Boolean::New(env, !value);
}

Napi::Value NodeTray::isWindowVisible(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    std::string windowTitle = info[0].ToString();

    HWND hWnd = findWindowByName(windowTitle);

    bool value = false;

	if (hWnd != NULL)
	{
        value = IsWindowVisible(hWnd);
	}

    return Napi::Boolean::New(env, value);
}

Napi::Value NodeTray::isWindowMinimized(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    std::string windowTitle = info[0].ToString();

    HWND hWnd = findWindowByName(windowTitle);

    bool value = false;

	if (hWnd != NULL)
	{
    	WINDOWPLACEMENT pl;
    	bool isSuccess = GetWindowPlacement(hWnd, &pl);

        value = IsWindowVisible(hWnd) == TRUE && pl.showCmd == SW_SHOWMINIMIZED;
	}

    return Napi::Boolean::New(env, value);
}

Napi::Value NodeTray::balloon(const Napi::CallbackInfo &info)
{
//    ETooltipIcon icon = ETooltipIcon.eTI_Info;

    Napi::Env env = info.Env();

    std::string title = info[0].ToString();
    std::string text = info[1].ToString();
    int timeout = info[1].ToNumber().Int32Value();

    if (!timeout)
    {
        timeout = 5000;
    }

#ifndef NOTIFYICONDATA_V2_SIZE
	return Napi::Boolean::New(env, false);
#else

	NOTIFYICONDATAA data;

    memset(&data, 0, sizeof(data));
    // the basic functions need only V1
#ifdef NOTIFYICONDATA_V1_SIZE
    data.cbSize = NOTIFYICONDATA_V1_SIZE;
#else
    data.cbSize = sizeof(data);
#endif
    data.hWnd = window_;
    assert(data.hWnd);
    data.uID = id_;

	data.uVersion = NOTIFYICON_VERSION_4;
	data.cbSize = NOTIFYICONDATAA_V2_SIZE; // win2k and later
	data.uFlags = NIF_INFO;
	data.dwInfoFlags = eTI_Info;
	data.uTimeout = timeout; // deprecated as of Windows Vista, it has a min(10000) and max(30000) value on previous Windows versions.
	data.uCallbackMessage  = NIN_BALLOONUSERCLICK;

	strcpy_s(data.szInfoTitle, title.c_str());
	strcpy_s(data.szInfo, text.c_str());

    return Napi::Boolean::New(env, FALSE != Shell_NotifyIconA(NIM_MODIFY, &data));
#endif

}

HMENU createPopupMenu(Napi::Array& array)
{
    HMENU menu = CreatePopupMenu();

    for (UINT i = 0; i < array.Length(); i++)
    {
        Napi::Object item = array.Get(i).As<Napi::Object>();

        INT itemId = item.Get("id").ToNumber();
        std::string title = item.Get("title").ToString();

        int menuType = MF_STRING;

        std::string::size_type loc = title.find( "---", 0 );
        if( loc != std::string::npos )
        {
            menuType = MF_SEPARATOR;
        }

        AppendMenuW(menu, menuType, itemId, utils::fromUtf8(title).c_str());
//        printf("Item id = %d, title = %s\n", itemId, title.c_str());
    }

    return menu;
}

int showPopupWindow(HMENU menu, HWND hWnd)
{
    INT result = -1;

    POINT pt;
    if (GetCursorPos(&pt))
    {
        SetForegroundWindow(hWnd);
        result = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_RIGHTBUTTON, pt.x, pt.y, 0, hWnd, NULL);
//        printf("TrackPopupMenu done, result = %d\n", result);

        PostMessage(hWnd, WM_NULL, 0, 0);
//        printf("PostMessage done\n");
    }

    DestroyMenu(menu);
//    printf("DestroyMenu done\n");

    return result;
}

class MyWorker : public Napi::AsyncWorker {
    public:
        MyWorker(Napi::Function& callback)
            : Napi::AsyncWorker(callback), result(-1) {
        }

        ~MyWorker() {
        }

        void Execute() {

            while(popupMenu != NULL)
            {
//                printf("Inside Async Call\n");
                Sleep(100);
            }

            result = popupMenuResult;

        }

        void OnOK() {

            Napi::HandleScope scope(Env());
            Callback().Call({Env().Undefined(), Napi::Number::New(Env(), result)});
        }

    private:
        int result;

};


Napi::Value NodeTray::showPopup(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    Napi::Array array = info[0].As<Napi::Array>();
    Napi::Function cb = info[1].As<Napi::Function>();
    popupMenu = createPopupMenu(array);

    PostMessage(window_, TRAY_WINDOW_MESSAGE, 0, WM_SHOW_POPUP_MESSAGE);

    Napi::AsyncWorker& worker = *new MyWorker(cb);

    worker.Queue();

    return env.Undefined();
}

LRESULT NodeTray::_windowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (TRAY_WINDOW_MESSAGE == uMsg)
    {
        switch (LOWORD(lParam))
        {
            case WM_LBUTTONDOWN:
            {
                node_async_call::async_call([this]() {
                    this->onClicked();
                });
                break;
            }

            case WM_RBUTTONUP:
            {
                node_async_call::async_call([this]() {
                    this->onRightClicked();
                });
                break;
            }

            case NIN_BALLOONUSERCLICK:
            {
                node_async_call::async_call([this]() {
                    this->balloonClicked();
                });
                break;
            }

            case WM_CONTEXTMENU:
            {
//                printf("TRAY_WINDOW_MESSAGE: WM_CONTEXTMENU\n");

                break;
            }

            case WM_SHOW_POPUP_MESSAGE:
            {
//                printf("TRAY_WINDOW_MESSAGE: WM_SHOW_POPUP_MESSAGE\n");
                popupMenuResult = showPopupWindow(popupMenu, window_);
                popupMenu = NULL;

                break;
            }


//                _handleClick(
//                    (lParam == WM_LBUTTONDOWN || lParam == NIN_BALLOONUSERCLICK || lParam == WM_LBUTTONDBLCLK),
//                    (lParam == WM_LBUTTONDBLCLK || lParam == WM_RBUTTONDBLCLK));
//                return TRUE;
    //        case WM_MOUSEMOVE:
            case WM_LBUTTONDBLCLK:
            case WM_RBUTTONDBLCLK:
    //            _handleMouseMove();
                break;
            default:
                break;
        }

        return TRUE;
    }

    if (g_WndMsgTaskbarCreated == uMsg)
    {
        addTrayIcon();
        return TRUE;
    }

//    if (uMsg == WM_TIMER)
//    {
//        _checkMouseLeave();
//    }

    return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
}

//void NodeTray::_handleClick(bool leftClick, bool doubleClick)
//{
//    if (leftClick)
//    {
//        if (doubleClick)
//        {
//            node_async_call::async_call([this]() {
//                this->onDoubleClicked();
//            });
//        }
//        else
//        {
//            node_async_call::async_call([this]() {
//                this->onClicked();
//            });
//        }
//
//        return;
//    }
//    else if (!doubleClick)
//    {
//        node_async_call::async_call([this]() {
//            this->onRightClicked();
//        });
//    }
//}

//void NodeTray::_handleMouseMove()
//{
//    GetCursorPos(&cursor_);
//    if (!mouseEntered_)
//    {
//        RECT rect = getRect();
//        if (cursor_.x < rect.left || cursor_.x > rect.right || cursor_.y < rect.top || cursor_.y > rect.bottom)
//        {
//            return;
//        }
//        else
//        {
//            mouseEntered_ = true;
//            node_async_call::async_call([this]() {
//                this->onMouseEnter();
//            });
//        }
//    }
//}

//void NodeTray::_checkMouseLeave()
//{
//    if (mouseEntered_)
//    {
//        POINT cursor;
//        GetCursorPos(&cursor);
//        RECT rect = getRect();
//        if (cursor.x < rect.left || cursor.x > rect.right || cursor.y < rect.top || cursor.y > rect.bottom)
//        {
//            mouseEntered_ = false;
//            node_async_call::async_call([this]() {
//                this->onMouseLeave();
//            });
//        }
//    }
//}

//void NodeTray::onMouseEnter()
//{
//    this->emit("mouse-enter");
//}

//void NodeTray::onMouseLeave()
//{
//    this->emit("mouse-leave");
//}

//void NodeTray::onDoubleClicked()
//{
//    this->emit("double-click");
//}

void NodeTray::onClicked()
{
    this->emit("click");
}

void NodeTray::onRightClicked()
{
    this->emit("right-click");
}

void NodeTray::balloonClicked()
{
    this->emit("balloon-click");
}

void NodeTray::start()
{
    HANDLE ready = CreateEventW(nullptr, true, false, nullptr);

    worker_ = std::make_shared<std::thread>([this, ready]() {
        _createMessageWindow();

        DWORD timer = ::SetTimer(window_, 0, 200, nullptr);

        SetEvent(ready);

        MSG msg;
        while (GetMessage(&msg, NULL, 0, 0))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        KillTimer(window_, timer);

        _destroyWindow();
    });

    WaitForSingleObject(ready, INFINITE);
    CloseHandle(ready);

    loadIcon();
    addTrayIcon();
}

void NodeTray::stop()
{
    if (worker_)
    {
        destroyTrayIcon();
        destroyIcon();

        PostThreadMessage(GetThreadId(worker_->native_handle()), WM_QUIT, NULL, NULL);
        worker_->join();
        worker_.reset();
    }
}

void NodeTray::_createMessageWindow()
{
    static const WCHAR windowclass[] = L"node_trayicon_msg_window_2a0b1c8d";

    WindowClassRegister::instance().registerClass(windowclass);

    HINSTANCE hInstance = (HINSTANCE)GetModuleHandle(NULL);

    WNDCLASSEXW wc;
    wc.cbSize = sizeof(wc);
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon = LoadIcon(NULL, IDI_WINLOGO);
    wc.hIconSm = NULL;
    wc.hInstance = hInstance;
    wc.lpfnWndProc = DummyWindow::WindowProc;
    wc.lpszClassName = windowclass;
    wc.lpszMenuName = NULL;
    wc.style = 0;

    window_ = CreateWindowExW(
        0,
        windowclass,
        L"trayicon_msg_window",
        WS_POPUP,
        0, 0, 0, 0,
        NULL,
        NULL,
        hInstance,
        this);
}

void NodeTray::_destroyWindow()
{
    if (window_)
    {
        DestroyWindow(window_);
        window_ = nullptr;
    }
}

void NodeTray::loadIcon()
{
    destroyIcon();
    icon_ = getIconHandle(iconPath_);
}

void NodeTray::destroyIcon()
{
    if (icon_)
    {
        ::DestroyIcon(icon_);
        icon_ = nullptr;
    }
}

bool NodeTray::addTrayIcon()
{
    NOTIFYICONDATAW icon_data = {0};
    getInitializedNCD(icon_data);

    icon_data.uFlags = NIF_ICON | NIF_MESSAGE;
    icon_data.hIcon = icon_;
    icon_data.uCallbackMessage = TRAY_WINDOW_MESSAGE;
    BOOL result = Shell_NotifyIcon(NIM_ADD, &icon_data);
    return !!result;
}

bool NodeTray::destroyTrayIcon()
{
    NOTIFYICONDATAW icon_data = {0};
    getInitializedNCD(icon_data);
    BOOL result = Shell_NotifyIcon(NIM_DELETE, &icon_data);
    CHECK_RESULT(result);
    return !!result;
}

bool NodeTray::updateIcon(const std::string &icon)
{
    iconPath_ = utils::fromUtf8(icon);
    loadIcon();

    NOTIFYICONDATAW icon_data = {0};
    getInitializedNCD(icon_data);
    icon_data.uFlags |= NIF_ICON;
    icon_data.hIcon = icon_;

    BOOL result = Shell_NotifyIconW(NIM_MODIFY, &icon_data);
    CHECK_RESULT(result);
    return !!result;
}

bool NodeTray::updateToolTip(const std::string &tip)
{
    NOTIFYICONDATAW icon_data = {0};
    getInitializedNCD(icon_data);
    icon_data.uFlags |= NIF_TIP;
    wcsncpy_s(icon_data.szTip, utils::fromUtf8(tip).c_str(), _TRUNCATE);
    BOOL result = Shell_NotifyIcon(NIM_MODIFY, &icon_data);
    CHECK_RESULT(result);
    return !!result;
}

//RECT NodeTray::getRect()
//{
//    NOTIFYICONIDENTIFIER icon_id;
//    memset(&icon_id, 0, sizeof(NOTIFYICONIDENTIFIER));
//    icon_id.uID = id_;
//    icon_id.hWnd = window_;
//    icon_id.cbSize = sizeof(NOTIFYICONIDENTIFIER);
//    RECT rect = {0};
//    Shell_NotifyIconGetRect(&icon_id, &rect);
//
//    return rect;
//}

HICON NodeTray::getIcon()
{
    return icon_;
}

void NodeTray::getInitializedNCD( NOTIFYICONDATAW &ncd)
{
    ncd.cbSize = sizeof(NOTIFYICONDATAW);
    ncd.hWnd = window_;
    ncd.uID = id_;
}


static HWND findWindowByName(std::string name)
{
	HWND hWnd = FindWindow(NULL, utils::fromUtf8(name).c_str());

//	printf("Pointer of %s %p\n", name.c_str(), hWnd);

	return hWnd;
}
