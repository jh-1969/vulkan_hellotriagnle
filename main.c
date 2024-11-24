#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <strings.h>
#include <tgmath.h>
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

const uint8_t globalDeviceExtensionCount = 1;
const char* globalDeviceExtensions[] = {
  VK_KHR_SWAPCHAIN_EXTENSION_NAME
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
  VkSwapchainKHR swapChain;
  VkImage* swapChainImages;
  VkFormat swapChainImageFormat;
  VkExtent2D swapChainExtent;
} typedef App;

void app_run(App* app);
//------------------------------------
void app_private_init_window(App *app);
//------------------------------------
void app_private_init_vulkan(App *app);

void app_private_init_vulkan_create_instance(App *app);
bool app_private_init_vulkan_create_instance_check_layer_support();

void app_private_init_vulkan_setup_debug_messenger(App *app);

void app_private_init_vulkan_create_surface(App *app);

void app_private_init_vulkan_pick_device(App *app);
bool app_private_init_vulkan_pick_device_check_device_extensions(VkPhysicalDevice device);

void app_private_init_vulkan_create_logical_device(App *app);

void app_private_init_vulkan_create_swap_chain(App* app);
VkSurfaceFormatKHR app_private_init_vulkan_create_swap_chain_choose_format(VkSurfaceFormatKHR *availableFormats, uint32_t count);
VkPresentModeKHR app_private_init_vulkan_create_swap_chain_choose_present_mode(VkPresentModeKHR *availablePresentModes, uint32_t count);
VkExtent2D app_private_init_vulkan_create_swap_chain_choose_swap_extend(VkSurfaceCapabilitiesKHR *capabilities, GLFWwindow* window);
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



struct {
  VkSurfaceCapabilitiesKHR capabilities;
  uint32_t formatsCount;
  VkSurfaceFormatKHR* formats;
  uint32_t presentModesCount;
  VkPresentModeKHR* presentModes;
} typedef SwapChainSupportDetails;

SwapChainSupportDetails swap_chain_support_details_query(VkPhysicalDevice device, VkSurfaceKHR surface);
void swap_chain_support_details_free(SwapChainSupportDetails details);


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
  app_private_init_vulkan_create_swap_chain(app);
}

