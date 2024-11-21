#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <strings.h>
#include <vulkan/vulkan_core.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

#define CHECK_ALLOC_FOR_NULL(x) if((x) == NULL) {printf("could not allocate memory\n"); exit(1);}

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
  VkPhysicalDevice physicalDevice;
  VkPhysicalDeviceFeatures deviceFeatures;
  VkDevice device;
  VkQueue graphicsQueue;
  VkQueue presentQueue;
  VkSurfaceKHR surface;
} typedef App;

void app_run(App* app);
//------------------------------------
void app_private_init_window(App *app);
//------------------------------------
void app_private_init_vulkan(App *app);

void app_private_init_vulkan_create_instance(App *app);
bool app_private_init_vulkan_create_instance_check_layer_support(const char **layers, uint8_t layerCount);

void app_private_init_vulkan_setup_debug_messenger(App *app);

void app_private_init_vulkan_create_surface(App *app);

void app_private_init_vulkan_pick_device(App *app);

void app_private_init_vulkan_create_logical_device(App *app);
//------------------------------------
VkDebugUtilsMessengerCreateInfoEXT app_private_populate_debug_messenger_info();
//------------------------------------
VKAPI_ATTR VkBool32 VKAPI_CALL app_private_debug_callback(
  VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
  VkDebugUtilsMessageTypeFlagsEXT messageType,
  const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
  void *pUserData);
//------------------------------------
void app_private_main_loop(App *app);
//------------------------------------
void app_private_cleanup(App *app);



struct {
  uint32_t graphicsFamily;
  uint32_t presentFamily;
  bool isComplete;
} typedef QueueFamilyIndices;

const uint8_t globalQueueFamilyIndicesFieldCount = 2;

QueueFamilyIndices queue_families_find(VkPhysicalDevice device, VkSurfaceKHR surface);


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

  app_private_init_vulkan_create_surface(app);
  app_private_init_vulkan_pick_device(app);
  app_private_init_vulkan_create_logical_device(app);
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
  CHECK_ALLOC_FOR_NULL(debugGlfwExtensions);

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
  CHECK_ALLOC_FOR_NULL(extensions);
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
  CHECK_ALLOC_FOR_NULL(availableLayers);
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

void app_private_init_vulkan_setup_debug_messenger(App *app) {
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

void app_private_init_vulkan_create_surface(App *app) {
  if(glfwCreateWindowSurface(app->instance, app->window, NULL, &app->surface) != VK_SUCCESS) {
    printf("failed to create window");
    exit(1);
  }
}

void app_private_init_vulkan_pick_device(App *app) {
  uint32_t deviceCount = 0;
  vkEnumeratePhysicalDevices(app->instance, &deviceCount, NULL);

  if(deviceCount == 0) {
    printf("failed to find GPU with vulkan support");
    exit(1);
  }

  VkPhysicalDevice* devices = calloc(deviceCount, sizeof(VkPhysicalDevice));
  CHECK_ALLOC_FOR_NULL(devices);
  vkEnumeratePhysicalDevices(app->instance, &deviceCount, devices);

  for(int i = 0; i < deviceCount; i++) {
    QueueFamilyIndices indices = queue_families_find(devices[i], app->surface);

    if(indices.isComplete) {
      app->physicalDevice = devices[i];
      break;
    }
  }
  free(devices);
}

void app_private_init_vulkan_create_logical_device(App *app) {
  QueueFamilyIndices indices = queue_families_find(app->physicalDevice, app->surface);

  uint8_t uniqueQueueFamilesCount = 0;
  uint32_t *uniqueQueueFamilies = malloc(sizeof(uint32_t));
  CHECK_ALLOC_FOR_NULL(uniqueQueueFamilies);

  if (indices.graphicsFamily == indices.presentFamily) {
    uniqueQueueFamilies[0] = indices.graphicsFamily;
    uniqueQueueFamilesCount = 1;
  } else {
    uniqueQueueFamilies = realloc(uniqueQueueFamilies, 2 * sizeof(uint32_t));
    CHECK_ALLOC_FOR_NULL(uniqueQueueFamilies);
    uniqueQueueFamilies[0] = indices.graphicsFamily;
    uniqueQueueFamilies[1] = indices.presentFamily;
    uniqueQueueFamilesCount = 2;
  }

  VkDeviceQueueCreateInfo *queueCreateInfos = calloc(globalQueueFamilyIndicesFieldCount, sizeof(VkDeviceQueueCreateInfo));
  CHECK_ALLOC_FOR_NULL(queueCreateInfos);

  float queuePriority = 1.0f;

  for(int i = 0; i < uniqueQueueFamilesCount; i++) {
    queueCreateInfos[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfos[i].queueFamilyIndex = uniqueQueueFamilies[i];
    queueCreateInfos[i].queueCount = 1;
    queueCreateInfos[i].pQueuePriorities = &queuePriority;
    queueCreateInfos[i].pNext = NULL;
    queueCreateInfos[i].flags = 0;
  }

  VkDeviceCreateInfo createInfo;
  createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

  createInfo.queueCreateInfoCount = uniqueQueueFamilesCount;
  createInfo.pQueueCreateInfos = queueCreateInfos;

  app->deviceFeatures = (VkPhysicalDeviceFeatures) {VK_FALSE};
  createInfo.pEnabledFeatures = &app->deviceFeatures;

  createInfo.enabledExtensionCount = 0;
  createInfo.enabledLayerCount = 0;

  createInfo.pNext = NULL;
  createInfo.flags = 0;

  if (vkCreateDevice(app->physicalDevice, &createInfo, NULL, &app->device) != VK_SUCCESS) {
    printf("failed to create logical device");
    exit(1);
  }

  vkGetDeviceQueue(app->device, indices.graphicsFamily, 0, &app->graphicsQueue);
  vkGetDeviceQueue(app->device, indices.presentFamily, 0, &app->presentQueue);

  free(uniqueQueueFamilies);
  free(queueCreateInfos);
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

  printf("\n");
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
  vkDestroyDevice(app->device, NULL);
  vkDestroySurfaceKHR(app->instance, app->surface, NULL);

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



QueueFamilyIndices queue_families_find(VkPhysicalDevice device, VkSurfaceKHR surface) {
  QueueFamilyIndices indices;
  indices.isComplete = false;

  uint32_t queueFamiliesCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamiliesCount, NULL);

  VkQueueFamilyProperties *queueFamilies = calloc(queueFamiliesCount, sizeof(VkQueueFamilyProperties));
  CHECK_ALLOC_FOR_NULL(queueFamilies);
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamiliesCount, queueFamilies);

  for(int i = 0; i < queueFamiliesCount; i++) {
    uint8_t conditionsMet = 0;

    VkBool32 presentSupport = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

    if(presentSupport) {
      indices.presentFamily = i;
      conditionsMet++;
    }

    if(queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      indices.graphicsFamily = i;
      conditionsMet++;
    }

    if(conditionsMet >= globalQueueFamilyIndicesFieldCount) {
      indices.isComplete = true;
      break;
    }
  }

  free(queueFamilies);

  return indices;
}



int main() {
  App app;

  app_run(&app);
}
