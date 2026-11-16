// C++20 / Windows 10+ / Dear ImGui 1.91.9b / DirectX 11 / UI-only configuration panel.
// This application does not inspect, modify, inject into, or control ROB RUN.
#include <Windows.h>
#include <d3d11.h>
#include <tchar.h>
#include <filesystem>
#include <fstream>
#include <string>
#include <array>
#include <algorithm>
#include <cstdint>
#include <nlohmann/json.hpp>
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
using json = nlohmann::json;

static ID3D11Device* g_device = nullptr;
static ID3D11DeviceContext* g_context = nullptr;
static IDXGISwapChain* g_swap_chain = nullptr;
static ID3D11RenderTargetView* g_main_rtv = nullptr;
static bool g_running = true;
static bool g_show_demo = false;

static void CreateRenderTarget() {
    ID3D11Texture2D* back_buffer = nullptr;
    if (SUCCEEDED(g_swap_chain->GetBuffer(0, IID_PPV_ARGS(&back_buffer)))) {
        g_device->CreateRenderTargetView(back_buffer, nullptr, &g_main_rtv);
        back_buffer->Release();
    }
}
static void CleanupRenderTarget() { if (g_main_rtv) { g_main_rtv->Release(); g_main_rtv = nullptr; } }
static HRESULT CreateDeviceD3D(HWND hwnd) {
    DXGI_SWAP_CHAIN_DESC sd{};
    sd.BufferCount = 2; sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate = { 144, 1 }; sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; sd.OutputWindow = hwnd;
    sd.SampleDesc.Count = 1; sd.Windowed = TRUE; sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    UINT flags = 0;
#ifdef _DEBUG
    flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
    D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };
    D3D_FEATURE_LEVEL obtained{};
    HRESULT hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags,
        levels, 2, D3D11_SDK_VERSION, &sd, &g_swap_chain, &g_device, &obtained, &g_context);
    if (FAILED(hr)) return hr;
    CreateRenderTarget(); return S_OK;
}
static void CleanupDeviceD3D() {
    CleanupRenderTarget();
    if (g_swap_chain) { g_swap_chain->Release(); g_swap_chain = nullptr; }
    if (g_context) { g_context->Release(); g_context = nullptr; }
    if (g_device) { g_device->Release(); g_device = nullptr; }
}

struct VisualSettings {
    bool enabled = true, box = true, corner_box = false, name = true, distance = true;
    bool health_bar = true, skeleton = false, snaplines = false;
    float max_distance = 250.0f, box_thickness = 1.5f, rounding = 3.0f;
    float box_color[4] = { 0.12f, 0.88f, 1.0f, 1.0f };
    float text_color[4] = { 0.88f, 0.96f, 1.0f, 1.0f };
    float visible_color[4] = { 0.20f, 1.0f, 0.68f, 1.0f };
    float hidden_color[4] = { 1.0f, 0.28f, 0.35f, 1.0f };
};
struct AimSettings {
    bool enabled = false, draw_fov = true, visibility_check = true;
    float strength = 35.0f, fov = 90.0f, smoothness = 8.0f, max_distance = 180.0f;
    int target_bone = 0, activation_key = VK_RBUTTON;
};
struct AppSettings {
    VisualSettings visuals;
    AimSettings aim;
    bool show_fps = true, watermark = true, save_on_exit = true;
    float ui_scale = 1.0f, accent[4] = { 0.12f, 0.88f, 1.0f, 1.0f };
    int selected_tab = 0;
};
static AppSettings g;
static const std::filesystem::path ConfigPath = "config/rob_run_cyber_cyan.json";

