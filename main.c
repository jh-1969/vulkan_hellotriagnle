#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
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
  uint32_t swapChainImagesCount;
  VkFormat swapChainImageFormat;
  VkExtent2D swapChainExtent;
  VkImageView* swapChainImageViews;
  VkRenderPass renderPass;
  VkPipelineLayout pipelineLayout;
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
VkExtent2D app_private_init_vulkan_create_swap_chain_choose_swap_extend(VkSurfaceCapabilitiesKHR *capabilities, GLFWwindow *window);

void app_private_init_vulkan_create_image_views(App *app);

void app_private_init_vulkan_create_render_pass(App *app);

void app_private_init_vulkan_create_graphics_pipeline(App* app);
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



static uint8_t *helper_read_file(const char *filename, size_t *filesize);



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
  app_private_init_vulkan_create_image_views(app);
  app_private_init_vulkan_create_graphics_pipeline(app);
  app_private_init_vulkan_create_render_pass(app);
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
  app->swapChainImagesCount = imageCount;
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

void app_private_init_vulkan_create_image_views(App* app) {
  app->swapChainImageViews = calloc(app->swapChainImagesCount, sizeof(VkImageView));
  CHECK_ALLOC_FOR_NULL(app->swapChainImageViews);

  for(int i = 0; i < app->swapChainImagesCount; i++) {
    VkImageViewCreateInfo createInfo;
    createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    createInfo.image = app->swapChainImages[i];
    createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    createInfo.format = app->swapChainImageFormat;
    createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    createInfo.subresourceRange.baseMipLevel = 0;
    createInfo.subresourceRange.levelCount = 1;
    createInfo.subresourceRange.baseArrayLayer = 0;
    createInfo.subresourceRange.layerCount = 1;
    createInfo.pNext = NULL;
    createInfo.flags = 0;

    if(vkCreateImageView(app->device, &createInfo, NULL, &app->swapChainImageViews[i]) != VK_SUCCESS) {
      printf("failed to create image view\n");
      exit(1);
    }
  }
}

void app_private_init_vulkan_create_render_pass(App *app) {
  VkAttachmentDescription colorAttachment;
  colorAttachment.format = app->swapChainImageFormat;
  colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
  colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
  colorAttachment.flags = 0;

  VkAttachmentReference colorAttachmentRef;
  colorAttachmentRef.attachment = 0;
  colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpass;
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &colorAttachmentRef;
  subpass.pDepthStencilAttachment = NULL;
  subpass.preserveAttachmentCount = 0;
  subpass.flags = 0;

  VkRenderPassCreateInfo renderPassInfo;
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassInfo.attachmentCount = 1;
  renderPassInfo.pAttachments = &colorAttachment;
  renderPassInfo.subpassCount = 1;
  renderPassInfo.pSubpasses = &subpass;
  renderPassInfo.dependencyCount = 0;
  renderPassInfo.pNext = NULL;
  renderPassInfo.flags = 0;

  if(vkCreateRenderPass(app->device, &renderPassInfo, NULL, &app->renderPass) != VK_SUCCESS) {
    printf("failed to create render pass\n");
    exit(1);
  }
}

