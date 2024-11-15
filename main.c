#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <vulkan/vulkan_core.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600



struct {
  GLFWwindow *window;
  VkInstance instance;
} typedef App;

int app_run(App* app);

int app_private_init_window(App *app);

int app_private_init_vulkan(App *app);
int app_private_init_vulkan_create_instance(App* app);

int app_private_main_loop(App* app);

int app_private_cleanup(App* app);



int app_run(App* app) {
  if(app_private_init_window(app))
    return 1;
  if(app_private_init_vulkan(app))
    return 1;
  if(app_private_main_loop(app))
    return 1;
  if(app_private_cleanup(app))
    return 1;

  return 0;
}

int app_private_init_window(App* app) {
  glfwInit();

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

  app->window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Vulkan", NULL, NULL);

  return 0;
}

int app_private_init_vulkan(App* app) {
  if(app_private_init_vulkan_create_instance(app))
    return 1;

  return 0;
}

int app_private_init_vulkan_create_instance(App* app) {
  VkApplicationInfo appInfo;
  appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.pApplicationName = "Hello Triangle";
  appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.pEngineName = "No Engine";
  appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.apiVersion = VK_API_VERSION_1_0;
  appInfo.pNext = NULL;

  VkInstanceCreateInfo createInfo;
  createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  createInfo.pApplicationInfo = &appInfo;

  uint32_t glfwExtensionCount = 0;
  const char **glfwExtensions;

  glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

  createInfo.enabledExtensionCount = glfwExtensionCount;
  createInfo.ppEnabledExtensionNames = glfwExtensions;

  createInfo.enabledLayerCount = 0;
  createInfo.pNext = NULL;

  if(vkCreateInstance(&createInfo, NULL, &app->instance) != VK_SUCCESS) {
    printf("failed to create instance!");
    return 1;
  }

  uint32_t extensionCount = 0;
  vkEnumerateInstanceExtensionProperties(NULL, &extensionCount, NULL);

  VkExtensionProperties extensions[extensionCount];
  vkEnumerateInstanceExtensionProperties(NULL, &extensionCount, extensions);

  printf("available extensions:\n");
  for(int i = 0; i < extensionCount; i++)
    printf("  %s\n", extensions[i].extensionName);

  return 0;
}

int app_private_main_loop(App* app) {
  while(!glfwWindowShouldClose(app->window)) {
    glfwPollEvents();
  }
  return 0;
}

int app_private_cleanup(App* app) {
  vkDestroyInstance(app->instance, NULL);

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