static json ColorToJson(const float c[4]) { return json::array({ c[0], c[1], c[2], c[3] }); }
static void JsonToColor(const json& j, float c[4]) {
    if (!j.is_array() || j.size() != 4) return;
    for (int i = 0; i < 4; ++i) if (j[i].is_number()) c[i] = std::clamp(j[i].get<float>(), 0.0f, 1.0f);
}
static json ToJson() {
    const auto& v = g.visuals; const auto& a = g.aim;
    return {
      {"visuals", {{"enabled",v.enabled},{"box",v.box},{"corner_box",v.corner_box},{"name",v.name},{"distance",v.distance},
        {"health_bar",v.health_bar},{"skeleton",v.skeleton},{"snaplines",v.snaplines},{"max_distance",v.max_distance},
        {"box_thickness",v.box_thickness},{"rounding",v.rounding},{"box_color",ColorToJson(v.box_color)},
        {"text_color",ColorToJson(v.text_color)},{"visible_color",ColorToJson(v.visible_color)},{"hidden_color",ColorToJson(v.hidden_color)}}},
      {"aim", {{"enabled",a.enabled},{"draw_fov",a.draw_fov},{"visibility_check",a.visibility_check},{"strength",a.strength},
        {"fov",a.fov},{"smoothness",a.smoothness},{"max_distance",a.max_distance},{"target_bone",a.target_bone},{"activation_key",a.activation_key}}},
      {"ui", {{"show_fps",g.show_fps},{"watermark",g.watermark},{"save_on_exit",g.save_on_exit},{"ui_scale",g.ui_scale},{"accent",ColorToJson(g.accent)}}}
    };
}
static void LoadConfig() {
    try {
        if (!std::filesystem::exists(ConfigPath)) return;
        std::ifstream in(ConfigPath); json j; in >> j;
        if (j.contains("visuals")) { auto x=j["visuals"]; auto& v=g.visuals;
            v.enabled=x.value("enabled",v.enabled); v.box=x.value("box",v.box); v.corner_box=x.value("corner_box",v.corner_box);
            v.name=x.value("name",v.name); v.distance=x.value("distance",v.distance); v.health_bar=x.value("health_bar",v.health_bar);
            v.skeleton=x.value("skeleton",v.skeleton); v.snaplines=x.value("snaplines",v.snaplines);
            v.max_distance=std::clamp(x.value("max_distance",v.max_distance),1.0f,2000.0f);
            v.box_thickness=std::clamp(x.value("box_thickness",v.box_thickness),0.5f,6.0f);
            v.rounding=std::clamp(x.value("rounding",v.rounding),0.0f,16.0f);
            if(x.contains("box_color"))JsonToColor(x["box_color"],v.box_color); if(x.contains("text_color"))JsonToColor(x["text_color"],v.text_color);
            if(x.contains("visible_color"))JsonToColor(x["visible_color"],v.visible_color); if(x.contains("hidden_color"))JsonToColor(x["hidden_color"],v.hidden_color);
        }
        if (j.contains("aim")) { auto x=j["aim"]; auto& a=g.aim;
            a.enabled=x.value("enabled",a.enabled); a.draw_fov=x.value("draw_fov",a.draw_fov); a.visibility_check=x.value("visibility_check",a.visibility_check);
            a.strength=std::clamp(x.value("strength",a.strength),0.0f,100.0f); a.fov=std::clamp(x.value("fov",a.fov),1.0f,360.0f);
            a.smoothness=std::clamp(x.value("smoothness",a.smoothness),1.0f,30.0f); a.max_distance=std::clamp(x.value("max_distance",a.max_distance),1.0f,2000.0f);
            a.target_bone=std::clamp(x.value("target_bone",a.target_bone),0,3); a.activation_key=x.value("activation_key",a.activation_key);
        }
        if (j.contains("ui")) { auto x=j["ui"]; g.show_fps=x.value("show_fps",g.show_fps); g.watermark=x.value("watermark",g.watermark);
            g.save_on_exit=x.value("save_on_exit",g.save_on_exit); g.ui_scale=std::clamp(x.value("ui_scale",g.ui_scale),0.8f,1.4f);
            if(x.contains("accent"))JsonToColor(x["accent"],g.accent); }
    } catch (...) { /* Keep safe defaults if the config is malformed. */ }
}
static bool SaveConfig() {
    try { std::filesystem::create_directories(ConfigPath.parent_path()); std::ofstream out(ConfigPath);
        out << ToJson().dump(2); return out.good(); } catch (...) { return false; }
}

