#include "Window.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <stdexcept>

Window::Window(int width, int height, const char* title) {
    if (!glfwInit()) throw std::runtime_error("GLFW init failed");
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    window_ = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (!window_) throw std::runtime_error("GLFW window creation failed");
}

Window::~Window() {
    if (window_) {
        glfwDestroyWindow(window_);
        glfwTerminate();
        window_ = nullptr;
    }
}

GLFWwindow* Window::getNativeWindow() const { return window_; }

void Window::pollEvents() { glfwPollEvents(); }

bool Window::shouldClose() const { return glfwWindowShouldClose(window_); }

void Window::setTitle(const std::string& title) { glfwSetWindowTitle(window_, title.c_str()); }

double Window::getTime() const { return glfwGetTime(); }
