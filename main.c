#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600



struct {
  GLFWwindow *window;
} typedef App;

int app_run(App* app);

int app_private_init_window(App* app);
int app_private_init_vulkan();
int app_private_main_loop(App* app);
int app_private_cleanup(App* app);



int app_run(App* app) {
  app_private_init_window(app);
  if(app_private_init_vulkan())
    return 1;
  app_private_main_loop(app);
  app_private_cleanup(app);

  return 0;
}

int app_private_init_window(App* app) {
  glfwInit();

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

  app->window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Vulkan", NULL, NULL);

  return 0;
}

int app_private_init_vulkan() {
  return 0;
}

int app_private_main_loop(App* app) {
  while(!glfwWindowShouldClose(app->window)) {
    glfwPollEvents();
  }
  return 0;
}

int app_private_cleanup(App* app) {
  glfwDestroyWindow(app->window);
  glfwTerminate();

  return 0;
}



int main() {
  App app;

  if(app_run(&app)) {
    perror("run fail");
    return 1;
  } else {
    return 0;
  }
}
