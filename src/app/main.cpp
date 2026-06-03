#include "app/Application.hpp"

#include "core/Log.hpp"

int main() 
{
    macad::app::Application app;

    if (!app.init()) 
    {
        MACAD_LOG_ERROR("Application failed to initialize");
        return 1;
    }
    app.run();
    app.shutdown();
    return 0;
}
