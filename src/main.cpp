#include <iostream>

#include "flint/main.h"

int main()
{
    try
    {
        flint::Application app;
        app.run();
    }
    catch (const std::exception &e)
    {
        std::cerr << "An unhandled exception occurred: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}