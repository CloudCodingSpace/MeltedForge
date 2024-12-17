#include "mf/backend/backend_types.h"
#include <mf.h>

#include <stdio.h>

#include <GLFW/glfw3.h>

#define WIDTH  800
#define HEIGHT 600
#define TITLE "MFTest"


int main(int argc, char** argv) {
  if(!glfwInit()) {
    printf("Failed to init GLFW!");
    return 1;
  }

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // To enable vulkan support
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
  glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

  GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, TITLE, NULL, NULL);
  if(!window) {
    printf("Failed to create the window!");
    return 1;
  }

  mfInitialize(MFBCKND_TYPE_VULKAN);

  glfwShowWindow(window);
  while(!glfwWindowShouldClose(window)) {
    

    glfwPollEvents();
  }

  mfShutdown();
  glfwDestroyWindow(window);
  glfwTerminate();
  return 0;
}