static void ApplyTheme() {
    ImGuiStyle& s = ImGui::GetStyle(); s.WindowRounding=10; s.ChildRounding=8; s.FrameRounding=5;
    s.PopupRounding=6; s.ScrollbarRounding=8; s.GrabRounding=5; s.WindowPadding={18,16}; s.FramePadding={10,7};
    s.ItemSpacing={10,10}; s.WindowBorderSize=1; s.FrameBorderSize=0;
    ImVec4* c=s.Colors;
    c[ImGuiCol_Text]=ImVec4(.88f,.94f,.97f,1); c[ImGuiCol_TextDisabled]=ImVec4(.38f,.48f,.53f,1);
    c[ImGuiCol_WindowBg]=ImVec4(.035f,.047f,.060f,1); c[ImGuiCol_ChildBg]=ImVec4(.047f,.063f,.078f,1);
    c[ImGuiCol_PopupBg]=ImVec4(.055f,.075f,.09f,.98f); c[ImGuiCol_Border]=ImVec4(.10f,.20f,.24f,.75f);
    c[ImGuiCol_FrameBg]=ImVec4(.075f,.105f,.125f,1); c[ImGuiCol_FrameBgHovered]=ImVec4(.09f,.19f,.22f,1);
    c[ImGuiCol_FrameBgActive]=ImVec4(.10f,.25f,.29f,1); c[ImGuiCol_TitleBg]=ImVec4(.025f,.035f,.045f,1);
    c[ImGuiCol_TitleBgActive]=ImVec4(.035f,.06f,.07f,1); c[ImGuiCol_CheckMark]=ImVec4(g.accent[0],g.accent[1],g.accent[2],1);
    c[ImGuiCol_SliderGrab]=ImVec4(g.accent[0],g.accent[1],g.accent[2],.85f); c[ImGuiCol_SliderGrabActive]=ImVec4(.35f,.96f,1,1);
    c[ImGuiCol_Button]=ImVec4(.06f,.20f,.23f,1); c[ImGuiCol_ButtonHovered]=ImVec4(.08f,.35f,.39f,1);
    c[ImGuiCol_ButtonActive]=ImVec4(.10f,.46f,.50f,1); c[ImGuiCol_Header]=ImVec4(.06f,.20f,.23f,1);
    c[ImGuiCol_HeaderHovered]=ImVec4(.08f,.30f,.34f,1); c[ImGuiCol_HeaderActive]=ImVec4(.09f,.38f,.42f,1);
    c[ImGuiCol_Separator]=ImVec4(.10f,.22f,.26f,1); c[ImGuiCol_ResizeGrip]=ImVec4(g.accent[0],g.accent[1],g.accent[2],.25f);
}
static void Section(const char* title) { ImGui::Spacing(); ImGui::TextColored(ImVec4(.12f,.88f,1,1),"%s",title); ImGui::Separator(); ImGui::Spacing(); }
static void Toggle(const char* label, bool* value) { ImGui::Checkbox(label,value); }
static void VisualsPage() {
    auto& v=g.visuals;
    ImGui::BeginChild("visuals_left",{0,0},ImGuiChildFlags_Border);
    Section("ESP / DISPLAY OPTIONS"); Toggle("Enable visual settings",&v.enabled); Toggle("2D Bounding Box",&v.box); Toggle("Corner Box",&v.corner_box);
    Toggle("Player Name",&v.name); Toggle("Distance",&v.distance); Toggle("Health Bar",&v.health_bar);
    Toggle("Skeleton overlay option",&v.skeleton); Toggle("Snaplines option",&v.snaplines);
    Section("GEOMETRY"); ImGui::SliderFloat("Maximum distance",&v.max_distance,1,2000,"%.0f m");
    ImGui::SliderFloat("Line thickness",&v.box_thickness,.5f,6,"%.1f px"); ImGui::SliderFloat("Corner rounding",&v.rounding,0,16,"%.1f px");
    ImGui::EndChild();
    ImGui::SameLine(); ImGui::BeginChild("visuals_right",{0,0},ImGuiChildFlags_Border);
    Section("COLOR STUDIO"); ImGui::ColorEdit4("Box color",v.box_color,ImGuiColorEditFlags_AlphaBar);
    ImGui::ColorEdit4("Text color",v.text_color,ImGuiColorEditFlags_AlphaBar);
    ImGui::ColorEdit4("Visible-state color",v.visible_color,ImGuiColorEditFlags_AlphaBar);
    ImGui::ColorEdit4("Occluded-state color",v.hidden_color,ImGuiColorEditFlags_AlphaBar);
    Section("PREVIEW");
    ImVec2 p=ImGui::GetCursorScreenPos(); ImVec2 avail=ImGui::GetContentRegionAvail(); float w=std::max(180.0f,avail.x-8), h=170;
    ImGui::InvisibleButton("preview_canvas",{w,h}); auto* d=ImGui::GetWindowDrawList();
    ImVec2 a={p.x+28,p.y+12}, b={p.x+w-28,p.y+h-12}; d->AddRectFilled(a,b,IM_COL32(12,24,31,255),7);
    ImVec2 ba={p.x+w*.40f,p.y+27}, bb={p.x+w*.62f,p.y+h-28};
    ImU32 bc=ImGui::ColorConvertFloat4ToU32({v.box_color[0],v.box_color[1],v.box_color[2],v.box_color[3]});
    d->AddRect(ba,bb,bc,v.rounding,0,v.box_thickness);
    d->AddText({ba.x,ba.y-17},ImGui::ColorConvertFloat4ToU32({v.text_color[0],v.text_color[1],v.text_color[2],1}),v.name?"Player_Example":"");
    d->AddText({ba.x,bb.y+3},IM_COL32(190,220,230,255),v.distance?"42 m":"");
    d->AddRectFilled({ba.x,bb.y+18},{bb.x,bb.y+23},IM_COL32(35,55,62,255));
    d->AddRectFilled({ba.x,bb.y+18},{ba.x+(bb.x-ba.x)*.72f,bb.y+23},IM_COL32(55,225,165,255));
    ImGui::EndChild();
}
static void AimPage() {
    auto& a=g.aim; ImGui::BeginChild("aim_page",{0,0},ImGuiChildFlags_Border);
    Section("AIM CONFIGURATION"); Toggle("Enable aim settings profile",&a.enabled);
    ImGui::TextDisabled("Configuration panel only — no game input or target acquisition is connected.");
    ImGui::SliderFloat("Strength",&a.strength,0,100,"%.0f%%"); ImGui::SliderFloat("FOV",&a.fov,1,360,"%.0f deg");
    ImGui::SliderFloat("Smoothness",&a.smoothness,1,30,"%.1f"); ImGui::SliderFloat("Maximum distance",&a.max_distance,1,2000,"%.0f m");
    const char* bones[]={"Head","Neck","Chest","Body"}; ImGui::Combo("Target point",&a.target_bone,bones,IM_ARRAYSIZE(bones));
    Toggle("Show FOV preview option",&a.draw_fov); Toggle("Visibility-check option",&a.visibility_check);
    Section("ACTIVATION"); const char* key_names[]={"Right Mouse","Left Mouse","Mouse 4","Mouse 5","Left Alt","Left Shift"};
    int key_values[]={VK_RBUTTON,VK_LBUTTON,VK_XBUTTON1,VK_XBUTTON2,VK_MENU,VK_SHIFT}; int current=0;
    for(int i=0;i<IM_ARRAYSIZE(key_values);++i) if(a.activation_key==key_values[i]) current=i;
    if(ImGui::Combo("Activation key",&current,key_names,IM_ARRAYSIZE(key_names))) a.activation_key=key_values[current];
    ImGui::EndChild();
}
static void MiscPage() {
    ImGui::BeginChild("misc_page",{0,0},ImGuiChildFlags_Border); Section("OVERLAY PREFERENCES");
    Toggle("Show FPS counter",&g.show_fps); Toggle("Show panel watermark",&g.watermark);
    ImGui::TextDisabled("These preferences affect this configuration UI only."); ImGui::EndChild();
}
static void SettingsPage() {
    ImGui::BeginChild("settings_page",{0,0},ImGuiChildFlags_Border); Section("APPEARANCE");
    ImGui::ColorEdit4("Accent color",g.accent,ImGuiColorEditFlags_NoInputs|ImGuiColorEditFlags_AlphaBar);
    ImGui::SliderFloat("UI scale",&g.ui_scale,.8f,1.4f,"%.2fx");
    Section("CONFIGURATION"); Toggle("Save on exit",&g.save_on_exit);
    if(ImGui::Button("Save configuration",{170,36})) SaveConfig(); ImGui::SameLine();
    if(ImGui::Button("Reload configuration",{170,36})) LoadConfig(); ImGui::SameLine();
    if(ImGui::Button("Reset defaults",{130,36})) { g=AppSettings{}; }
    ImGui::TextDisabled("File: config/rob_run_cyber_cyan.json");
    Section("ABOUT"); ImGui::Text("ROB RUN / CYBER CYAN"); ImGui::TextDisabled("C++20  |  Dear ImGui  |  DirectX 11");
    ImGui::TextDisabled("Standalone settings UI. No process access, injection, memory reads, or game automation."); ImGui::EndChild();
}
static void RenderUI() {
    ApplyTheme(); ImGui::SetNextWindowSize({900,610},ImGuiCond_FirstUseEver); ImGui::SetNextWindowSizeConstraints({760,520},{1600,1100});
    ImGui::Begin("CYBER CYAN  /  ROB RUN CONFIG",nullptr,ImGuiWindowFlags_NoCollapse);
    ImGui::TextColored(ImVec4(.12f,.88f,1,1),"CYBER CYAN"); ImGui::SameLine(); ImGui::TextDisabled("  //  CONFIGURATION STUDIO");
    ImGui::SameLine(ImGui::GetWindowWidth()-125); ImGui::TextColored(ImVec4(.25f,.85f,.7f,1),"● LOCAL UI"); ImGui::Separator();
    ImGui::BeginChild("sidebar",{155,0},ImGuiChildFlags_Border);
    const char* tabs[]={"Visuals","Aim","Misc","Settings"};
    for(int i=0;i<4;++i) { if(ImGui::Selectable(tabs[i],g.selected_tab==i,0,{130,38})) g.selected_tab=i; }
    ImGui::SetCursorPosY(ImGui::GetWindowHeight()-55); ImGui::TextDisabled("BUILD  1.0.0"); ImGui::EndChild(); ImGui::SameLine();
    ImGui::BeginChild("content",{0,0});
    if(g.selected_tab==0) VisualsPage(); else if(g.selected_tab==1) AimPage(); else if(g.selected_tab==2) MiscPage(); else SettingsPage();
    ImGui::EndChild(); ImGui::End();
    if(g.show_fps) { ImGui::SetNextWindowBgAlpha(.35f); ImGui::Begin("##fps",nullptr,ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_AlwaysAutoResize|ImGuiWindowFlags_NoInputs);
        ImGui::Text("%.0f FPS",ImGui::GetIO().Framerate); ImGui::End(); }
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);
static LRESULT WINAPI WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    if(ImGui_ImplWin32_WndProcHandler(hwnd,msg,wparam,lparam)) return true;
    switch(msg) {
    case WM_SIZE: if(g_device && wparam!=SIZE_MINIMIZED) { CleanupRenderTarget(); g_swap_chain->ResizeBuffers(0,(UINT)LOWORD(lparam),(UINT)HIWORD(lparam),DXGI_FORMAT_UNKNOWN,0); CreateRenderTarget(); } return 0;
    case WM_SYSCOMMAND: if((wparam&0xfff0)==SC_KEYMENU) return 0; break;
    case WM_DESTROY: PostQuitMessage(0); return 0;
    }
    return DefWindowProc(hwnd,msg,wparam,lparam);
}
int WINAPI WinMain(HINSTANCE instance,HINSTANCE,LPSTR,int) {
    LoadConfig();
    WNDCLASSEX wc{sizeof(WNDCLASSEX),CS_CLASSDC,WndProc,0,0,instance,nullptr,nullptr,nullptr,nullptr,_T("RobRunCyberCyan"),nullptr};
    RegisterClassEx(&wc);
    HWND hwnd=CreateWindow(wc.lpszClassName,_T("ROB RUN | CYBER CYAN"),WS_OVERLAPPEDWINDOW,100,100,1100,760,nullptr,nullptr,wc.hInstance,nullptr);
    if(!hwnd) { UnregisterClass(wc.lpszClassName,wc.hInstance); return 1; }
    if(FAILED(CreateDeviceD3D(hwnd))) { CleanupDeviceD3D(); DestroyWindow(hwnd); UnregisterClass(wc.lpszClassName,wc.hInstance); return 1; }
    ShowWindow(hwnd,SW_SHOWDEFAULT); UpdateWindow(hwnd);
    IMGUI_CHECKVERSION(); ImGui::CreateContext(); ImGuiIO& io=ImGui::GetIO(); io.ConfigFlags|=ImGuiConfigFlags_NavEnableKeyboard;
    io.FontGlobalScale=g.ui_scale; ApplyTheme(); ImGui_ImplWin32_Init(hwnd); ImGui_ImplDX11_Init(g_device,g_context);
    MSG msg{};
    while(g_running) {
        while(PeekMessage(&msg,nullptr,0,0,PM_REMOVE)) { TranslateMessage(&msg); DispatchMessage(&msg); if(msg.message==WM_QUIT) g_running=false; }
        if(!g_running) break;
        io.FontGlobalScale=g.ui_scale;
        ImGui_ImplDX11_NewFrame(); ImGui_ImplWin32_NewFrame(); ImGui::NewFrame(); RenderUI(); ImGui::Render();
        const float clear[4]={.025f,.035f,.045f,1}; g_context->OMSetRenderTargets(1,&g_main_rtv,nullptr); g_context->ClearRenderTargetView(g_main_rtv,clear);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData()); g_swap_chain->Present(1,0);
    }
    if(g.save_on_exit) SaveConfig(); ImGui_ImplDX11_Shutdown(); ImGui_ImplWin32_Shutdown(); ImGui::DestroyContext();
    CleanupDeviceD3D(); DestroyWindow(hwnd); UnregisterClass(wc.lpszClassName,wc.hInstance); return 0;
}
