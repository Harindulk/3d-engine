#pragma once

#include <string>

struct GLFWwindow;

class Window {
public:
    Window(int width, int height, const char* title);
    ~Window();

    GLFWwindow* getNativeWindow() const;
    void pollEvents();
    bool shouldClose() const;
    void setTitle(const std::string& title);
    double getTime() const;
    bool wasResized() const;
    void clearResizedFlag();

private:
    GLFWwindow* window_ = nullptr;
    bool resized_ = false;
};
