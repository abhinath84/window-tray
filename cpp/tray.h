#pragma once
#include <napi.h>
#include <memory>
#include <thread>
#include <functional>
#include <unordered_map>
#include "utils/n-utils.h"
#include "utils/win-utils.h"
#include <shellapi.h>

class NodeTray : public Napi::ObjectWrap<NodeTray>
{
    static Napi::FunctionReference constructor_;

    std::wstring iconPath_;
    HICON icon_ = nullptr;
    UINT id_ = 0;
    std::shared_ptr<std::thread> worker_;
    HWND window_ = nullptr;
    POINT cursor_;
    bool mouseEntered_ = false;

    template <class... Arg>
    void emit(const std::string &eventName, const Arg &... args)
    {
            Napi::HandleScope scope(wrapper_.Env());

            Napi::Object wrapper = wrapper_.Value();
            Napi::Function emiiter = wrapper.Get("emit").As<Napi::Function>();
            emiiter.MakeCallback(wrapper, {Napi::String::New(wrapper.Env(), eventName), Napi::Value::From(wrapper.Env(), args)...});
    }

    Napi::ObjectReference wrapper_;


  public:
    static Napi::Object Init(Napi::Env env, Napi::Object exports);

    NodeTray(const Napi::CallbackInfo &info);
    ~NodeTray();

    Napi::Value destroy(const Napi::CallbackInfo &info);
    Napi::Value setIcon(const Napi::CallbackInfo &info);
    Napi::Value setToolTip(const Napi::CallbackInfo &info);
//    Napi::Value getBounds(const Napi::CallbackInfo &info);

    Napi::Value toggleWindow(const Napi::CallbackInfo &info);
    Napi::Value isWindowVisible(const Napi::CallbackInfo &info);
    Napi::Value isWindowMinimized(const Napi::CallbackInfo &info);
    Napi::Value balloon(const Napi::CallbackInfo &info);
    Napi::Value showPopup(const Napi::CallbackInfo &info);

    LRESULT _windowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

//    void _handleClick(bool left_mouse_click, bool double_button_click);
//    void _handleMouseMove();
//    void _checkMouseLeave();

//    void onMouseEnter();
//    void onMouseLeave();

//    void onDoubleClicked();
    void onClicked();
    void onRightClicked();
    void balloonClicked();

    void start();
    void stop();

    void _createMessageWindow();
    void _destroyWindow();

    void loadIcon();
    void destroyIcon();

    bool addTrayIcon();
    bool destroyTrayIcon();
    bool updateIcon(const std::string &icon);
    bool updateToolTip(const std::string &tip);

//    RECT getRect();

    HICON getIcon();

    void getInitializedNCD(NOTIFYICONDATAW &ncd);

};

static HWND findWindowByName(std::string name);

enum ETooltipIcon
{
    eTI_None,			// NIIF_NONE(0)
    eTI_Info,			// NIIF_INFO(1)
    eTI_Warning,		// NIIF_WARNING(2)
    eTI_Error			// NIIF_ERROR(3)
};
