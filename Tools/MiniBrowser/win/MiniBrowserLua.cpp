// MiniBrowserLua.cpp — single-file LuaJIT control bridge for WebKit WinCairo MiniBrowser.
// Modern WebKit2 C API (WebKit2_C.h + WKPreferencesRefPrivate.h). Links LuaJIT 2.0.5 (Lua 5.1).
// Default-off: only engaged when the process is started with "--lua=<path.lua>".
/*
 * Integration (minimal, 3 touch-points in the tree):
 *   1) webkit-build: link LuaJIT (set env LUAJIT_DIR), add this file to MiniBrowser_SOURCES.
 *   2) WebKitBrowserWindow.cpp : after clients are set, call
 *        extern "C" void MiniBrowserLuaInit(void* page, void* hwnd);
 *        extern "C" void MiniBrowserLuaOnEvent(const char* type, const char* json);
 *        extern "C" void MiniBrowserLuaShutdown();
 *      (init once at page creation; forward title/url/load events via MiniBrowserLuaOnEvent;
 *       call Shutdown on window close)
 * Lua API (global table "minibrowser", alias "mb"):
 *   nav(url)  back()  forward()  reload()  stop()
 *   url()  title()  contents()
 *   eval(js) -> decoded value      execute(js) -> json string
 *   user_agent()  set_user_agent(s)
 *   zoom(f)  zoom_factor()  zoom_reset()
 *   script_enabled(b?)  devtools_enabled(b?)  images_enabled(b?)
 *   features() -> {{key,name,on,default}}     set_feature(key, on)
 *   pump_events()  on(type, fn)  quit()
 */
#include "stdafx.h"
#include <windows.h>
#include <WebKit/WebKit2_C.h>
#include <WebKit/WKPreferencesRefPrivate.h>
#include <WebKit/WKRetainPtr.h>

#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <condition_variable>
#include <thread>

#ifndef MINIBROWSER_LUAJIT
// LuaJIT not in this build: no-op entry points keep the tree linkable without Lua.
extern "C" int  MiniBrowserLuaInit(void* page, void* hwnd) { (void)page; (void)hwnd; return 0; }
extern "C" void MiniBrowserLuaOnEvent(const char* t, const char* j) { (void)t; (void)j; }
extern "C" void MiniBrowserLuaShutdown(void) {}
#else
#include <lua.hpp>
#endif

using WK::WKRetainPtr;
using WK::adoptWK;

namespace {
    // ---- generic helpers ----
    std::string utf8FromWide(const wchar_t* s, int len) {
        if (!s) return {};
        int n = WideCharToMultiByte(CP_UTF8, 0, s, len, nullptr, 0, nullptr, nullptr);
        std::string out(n, '\0');
        if (n) WideCharToMultiByte(CP_UTF8, 0, s, len, &out[0], n, nullptr, nullptr);
        return out;
    }
    std::string cmdlineArgValue(const char* key) {
        // look for --key=value in GetCommandLineW
        std::wstring k = L"--" + std::wstring(key, key + strlen(key)) + L"=";
        std::wstring cl = GetCommandLineW();
        size_t p = cl.find(k);
        if (p == std::wstring::npos) return {};
        std::wstring v = cl.substr(p + k.size());
        size_t e = v.find(L' ');
        if (e != std::wstring::npos) v = v.substr(0, e);
        return utf8FromWide(v.c_str(), (int)v.size());
    }

    std::string wkStringToUTF8(WKStringRef s) {
        if (!s) return {};
        size_t len = WKStringGetLength(s);
        std::string out(len + 1, '\0');
        size_t n = WKStringGetUTF8CString(s, &out[0], out.size());
        out.resize(n);
        return out;
    }
    WKRetainPtr<WKStringRef> makeWKString(const std::string& s) {
        return adoptWK(WKStringCreateWithUTF8CString(s.c_str()));
    }
    WKRetainPtr<WKURLRef> makeWKURL(const std::string& s) {
        return adoptWK(WKURLCreateWithUTF8CString(s.c_str()));
    }
    std::string wkURLToUTF8(WKURLRef u) {
        if (!u) return {};
        return wkStringToUTF8(WKURLCopyString(u));
    }