void app_private_init_vulkan_create_graphics_pipeline(App *app) {
  size_t vertCodeSize, fragCodeSize;
  uint8_t* vertShaderCode = helper_read_file("shaders/vert.spv", &vertCodeSize);
  uint8_t* fragShaderCode = helper_read_file("shaders/frag.spv", &fragCodeSize);

  if(vertShaderCode == NULL || fragShaderCode == NULL) {
    printf("failed to load shader code\n");
    exit(1);
  }

  VkShaderModuleCreateInfo vertModuleInfo;
  vertModuleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  vertModuleInfo.codeSize = vertCodeSize;
  vertModuleInfo.pCode = (uint32_t*)vertShaderCode;
  vertModuleInfo.pNext = NULL;
  vertModuleInfo.flags = 0;

  VkShaderModuleCreateInfo fragModuleInfo;
  fragModuleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  fragModuleInfo.codeSize = fragCodeSize;
  fragModuleInfo.pCode = (uint32_t*)fragShaderCode;
  fragModuleInfo.pNext = NULL;
  fragModuleInfo.flags = 0;

  VkShaderModule vertModule, fragModule;
  if (vkCreateShaderModule(app->device, &vertModuleInfo, NULL, &vertModule) != VK_SUCCESS
      || vkCreateShaderModule(app->device, &fragModuleInfo, NULL, &fragModule) != VK_SUCCESS
  ){
    printf("failed to create shader module\n");
    exit(1);
  }
  VkPipelineShaderStageCreateInfo vertShaderStageInfo;
  vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
  vertShaderStageInfo.module = vertModule;
  vertShaderStageInfo.pName = "main";
  vertShaderStageInfo.pSpecializationInfo = NULL;
  vertShaderStageInfo.pNext = NULL;
  vertShaderStageInfo.flags = 0;

  VkPipelineShaderStageCreateInfo fragShaderStageInfo;
  fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  fragShaderStageInfo.module = fragModule;
  fragShaderStageInfo.pName = "main";
  fragShaderStageInfo.pSpecializationInfo = NULL;
  fragShaderStageInfo.pNext = NULL;
  fragShaderStageInfo.flags = 0;

  VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

  VkPipelineVertexInputStateCreateInfo vertexInputInfo;
  vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertexInputInfo.vertexBindingDescriptionCount = 0;
  vertexInputInfo.pVertexBindingDescriptions = NULL;
  vertexInputInfo.vertexAttributeDescriptionCount = 0;
  vertexInputInfo.pVertexAttributeDescriptions = NULL;
  vertexInputInfo.pNext = NULL;
  vertexInputInfo.flags = 0;

  VkPipelineInputAssemblyStateCreateInfo inputAssembly;
  inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  inputAssembly.primitiveRestartEnable = VK_FALSE;
  inputAssembly.pNext = NULL;
  inputAssembly.flags = 0;

  VkViewport viewport;
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = (float)app->swapChainExtent.width;
  viewport.height = (float)app->swapChainExtent.height;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;

  VkRect2D scissor;
  VkOffset2D offset = {0, 0};
  scissor.offset = offset;
  scissor.extent = app->swapChainExtent;

  VkPipelineViewportStateCreateInfo viewportState;
  viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewportState.viewportCount = 1;
  viewportState.pViewports = &viewport;
  viewportState.scissorCount = 1;
  viewportState.pScissors = &scissor;
  viewportState.pNext = NULL;
  viewportState.flags = 0;

  VkPipelineRasterizationStateCreateInfo rasterizer;
  rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizer.depthClampEnable = VK_FALSE;
  rasterizer.rasterizerDiscardEnable = VK_FALSE;
  rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
  rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
  rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
  rasterizer.depthBiasEnable = VK_FALSE;
  rasterizer.depthBiasConstantFactor = 0.0f;
  rasterizer.depthBiasClamp = 0.0f;
  rasterizer.depthBiasSlopeFactor = 0.0f;
  rasterizer.pNext = NULL;
  rasterizer.flags = 0;

  VkPipelineMultisampleStateCreateInfo multisampling;
  multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisampling.sampleShadingEnable = VK_FALSE;
  multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
  multisampling.minSampleShading = 1.0f;
  multisampling.pSampleMask = NULL;
  multisampling.alphaToCoverageEnable = VK_FALSE;
  multisampling.alphaToOneEnable = VK_FALSE;
  multisampling.pNext = NULL;
  multisampling.flags = 0;

  VkPipelineColorBlendAttachmentState colorBlendAttachment;
  colorBlendAttachment.colorWriteMask =
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  colorBlendAttachment.blendEnable = VK_FALSE;
  colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
  colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
  colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
  colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
  colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
  colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

  VkPipelineColorBlendStateCreateInfo colorBlending;
  colorBlending.sType =
      VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  colorBlending.logicOpEnable = VK_FALSE;
  colorBlending.logicOp = VK_LOGIC_OP_COPY;
  colorBlending.attachmentCount = 1;
  colorBlending.pAttachments = &colorBlendAttachment;
  colorBlending.blendConstants[0] = 0.0f;
  colorBlending.blendConstants[1] = 0.0f;
  colorBlending.blendConstants[2] = 0.0f;
  colorBlending.blendConstants[3] = 0.0f;
  colorBlending.pNext = NULL;
  colorBlending.flags = 0;

  VkPipelineLayoutCreateInfo pipelineLayoutInfo;
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount = 0;
  pipelineLayoutInfo.pSetLayouts = NULL;
  pipelineLayoutInfo.pushConstantRangeCount = 0;
  pipelineLayoutInfo.pPushConstantRanges = NULL;
  pipelineLayoutInfo.pNext = NULL;
  pipelineLayoutInfo.flags = 0;

  if(vkCreatePipelineLayout(app->device, &pipelineLayoutInfo, NULL, &app->pipelineLayout) != VK_SUCCESS) {
    printf("failed to create pipeline layout\n");
    exit(1);
  }

  vkDestroyShaderModule(app->device, fragModule, NULL);
  vkDestroyShaderModule(app->device, vertModule, NULL);

  free(vertShaderCode);
  free(fragShaderCode);
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
  vkDestroyPipelineLayout(app->device, app->pipelineLayout, NULL);
  vkDestroyRenderPass(app->device, app->renderPass, NULL);

  for(int i = 0; i < app->swapChainImagesCount; i++) {
    vkDestroyImageView(app->device, app->swapChainImageViews[i], NULL);
  }

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



static uint8_t *helper_read_file(const char *filename, size_t* filesize) {
  FILE* fp = fopen(filename, "rb");

  if(fp != NULL) {
    fseek(fp, 0L, SEEK_END);

    size_t fsize = ftell(fp);
    uint8_t *buffer = calloc(fsize, 1);
    CHECK_ALLOC_FOR_NULL(buffer);

    fseek(fp, 0L, SEEK_SET);
    fread(buffer, fsize, 1, fp);

    *filesize = fsize;
    return buffer;
  } else
    return NULL;
}



int main() {
  App app;

  app_run(&app);
}
