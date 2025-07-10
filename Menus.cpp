#include "Page.hpp"

#include <sstream>
#include <iomanip>
#include <string>
#include <cpp_redis/cpp_redis>

// Move this struct here, before any function that uses it
struct MenuSetting {
    const char* label;
    const char* redis_key;
    int* current_value;
    const int* options;
    int num_options;
};


static int current_fps = 24;
static int current_iso = 100;
static int current_sht = 180; // example default
static int current_wb = 5600; // example default

void set_redis_value(const char* key, int value, int* current) {
    cpp_redis::client client;
    client.connect();
    client.set(key, std::to_string(value));
    client.publish("cp_controls", key);
    client.sync_commit();
    *current = value;
}

void load_value_from_redis(const char* key, int* current, int fallback) {
    cpp_redis::client client;
    client.connect();
    auto reply = client.get(key);
    client.sync_commit();
    auto result = reply.get();
    if (result.is_string()) {
        try {
            *current = std::stoi(result.as_string());
        } catch (...) {
            *current = fallback;
        }
    } else {
        *current = fallback;
    }
}

void load_all_menu_settings() {
    load_value_from_redis("fps", &current_fps, 24);
    load_value_from_redis("iso", &current_iso, 100);
    load_value_from_redis("shutter_a", &current_sht, 180);
    load_value_from_redis("awb", &current_wb, 5600);
}

std::string floatToFormattedString(float value) {
    std::ostringstream oss;
    oss << std::setw(7) << std::setfill(' ') << std::fixed << std::setprecision(0) << value;
    return oss.str();
}

// filepath: /home/pi/cinepi-gui/Menus.cpp
Menus::Menus(Application& app)
    : Page(app), logo("cinepi-gui/assets/img/logo.png")
{
    console = spdlog::stdout_color_mt("menus");
    load_all_menu_settings();
}
void Menus::show()
{
    menu_top();
    menu_bottom();
}



void Menus::menu_top()
{
    ImGuiIO &io = ImGui::GetIO(); (void)io;
    FrameBuffer fb = app.buffers.getBuffer();

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0f, 16.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(32.0f, 4.0f));
    {
        ImGui::SetNextWindowSize(ImVec2(720, 80));
        ImGui::SetNextWindowPos(ImVec2(0, 0));

        ImGuiWindowFlags window_flags = 0;
        window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_MenuBar |
                        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
                        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoBackground;

        ImGui::PushStyleColor(ImGuiCol_MenuBarBg, ImVec4(0.0f, 0.0f, 0.0f, 0.5f));
        ImGui::Begin("Menus", NULL, window_flags);

        if (ImGui::BeginMenuBar())
        {
            ImDrawList* draw_list = ImGui::GetWindowDrawList();

            // Define options for each menu
            static const int fps_options[] = {24, 30, 50};
            static const int iso_options[] = {100, 200, 400, 800};
            static const int sht_options[] = {45, 90, 144, 180};
            static const int wb_options[] = {3200, 4000, 5600, 6500};  // Example values

            MenuSetting settings[] = {
                {"FPS", "fps", &current_fps, fps_options, sizeof(fps_options)/sizeof(int)},
                {"ISO", "iso", &current_iso, iso_options, sizeof(iso_options)/sizeof(int)},
                {"SHT", "shutter_a", &current_sht, sht_options, sizeof(sht_options)/sizeof(int)},
                {"WB", "awb", &current_wb, wb_options, sizeof(wb_options)/sizeof(int)},
            };

            int num_menus = 4; // FPS, ISO, SHT, WB
            float total_width = 720.0f;
            float y_pos = 32.0f;
            float x_margin = 40.0f; // optional, for some left/right margin
            float available_width = total_width - 2 * x_margin;
            float spacing = available_width / (num_menus - 1);

            for (int i = 0; i < num_menus; ++i) {
                std::string menu_label = "   " + std::to_string(*settings[i].current_value);
                if (ImGui::BeginMenu(menu_label.c_str())) {
                    for (int j = 0; j < settings[i].num_options; ++j) {
                        int val = settings[i].options[j];
                        if (ImGui::MenuItem(std::to_string(val).c_str(), nullptr, *settings[i].current_value == val)) {
                            set_redis_value(settings[i].redis_key, val, settings[i].current_value);
                        }
                    }
                    ImGui::EndMenu();
                }
                float x_pos = x_margin + i * spacing;
                draw_list->AddText(app.ui24, app.ui24->FontSize, ImVec2(x_pos, y_pos), IM_COL32(100, 100, 100, 255), settings[i].label);
            }

            // ...rest of your menu (cog/settings, etc)...
            ImGui::Separator();
            if (ImGui::BeginMenu(ICON_FA_COG))
            {
                // ...existing settings menu code...
                ImGui::EndMenu();
            }
            ImGui::Separator();

            ImGui::EndMenuBar();
        }
        ImGui::End();
        ImGui::PopStyleColor(1);
    }
    ImGui::PopStyleVar(2);
}

void Menus::menu_bottom()
{
    ImGuiIO &io = ImGui::GetIO(); (void)io;
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0f, 16.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(32.0f, 4.0f));

    float bar_height = ImGui::GetFrameHeight();
    float screen_width = 720.0f;
    float screen_height = 720.0f;

    // Set position to the bottom of the viewport
    ImGui::SetNextWindowPos(ImVec2(0, screen_height - bar_height));
    ImGui::SetNextWindowSize(ImVec2(screen_width, bar_height));

    ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollWithMouse |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoBackground |
        ImGuiWindowFlags_MenuBar;

    ImGui::PushStyleColor(ImGuiCol_MenuBarBg, ImVec4(0.0f, 0.0f, 0.0f, 0.5f));
    if (ImGui::Begin("StatusBar", nullptr, flags)) {
        if (ImGui::BeginMenuBar()) {
            ImDrawList* draw_list = ImGui::GetWindowDrawList();

            // Example: 3 status texts, spaced evenly
            const char* labels[] = {
                ("FPS: " + std::to_string((int)io.Framerate)).c_str(),
                ("GPU: " + std::to_string((int)app.stats.stat_gpu) + "%").c_str(),
                ("CPU: " + std::to_string((int)app.stats.stat_cpu) + "%").c_str()
            };
            int num_labels = 3;
            float x_margin = 40.0f;
            float available_width = screen_width - 2 * x_margin;
            float spacing = available_width / (num_labels - 1);
            float y_pos = 8.0f; // Adjust as needed for vertical alignment

            for (int i = 0; i < num_labels; ++i) {
                float x_pos = x_margin + i * spacing;
                draw_list->AddText(app.ui24, app.ui24->FontSize, ImVec2(x_pos, y_pos), IM_COL32(255, 255, 255, 255), labels[i]);
            }

            ImGui::EndMenuBar();
        }
        ImGui::End();
    }
    ImGui::PopStyleColor(1);
    ImGui::PopStyleVar(2);
}

Page *make_menus_page(Application& app){
    return new Menus(app);
}