    // job kind enum
    enum JobKind {
        J_NAV, J_BACK, J_FORWARD, J_RELOAD, J_STOP,
        J_URL, J_TITLE, J_CONTENTS,
        J_EVAL, J_EXEC_SYNC,
        J_SET_UA, J_GET_UA,
        J_ZOOM, J_GET_ZOOM, J_ZOOM_RESET,
        J_SCRIPT_ENABLED, J_DEVTOOLS, J_IMAGES,
        J_FEATURES, J_SET_FEATURE,
        J_QUIT
    };

    struct Job {
        int id = 0;
        JobKind kind = J_NAV;
        std::string s;   // url / script / ua string / feature key
        double d = 0;    // zoom factor / bool-as-double
        bool done = false;
        bool ok = false;
        std::string result;     // textual result
        std::string error;
    };

    struct Bridge {
        void* pageRaw = nullptr;      // WKPageRef
        HWND mainHwnd = nullptr;
        HWND msgWnd = nullptr;        // hidden message-only window (created on main/UI thread)
        bool enabled = false;

        std::mutex mtx;
        std::condition_variable cv;
        int nextId = 1;
        std::map<int, Job*> pending;

        std::thread luaThread;

        // Lua state (used only from luaThread)
        lua_State* L = nullptr;

        // event queue (main thread -> lua thread)
        std::mutex evMtx;
        std::condition_variable evCv;
        std::vector<std::string> events;    // "type\tjson"
        bool quitting = false;

        // returns true if a stale Lua result was captured
        std::string lastEvalJson;
    } g_bridge;

    // forward
    bool executeJob(Job& job);
    void runLuaMain(Bridge&);
    void enableLuaIfRequested();
}

namespace {
    const UINT WM_LUA_JOB = WM_APP + 0x311;

    void signalDone(Bridge& b, Job& job) {
        std::lock_guard<std::mutex> lk(b.mtx);
        job.done = true;
        b.cv.notify_all();
    }

    std::string wkTypeToString(WKTypeRef r) {
        if (!r) return "null";
        if (WKGetTypeID(r) == WKStringGetTypeID())
            return wkStringToUTF8((WKStringRef)r);
        if (WKGetTypeID(r) == WKBooleanGetTypeID())
            return WKBooleanGetValue((WKBooleanRef)r) ? "true" : "false";
        if (WKGetTypeID(r) == WKDoubleGetTypeID()) {
            char buf[64]; snprintf(buf, sizeof buf, "%.17g", WKDoubleGetValue((WKDoubleRef)r)); return buf;
        }
        if (WKGetTypeID(r) == WKUInt64GetTypeID()) {
            char buf[32]; snprintf(buf, sizeof buf, "%llu", (unsigned long long)WKUInt64GetValue((WKUInt64Ref)r)); return buf;
        }
        return "null";
    }

    void evalCb(WKTypeRef result, WKErrorRef, void* ctx) {
        Job* job = (Job*)ctx;
        job->ok = true;
        job->result = wkTypeToString(result);
        signalDone(g_bridge, *job);
    }
    void contentsCb(WKStringRef text, WKErrorRef, void* ctx) {
        Job* job = (Job*)ctx;
        job->ok = true;
        job->result = text ? wkStringToUTF8(text) : "";
        signalDone(g_bridge, *job);
    }

