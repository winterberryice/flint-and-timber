#include <iostream>

#include "flint/app.hpp"

int main()
{
    try
    {
        flint::App app;
        app.run();
    }
    catch (const std::exception &e)
    {
        std::cerr << "An unhandled exception occurred: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
