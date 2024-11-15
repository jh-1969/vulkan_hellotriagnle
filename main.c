#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

#ifdef NDEBUG
const bool globalValidationLayersEnabled = false;
#else
const bool globalValidationLayersEnabled = true;
#endif



const uint8_t globalValidationLayersCount = 1;
const char *globalValidationLayers[] = {
  "VK_LAYER_KHRONOS_validation"
};



struct {
  GLFWwindow *window;
  VkInstance instance;
  VkDebugUtilsMessengerEXT debugMessenger;
} typedef App;



void app_run(App* app);
//------------------------------------
void app_private_init_window(App *app);
//------------------------------------
void app_private_init_vulkan(App *app);

void app_private_init_vulkan_create_instance(App* app);
bool app_private_init_vulkan_create_instance_check_layer_support(const char** layers, uint8_t layerCount);

void app_private_init_vulkan_setup_debug_messenger(App *app);

VkDebugUtilsMessengerCreateInfoEXT app_private_populate_debug_messenger_info();
//------------------------------------
VKAPI_ATTR VkBool32 VKAPI_CALL app_private_debug_callback(
  VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
  VkDebugUtilsMessageTypeFlagsEXT messageType,
  const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
  void* pUserData);
//------------------------------------
void app_private_main_loop(App* app);
//------------------------------------
void app_private_cleanup(App* app);



void app_run(App* app) {
  app_private_init_window(app);
  app_private_init_vulkan(app);
  app_private_main_loop(app);
  app_private_cleanup(app);
}

void app_private_init_window(App* app) {
  glfwInit();

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

  app->window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Vulkan", NULL, NULL);
}

void app_private_init_vulkan(App* app) {
  app_private_init_vulkan_create_instance(app);

  if(globalValidationLayersEnabled)
    app_private_init_vulkan_setup_debug_messenger(app);
}

void app_private_init_vulkan_create_instance(App* app) {
  if(globalValidationLayersEnabled &&
     !app_private_init_vulkan_create_instance_check_layer_support(globalValidationLayers, globalValidationLayersCount))
  {
    printf("validation layers unavailable\n");
    exit(1);
  }

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

  uint32_t glfwExtensionsCount = 0;
  const char **glfwExtensions;

  glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionsCount);

  const char **debugGlfwExtensions = calloc(glfwExtensionsCount + 1, sizeof(char *));

  VkDebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo = app_private_populate_debug_messenger_info();
  if (globalValidationLayersEnabled) {
    for(int i = 0; i < glfwExtensionsCount; i++){
      debugGlfwExtensions[i] = glfwExtensions[i];
    }
    debugGlfwExtensions[glfwExtensionsCount] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;

    createInfo.enabledLayerCount = globalValidationLayersCount;
    createInfo.ppEnabledLayerNames = globalValidationLayers;
    createInfo.enabledExtensionCount = glfwExtensionsCount + 1;
    createInfo.ppEnabledExtensionNames = debugGlfwExtensions;
    createInfo.pNext = &debugMessengerCreateInfo;
  } else {
    createInfo.enabledLayerCount = 0;
    createInfo.enabledExtensionCount = glfwExtensionsCount;
    createInfo.ppEnabledExtensionNames = glfwExtensions;
    createInfo.pNext = NULL;
  }

  if(vkCreateInstance(&createInfo, NULL, &app->instance) != VK_SUCCESS) {
    printf("failed to create instance\n");
    exit(1);
  }

  uint32_t extensionCount = 0;
  vkEnumerateInstanceExtensionProperties(NULL, &extensionCount, NULL);

  VkExtensionProperties* extensions = calloc(extensionCount, sizeof(VkExtensionProperties));
  vkEnumerateInstanceExtensionProperties(NULL, &extensionCount, extensions);

  printf("available extensions:\n");
  for (int i = 0; i < extensionCount; i++)
    printf("  %s\n", extensions[i].extensionName);

  free(debugGlfwExtensions);
  free(extensions);
}

bool app_private_init_vulkan_create_instance_check_layer_support(const char **layers, uint8_t layerCount) {
  uint32_t availableLayersCount;
  vkEnumerateInstanceLayerProperties(&availableLayersCount, NULL);

  VkLayerProperties* availableLayers = calloc(availableLayersCount, sizeof(VkLayerProperties));
  vkEnumerateInstanceLayerProperties(&availableLayersCount, availableLayers);

  for(int i = 0; i < globalValidationLayersCount; i++) {
    bool layerFound = false;

    for(int j = 0; j < availableLayersCount; j++) {
      if(strcmp(globalValidationLayers[i], availableLayers[j].layerName) == 0) {
        layerFound = true;
        break;
      }
    }
    if(!layerFound) {
      free(availableLayers);
      return false;
    }
  }
  if(availableLayers != NULL)
    free(availableLayers);

  return true;
}

void app_private_init_vulkan_setup_debug_messenger(App* app) {
  VkDebugUtilsMessengerCreateInfoEXT createInfo = app_private_populate_debug_messenger_info();

  PFN_vkCreateDebugUtilsMessengerEXT func = (PFN_vkCreateDebugUtilsMessengerEXT)
    vkGetInstanceProcAddr(app->instance, "vkCreateDebugUtilsMessengerEXT");

  if (func != NULL) {
    func(app->instance, &createInfo, NULL, &app->debugMessenger);
  } else {
    printf("failed to set up messenger\n");
    exit(1);
  }
}

VkDebugUtilsMessengerCreateInfoEXT app_private_populate_debug_messenger_info() {
  VkDebugUtilsMessengerCreateInfoEXT createInfo;
  createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  createInfo.pfnUserCallback = app_private_debug_callback;
  createInfo.flags = 0;
  createInfo.pNext = NULL;
  return createInfo;
}

VKAPI_ATTR VkBool32 VKAPI_CALL app_private_debug_callback(
  VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
  VkDebugUtilsMessageTypeFlagsEXT messageType,
  const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
  void *pUserData) {

  printf("%s", pCallbackData->pMessage);
  printf("\n");

  return VK_FALSE;
}

void app_private_main_loop(App* app) {
  while(!glfwWindowShouldClose(app->window)) {
    glfwPollEvents();
  }
}

void app_private_cleanup(App* app) {
  PFN_vkDestroyDebugUtilsMessengerEXT func = (PFN_vkDestroyDebugUtilsMessengerEXT)
    vkGetInstanceProcAddr(app->instance, "vkDestroyDebugUtilsMessengerEXT");

  if(func != NULL)
    func(app->instance, app->debugMessenger, NULL);
  else {
    printf("failed to load messenger destroy function\n");
    exit(1);
  }

  vkDestroyInstance(app->instance, NULL);

  glfwDestroyWindow(app->window);
  glfwTerminate();
}



int main() {
  App app;

  app_run(&app);
}