    // Returns true if the job completed synchronously (caller signals done),
    // false if completion happens in an async callback.
    bool executeJob(Job& job) {
        Bridge& b = g_bridge;
        WKPageRef page = (WKPageRef)b.pageRaw;
        bool sync = true;
        switch (job.kind) {
        case J_NAV:        WKPageLoadURL(page, makeWKURL(job.s).get()); break;
        case J_BACK:       WKPageGoBack(page); break;
        case J_FORWARD:    WKPageGoForward(page); break;
        case J_RELOAD:     WKPageReload(page); break;
        case J_STOP:       WKPageStopLoading(page); break;
        case J_URL:        job.result = wkURLToUTF8(WKPageCopyActiveURL(page)); break;
        case J_TITLE:      job.result = wkStringToUTF8(WKPageCopyTitle(page)); break;
        case J_CONTENTS:   WKPageGetContentsAsString(page, &job, contentsCb); sync = false; break;
        case J_EVAL:
        case J_EXEC_SYNC:  WKPageEvaluateJavaScriptInMainFrame(page, makeWKString(job.s).get(), &job, evalCb); sync = false; break;
        case J_SET_UA:     WKPageSetCustomUserAgent(page, makeWKString(job.s).get()); break;
        case J_GET_UA: {
            WKRetainPtr<WKStringRef> ua = adoptWK(WKPageCopyCustomUserAgent(page));
            job.result = ua ? wkStringToUTF8(ua.get()) : ""; break;
        }
        case J_ZOOM:       WKPageSetPageZoomFactor(page, job.d); break;
        case J_ZOOM_RESET: WKPageSetPageZoomFactor(page, 1.0); break;
        case J_GET_ZOOM: { char buf[32]; snprintf(buf, sizeof buf, "%.4f", WKPageGetPageZoomFactor(page)); job.result = buf; break; }
        case J_SCRIPT_ENABLED: {
            auto conf = adoptWK(WKPageCopyPageConfiguration(page));
            auto pref = WKPageConfigurationGetPreferences(conf.get());
            if (job.ok) WKPreferencesSetJavaScriptEnabled(pref, job.d != 0);
            else { job.result = WKPreferencesGetJavaScriptEnabled(pref) ? "true" : "false"; }
            break;
        }
        case J_DEVTOOLS: {
            auto conf = adoptWK(WKPageCopyPageConfiguration(page));
            auto pref = WKPageConfigurationGetPreferences(conf.get());
            if (job.ok) WKPreferencesSetDeveloperExtrasEnabled(pref, job.d != 0);
            else { job.result = WKPreferencesGetDeveloperExtrasEnabled(pref) ? "true" : "false"; }
            break;
        }
        case J_IMAGES: {
            auto conf = adoptWK(WKPageCopyPageConfiguration(page));
            auto pref = WKPageConfigurationGetPreferences(conf.get());
            if (job.ok) WKPreferencesSetLoadsImagesAutomatically(pref, job.d != 0);
            else { job.result = WKPreferencesGetLoadsImagesAutomatically(pref) ? "true" : "false"; }
            break;
        }
        case J_FEATURES: {
            auto conf = adoptWK(WKPageCopyPageConfiguration(page));
            auto pref = WKPageConfigurationGetPreferences(conf.get());
            std::string out = "[";
            auto feat = adoptWK(WKPreferencesCopyExperimentalFeatures(pref));
            size_t n = WKArrayGetSize(feat.get());
            for (size_t i = 0; i < n; i++) {
                WKFeatureRef f = (WKFeatureRef)WKArrayGetItemAtIndex(feat.get(), i);
                WKRetainPtr<WKStringRef> key = adoptWK(WKFeatureCopyKey(f));
                WKRetainPtr<WKStringRef> name = adoptWK(WKFeatureCopyName(f));
                bool def = WKFeatureDefaultValue(f);
                if (i) out += ",";
                out += "{\"key\":\"" + wkStringToUTF8(key.get()) +
                       "\",\"name\":\"" + wkStringToUTF8(name.get()) +
                       "\",\"default\":" + (def ? "true" : "false") + "}";
            }
            out += "]";
            job.result = out;
            break;
        }
        case J_SET_FEATURE: {
            auto conf = adoptWK(WKPageCopyPageConfiguration(page));
            auto pref = WKPageConfigurationGetPreferences(conf.get());
            WKPreferencesSetExperimentalFeatureForKey(pref, job.d != 0, makeWKString(job.s).get());
            break;
        }
        case J_QUIT: PostMessageW(b.mainHwnd, WM_CLOSE, 0, 0); break;
        }
        return sync;
    }

    LRESULT CALLBACK LuaMsgWndProc(HWND hw, UINT msg, WPARAM wp, LPARAM lp) {
        if (msg == WM_LUA_JOB) {
            Job* job = (Job*)lp;
            if (executeJob(*job))
                signalDone(g_bridge, *job);
            return 0;
        }
        return DefWindowProcW(hw, msg, wp, lp);
    }

