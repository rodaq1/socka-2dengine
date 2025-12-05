#include "EditorApp.h"
#include <iostream>

int main(int argc, char* argv[]) {
    try {
        EditorApp app;
        app.run();
    } catch (const std::exception& e) {
        std::cerr << "unhandled exception: " << e.what() << std::endl;

        std::cin.get();
        return 1;
    }
    return 0;
}