#include "Application.hpp"
#include "Page.hpp"

#define DISPLAY_WIDTH 720
#define DISPLAY_HEIGHT 720

#include <cpp_redis/cpp_redis>
#include <iostream>

void test_redis() {
    cpp_redis::client client;
    client.connect();
    client.set("cinepi-gui:test", "hello", [](cpp_redis::reply& reply) {});
    client.get("cinepi-gui:test", [](cpp_redis::reply& reply) {
        if (reply.is_string()) {
            std::cout << "Redis test value: " << reply.as_string() << std::endl;
        }
    });
    client.sync_commit();
}

int main()
{    
    Application app;
    app.init(DISPLAY_WIDTH,DISPLAY_HEIGHT);

    test_redis(); // <-- Add here

    Page* viewport_page = make_viewport_page(app);
    Viewport* viewport = reinterpret_cast<Viewport*>(viewport_page);

    Page* overlay_page = make_overlays_page(app);
    Overlays* overlays = reinterpret_cast<Overlays*>(overlay_page);

    Page* menus_page = make_menus_page(app);
    Menus* menus = reinterpret_cast<Menus*>(menus_page);

    bool done = false;
    while (!done)
    {
        app.buffers.update();

        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT){
                done = true;
            }
            else if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(app.window)){
                done = true;
            }
        }

        // ---- start render ---- //
        app.beginDraw();

        // when new frame ready
        if(app.buffers.new_frame())
        {
            FrameBuffer fb = app.buffers.getBuffer();

            app.console->info(fb.framerate);
            // execute shaders and run image processing on new frame here

            // render the viewport shaders 
            viewport->process();
        }

        // show the viewport 
        viewport->show();

        // show the overlays ( crosshairs, thirds, etc.. )
        overlays->show();

        // show the menus / controls 
        menus->show();

        // ---- end render ----- //
        app.endDraw();
    }

    app.cleanup();
    return 0;
}