    bool registerMsgClass() {
        static bool done = false;
        if (done) return true;
        WNDCLASSEXW wc = { sizeof(wc) };
        wc.lpfnWndProc = LuaMsgWndProc;
        wc.lpszClassName = L"MiniBrowserLuaMsg";
        if (!RegisterClassExW(&wc)) {
            DWORD e = GetLastError();
            if (e != ERROR_CLASS_ALREADY_EXISTS) return false;
        }
        done = true;
        return true;
    }

    void createMsgWindow(Bridge& b) {
        if (!registerMsgClass()) return;
        b.msgWnd = CreateWindowExW(0, L"MiniBrowserLuaMsg", L"", 0,
                                   0, 0, 0, 0, HWND_MESSAGE, nullptr, GetModuleHandleW(nullptr), nullptr);
    }

    // Lua thread -> main thread. Blocks until the job's WK call completes.
    void dispatch(Bridge& b, Job& j) {
        if (!b.msgWnd || !b.pageRaw) { j.ok = false; j.error = "bridge not ready"; return; }
        { std::lock_guard<std::mutex> lk(b.mtx); b.pending[j.id] = &j; }
        if (!PostMessageW(b.msgWnd, WM_LUA_JOB, 0, (LPARAM)&j)) {
            std::lock_guard<std::mutex> lk(b.mtx); b.pending.erase(j.id);
            j.ok = false; j.error = "post failed";
            return;
        }
        std::unique_lock<std::mutex> lk(b.mtx);
        b.cv.wait(lk, [&]{ return j.done; });
        b.pending.erase(j.id);
    }

    void pumpEvents(Bridge& b);
}

namespace {
    // ---- Lua bindings ----
    // Run a job on the main thread, push result-string or nil,error.
    static int luaJob(lua_State* L, JobKind k, const char* s, double d, bool isSet) {
        Job j;
        j.id = g_bridge.nextId++;
        j.kind = k;
        j.s = s ? s : "";
        j.d = d;
        j.ok = isSet;
        dispatch(g_bridge, j);
        if (j.ok) {
            lua_pushstring(L, j.result.c_str());
            return 1;
        }
        lua_pushnil(L);
        lua_pushstring(L, j.error.empty() ? "job failed" : j.error.c_str());
        return 2;
    }

    // Wrap a user expression so eval always returns a JSON string.
    static std::string wrapEval(const char* expr) {
        return "(function(){'use strict';var __r=(function(){return (" +
               std::string(expr) + ");})();return JSON.stringify(__r===undefined?null:__r);})()";
    }
    static std::string wrapFunction(const char* body, const char* args) {
        return "(function(" + std::string(args) + "){'use strict';var __r=" +
               std::string(body) + ";return JSON.stringify(__r===undefined?null:__r);})()";
    }

