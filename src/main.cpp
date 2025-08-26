#include "App.h"
#include <iostream>

int main() {
    try {
        App app(1280, 720, "Aurora3D Starter");
        app.run();
    } catch (const std::exception& e) {
        std::cerr << "Fatal: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
