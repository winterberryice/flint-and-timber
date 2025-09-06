#include <iostream>

#include "flint/app.hpp"

int main()
{
    flint::App app;

    if (!app.Initialize())
    {
        std::cerr << "Failed to initialize app" << std::endl;
        return -1;
    }

    app.Run();
    app.Terminate();

    return 0;
}