    static int luab_nav(lua_State* L) { const char* u = luaL_checkstring(L, 1); return luaJob(L, J_NAV, u, 0, false); }
    static int luab_back(lua_State* L) { return luaJob(L, J_BACK, nullptr, 0, false); }
    static int luab_forward(lua_State* L) { return luaJob(L, J_FORWARD, nullptr, 0, false); }
    static int luab_reload(lua_State* L) { return luaJob(L, J_RELOAD, nullptr, 0, false); }
    static int luab_stop(lua_State* L) { return luaJob(L, J_STOP, nullptr, 0, false); }
    static int luab_url(lua_State* L) { return luaJob(L, J_URL, nullptr, 0, false); }
    static int luab_title(lua_State* L) { return luaJob(L, J_TITLE, nullptr, 0, false); }
    static int luab_contents(lua_State* L) { return luaJob(L, J_CONTENTS, nullptr, 0, false); }
    static int luab_eval(lua_State* L) { std::string s = wrapEval(luaL_checkstring(L, 1)); return luaJob(L, J_EVAL, s.c_str(), 0, false); }
    static int luab_execute(lua_State* L) {
        std::string body = luaL_optstring(L, 1, "return null");
        std::string args = luaL_optstring(L, 2, "");
        std::string s = wrapFunction(body.c_str(), args.c_str());
        return luaJob(L, J_EXEC_SYNC, s.c_str(), 0, false);
    }
    static int luab_set_ua(lua_State* L) { return luaJob(L, J_SET_UA, luaL_checkstring(L, 1), 0, false); }
    static int luab_get_ua(lua_State* L) { return luaJob(L, J_GET_UA, nullptr, 0, false); }
    static int luab_zoom(lua_State* L) { double f = luaL_checknumber(L, 1); return luaJob(L, J_ZOOM, nullptr, f, false); }
    static int luab_zoom_factor(lua_State* L) { return luaJob(L, J_GET_ZOOM, nullptr, 0, false); }
    static int luab_zoom_reset(lua_State* L) { return luaJob(L, J_ZOOM_RESET, nullptr, 0, false); }
    static int luab_script_enabled(lua_State* L) {
        if (lua_gettop(L) >= 1 && !lua_isnil(L, 1)) return luaJob(L, J_SCRIPT_ENABLED, nullptr, lua_toboolean(L, 1) ? 1 : 0, true);
        return luaJob(L, J_SCRIPT_ENABLED, nullptr, 0, false);
    }
    static int luab_devtools_enabled(lua_State* L) {
        if (lua_gettop(L) >= 1 && !lua_isnil(L, 1)) return luaJob(L, J_DEVTOOLS, nullptr, lua_toboolean(L, 1) ? 1 : 0, true);
        return luaJob(L, J_DEVTOOLS, nullptr, 0, false);
    }
    static int luab_images_enabled(lua_State* L) {
        if (lua_gettop(L) >= 1 && !lua_isnil(L, 1)) return luaJob(L, J_IMAGES, nullptr, lua_toboolean(L, 1) ? 1 : 0, true);
        return luaJob(L, J_IMAGES, nullptr, 0, false);
    }
    static int luab_features(lua_State* L) { return luaJob(L, J_FEATURES, nullptr, 0, false); }
    static int luab_set_feature(lua_State* L) {
        const char* key = luaL_checkstring(L, 1);
        bool on = lua_toboolean(L, 2);
        return luaJob(L, J_SET_FEATURE, key, on ? 1 : 0, false);
    }
    static int luab_quit(lua_State* L) { return luaJob(L, J_QUIT, nullptr, 0, false); }
    static int luab_pump(lua_State* L) { pumpEvents(g_bridge); return 0; }

    static int luab_on(lua_State* L);   // defined below

    void registerLuaApi(lua_State* L) {
        static const luaL_Reg fns[] = {
            { "nav", luab_nav }, { "back", luab_back }, { "forward", luab_forward },
            { "reload", luab_reload }, { "stop", luab_stop },
            { "url", luab_url }, { "title", luab_title }, { "contents", luab_contents },
            { "eval", luab_eval }, { "execute", luab_execute },
            { "set_user_agent", luab_set_ua }, { "user_agent", luab_get_ua },
            { "zoom", luab_zoom }, { "zoom_factor", luab_zoom_factor }, { "zoom_reset", luab_zoom_reset },
            { "script_enabled", luab_script_enabled }, { "devtools_enabled", luab_devtools_enabled },
            { "images_enabled", luab_images_enabled },
            { "features", luab_features }, { "set_feature", luab_set_feature },
            { "on", luab_on }, { "pump_events", luab_pump }, { "quit", luab_quit },
            { nullptr, nullptr }
        };
        luaL_register(L, "minibrowser", fns);
        lua_pushvalue(L, -1);
        lua_setglobal(L, "mb");
    }
}

