#include <iostream>

#include "flint/app.h"

int main()
{
    flint::App app;

    try
    {
        app.run();
    }
    catch (const std::exception &e)
    {
        std::cerr << "An error occurred: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
