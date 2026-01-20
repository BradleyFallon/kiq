#ifdef __APPLE__

#include "StandaloneApp.h"
#include <iostream>
#include <csignal>

using namespace KickDrum;

// Global pointer for signal handler
StandaloneApp* g_app = nullptr;

// Signal handler for Ctrl+C
void signalHandler(int signal) {
    if (signal == SIGINT && g_app != nullptr) {
        std::cout << std::endl;
        std::cout << "Received interrupt signal..." << std::endl;
        g_app->shutdown();
        exit(0);
    }
}

int main(int argc, char* argv[]) {
    // Create application
    StandaloneApp app;
    g_app = &app;
    
    // Register signal handler
    std::signal(SIGINT, signalHandler);
    
    // Initialize
    if (!app.initialize()) {
        std::cerr << "Failed to initialize application" << std::endl;
        return 1;
    }
    
    // Run main loop
    app.run();
    
    // Shutdown
    app.shutdown();
    
    return 0;
}

#else

#include <iostream>

int main(int argc, char* argv[]) {
    std::cout << "Standalone application is only available on macOS" << std::endl;
    return 1;
}

#endif // __APPLE__