void app_private_init_vulkan_create_instance(App* app) {
  if(globalValidationLayersEnabled
     && !app_private_init_vulkan_create_instance_check_layer_support()
  ){
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

  /* ----- listing extensions
  uint32_t extensionCount = 0;
  vkEnumerateInstanceExtensionProperties(NULL, &extensionCount, NULL);

  VkExtensionProperties* extensions = calloc(extensionCount, sizeof(VkExtensionProperties));
  CHECK_ALLOC_FOR_NULL(extensions);
  vkEnumerateInstanceExtensionProperties(NULL, &extensionCount, extensions);

  printf("available extensions:\n");
  for (int i = 0; i < extensionCount; i++)
    printf("  %s\n", extensions[i].extensionName);
  */

  free(debugGlfwExtensions);
  //free(extensions);
}

bool app_private_init_vulkan_create_instance_check_layer_support() {
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

  bool devicePicked = false;

  for(int i = 0; i < deviceCount; i++) {
    QueueFamilyIndices indices = queue_families_find(devices[i], app->surface);

    SwapChainSupportDetails details = swap_chain_support_details_query(devices[i], app->surface);

    if(indices.isComplete
       && app_private_init_vulkan_pick_device_check_device_extensions(devices[i])
       && details.formatsCount != 0 && details.presentModesCount != 0
    ){
      app->physicalDevice = devices[i];
      devicePicked = true;
      break;
    }
    swap_chain_support_details_free(details);
  }

  if(!devicePicked) {
    printf("suitable device not found\n");
    exit(1);
  }

  free(devices);
}

bool app_private_init_vulkan_pick_device_check_device_extensions(VkPhysicalDevice device) {
  uint32_t availableExtensionsCount;
  vkEnumerateDeviceExtensionProperties(device, NULL, &availableExtensionsCount, NULL);

  VkExtensionProperties* availableExtensions = calloc(availableExtensionsCount, sizeof(VkExtensionProperties));
  CHECK_ALLOC_FOR_NULL(availableExtensions);
  vkEnumerateDeviceExtensionProperties(device, NULL, &availableExtensionsCount, availableExtensions);

  for (int i = 0; i < globalDeviceExtensionCount; i++) {
    bool extensionFound = false;

    for(int j = 0; j < availableExtensionsCount; j++) {
      if(strcmp(globalDeviceExtensions[i], availableExtensions[j].extensionName) == 0) {
        extensionFound = true;
        break;
      }
    }
    if(!extensionFound) {
      free(availableExtensions);
      return false;
    }
  }
  if(availableExtensions != NULL)
    free(availableExtensions);

  return true;
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

  createInfo.enabledExtensionCount = globalDeviceExtensionCount;
  createInfo.ppEnabledExtensionNames = globalDeviceExtensions;

  if(globalValidationLayersEnabled) {
    createInfo.enabledLayerCount = globalValidationLayersCount;
    createInfo.ppEnabledLayerNames = globalValidationLayers;
  }
  else
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

void app_private_init_vulkan_create_swap_chain(App *app) {
  SwapChainSupportDetails swapChainSupport = swap_chain_support_details_query(app->physicalDevice, app->surface);

  VkSurfaceFormatKHR surfaceFormat = app_private_init_vulkan_create_swap_chain_choose_format(swapChainSupport.formats, swapChainSupport.formatsCount);
  VkPresentModeKHR presentMode = app_private_init_vulkan_create_swap_chain_choose_present_mode(swapChainSupport.presentModes, swapChainSupport.presentModesCount);
  VkExtent2D extent = app_private_init_vulkan_create_swap_chain_choose_swap_extend(&swapChainSupport.capabilities, app->window);

  uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

  if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
    imageCount = swapChainSupport.capabilities.maxImageCount;
  }

  VkSwapchainCreateInfoKHR createInfo;
  createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  createInfo.surface = app->surface;
  createInfo.minImageCount = imageCount;
  createInfo.imageFormat = surfaceFormat.format;
  createInfo.imageColorSpace = surfaceFormat.colorSpace;
  createInfo.imageExtent = extent;
  createInfo.imageArrayLayers = 1;
  createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

  QueueFamilyIndices indices = queue_families_find(app->physicalDevice, app->surface);
  uint32_t queueFamilyIndices[] = {indices.graphicsFamily, indices.presentFamily};

  if(indices.graphicsFamily != indices.presentFamily) {
    createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    createInfo.queueFamilyIndexCount = 2;
    createInfo.pQueueFamilyIndices = queueFamilyIndices;
  } else {
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.queueFamilyIndexCount = 0;
    createInfo.pQueueFamilyIndices = NULL;
  }
  createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
  createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  createInfo.presentMode = presentMode;
  createInfo.clipped = VK_TRUE;
  createInfo.oldSwapchain = VK_NULL_HANDLE;
  createInfo.pNext = NULL;
  createInfo.flags = 0;

  if(vkCreateSwapchainKHR(app->device, &createInfo, NULL, &app->swapChain) != VK_SUCCESS) {
    printf("failed to create swapchain\n");
    exit(1);
  }

  vkGetSwapchainImagesKHR(app->device, app->swapChain, &imageCount, NULL);
  app->swapChainImages = calloc(imageCount, sizeof(VkImage));
  CHECK_ALLOC_FOR_NULL(app->swapChainImages);
  vkGetSwapchainImagesKHR(app->device, app->swapChain, &imageCount, app->swapChainImages);

  app->swapChainImageFormat = surfaceFormat.format;
  app->swapChainExtent = extent;

  swap_chain_support_details_free(swapChainSupport);
}

VkSurfaceFormatKHR app_private_init_vulkan_create_swap_chain_choose_format(VkSurfaceFormatKHR *availableFormats, uint32_t count) {
  for(int i = 0; i < count; i++) {
    if(availableFormats[i].format == VK_FORMAT_B8G8R8A8_SRGB && availableFormats[i].colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR) {
      return availableFormats[i];
    }
  }
  return availableFormats[0];
}

VkPresentModeKHR app_private_init_vulkan_create_swap_chain_choose_present_mode(VkPresentModeKHR *availablePresentModes, uint32_t count) {
  for(int i = 0; i < count; i++) {
    if(availablePresentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
      return availablePresentModes[i];
    }
  }
  return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D app_private_init_vulkan_create_swap_chain_choose_swap_extend(VkSurfaceCapabilitiesKHR *capabilities, GLFWwindow* window) {
  if(capabilities->currentExtent.width != UINT32_MAX) {
    return capabilities->currentExtent;
  } else {
    int width = 0, height = 0;
    glfwGetFramebufferSize(window, &width, &height);

    VkExtent2D actualExtend = {
      fmin(capabilities->maxImageExtent.width, fmax(width, capabilities->minImageExtent.width)),
      fmax(capabilities->maxImageExtent.height, fmax(height, capabilities->minImageExtent.height))
    };
    return actualExtend;
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
  vkDestroySwapchainKHR(app->device, app->swapChain, NULL);
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



SwapChainSupportDetails swap_chain_support_details_query(VkPhysicalDevice device, VkSurfaceKHR surface) {
  SwapChainSupportDetails details;

  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

  vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &details.formatsCount, NULL);

  if(details.formatsCount != 0) {
    details.formats = calloc(details.formatsCount, sizeof(VkSurfaceFormatKHR));
    CHECK_ALLOC_FOR_NULL(details.formats);
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &details.formatsCount, details.formats);
  }

  vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &details.presentModesCount, NULL);

  if(details.presentModesCount != 0) {
    details.presentModes = calloc(details.presentModesCount, sizeof(VkPresentModeKHR));
    CHECK_ALLOC_FOR_NULL(details.presentModes);
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &details.presentModesCount, details.presentModes);
  }

  return details;
}

void swap_chain_support_details_free(SwapChainSupportDetails details) {
  free(details.formats);
  free(details.presentModes);
}



int main() {
  App app;

  app_run(&app);
}
