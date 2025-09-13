#include <iostream>
#include "flint/app.hpp"

int main(int argc, char* argv[]) {
    try {
        flint::App app;
        if (!app.Initialize()) {
            std::cerr << "Failed to initialize app" << std::endl;
            return 1;
        }
        app.Run();
        app.Terminate();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