#include <chrono>
namespace {
    static int evTableRef = LUA_NOREF;
    static void ensureEvTable(lua_State* L) {
        if (evTableRef == LUA_NOREF) {
            lua_newtable(L);
            evTableRef = luaL_ref(L, LUA_REGISTRYINDEX);
        }
    }
    static int luab_on(lua_State* L) {
        const char* type = luaL_checkstring(L, 1);
        luaL_checktype(L, 2, LUA_TFUNCTION);
        ensureEvTable(L);
        lua_rawgeti(L, LUA_REGISTRYINDEX, evTableRef);
        lua_pushvalue(L, 2);
        lua_setfield(L, -2, type);
        lua_pop(L, 1);
        return 0;
    }
    static void dispatchEvent(lua_State* L, const std::string& type, const std::string& json) {
        if (!L) return;
        ensureEvTable(L);
        lua_rawgeti(L, LUA_REGISTRYINDEX, evTableRef);
        lua_getfield(L, -1, type.c_str());
        if (lua_isfunction(L, -1)) {
            lua_pushstring(L, json.c_str());
            if (lua_pcall(L, 1, 0, 0) != 0) {
                const char* e = lua_tostring(L, -1);
                fprintf(stderr, "[lua] %s\n", e ? e : "?");
                lua_pop(L, 1);
            }
        }
        lua_pop(L, 2);
    }

    void pumpEvents(Bridge& b) {
        std::vector<std::string> batch;
        { std::lock_guard<std::mutex> lk(b.evMtx); batch.swap(b.events); }
        for (auto& ev : batch) {
            size_t t = ev.find('\t');
            std::string type = (t == std::string::npos) ? ev : ev.substr(0, t);
            std::string json = (t == std::string::npos) ? "" : ev.substr(t + 1);
            if (b.quitting) break;
            dispatchEvent(b.L, type, json);
        }
    }

    void runLuaMain(Bridge& b) {
        lua_State* L = luaL_newstate();
        if (!L) return;
        b.L = L;
        luaL_openlibs(L);
        registerLuaApi(L);
        ensureEvTable(L);

        std::string script = cmdlineArgValue("lua");
        if (!script.empty()) {
            if (luaL_loadfile(L, script.c_str()) == 0) {
                if (lua_pcall(L, 0, 0, 0) != 0) {
                    const char* e = lua_tostring(L, -1);
                    fprintf(stderr, "[lua] %s\n", e ? e : "?");
                    lua_pop(L, 1);
                }
            } else {
                const char* e = lua_tostring(L, -1);
                fprintf(stderr, "[lua] load %s: %s\n", script.c_str(), e ? e : "?");
                lua_pop(L, 1);
            }
        }

        // Event pump loop (runs until shutdown/quit).
        std::unique_lock<std::mutex> lk(b.evMtx);
        while (!b.quitting) {
            if (b.events.empty())
                b.evCv.wait_for(lk, std::chrono::milliseconds(100),
                                [&]{ return b.quitting || !b.events.empty(); });
            lk.unlock();
            pumpEvents(b);
            lk.lock();
        }
        lua_close(L);
        b.L = nullptr;
    }
}

extern "C" int MiniBrowserLuaInit(void* page, void* hwnd) {
    if (cmdlineArgValue("lua").empty())
        return 0;                 // not requested -> stay disabled, zero behavior change
    bool first = !g_bridge.enabled;
    g_bridge.pageRaw = page;
    g_bridge.mainHwnd = (HWND)hwnd;
    g_bridge.enabled = true;
    if (first) {
        createMsgWindow(g_bridge);
        g_bridge.luaThread = std::thread(runLuaMain, std::ref(g_bridge));
    }
    return 1;
}

extern "C" void MiniBrowserLuaOnEvent(const char* type, const char* json) {
    if (!g_bridge.enabled) return;
    { std::lock_guard<std::mutex> lk(g_bridge.evMtx);
      g_bridge.events.push_back(std::string(type ? type : "") + "\t" + (json ? json : "")); }
    g_bridge.evCv.notify_all();
}

extern "C" void MiniBrowserLuaShutdown() {
    if (!g_bridge.enabled) return;
    { std::lock_guard<std::mutex> lk(g_bridge.evMtx); g_bridge.quitting = true; }
    g_bridge.evCv.notify_all();
    if (g_bridge.luaThread.joinable())
        g_bridge.luaThread.join();
    if (g_bridge.msgWnd) {
        DestroyWindow(g_bridge.msgWnd);
        g_bridge.msgWnd = nullptr;
    }
    g_bridge.enabled = false;
}
#endif // MINIBROWSER_LUAJIT

