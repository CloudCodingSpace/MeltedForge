#include "vkbcknd.h"
#include "slog/slog.h"
#include <GLFW/glfw3.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include <vulkan/vulkan_core.h>
#include <vulkan/vk_enum_string_helper.h>

#include <mf/core.h>

#define CHECKRESULT(result) check_result(result, __LINE__, __FILE__);
#define ARR_SIZE(arr) sizeof(arr)/sizeof(arr[0])

void check_result(VkResult result, int line, char* file) {
  if(result != VK_SUCCESS) {
    SLogger* logger = mfGetSLoggerHandle();
    char msg[32000];
    sprintf(msg, "The result is %s at line: %d of file: %s", string_VkResult(result), line, file);
    slogLogConsole(logger, SLOG_SEVERITY_ERROR, msg);
    exit(EXIT_FAILURE);
  }
}

bool check_dbg_exts(uint32_t count, const char** lyrs) {
  uint32_t lyrCount = 0;
  CHECKRESULT(vkEnumerateInstanceLayerProperties(&lyrCount, NULL))
  VkLayerProperties props[lyrCount];
  CHECKRESULT(vkEnumerateInstanceLayerProperties(&lyrCount, props))

  bool found = false;

  for(int i = 0; i < lyrCount; i++) {
    for(int j = 0; j < count; j++) {
      if(strcmp(props[i].layerName, lyrs[j]) == 0) {
        found = true;
        break;
      }
    }
  }

  return found;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL dbgCallbck(
                      VkDebugUtilsMessageSeverityFlagBitsEXT msgSeverity,
                      VkDebugUtilsMessageTypeFlagsEXT msgType,
                      const VkDebugUtilsMessengerCallbackDataEXT* callbckData,
                      void* usrData) {

  slogLogConsole(mfGetSLoggerHandle(), SLOG_SEVERITY_FATAL, (char*)((void*)callbckData->pMessage));

  return VK_FALSE;
} 

void mfVkBckndInit(MFVkBckndState* state) {
  SLogger* logger = mfGetSLoggerHandle();

#ifdef _DEBUG
  const char* dbgLyrs[] = {
    "VK_LAYER_KHRONOS_validation"
  };

  if(!check_dbg_exts(1, dbgLyrs)) {
    slogLogConsole(logger, SLOG_SEVERITY_ERROR, "Can't find the debug extensions!");
    exit(EXIT_FAILURE);
  }

#endif

  // Instance
  {
    VkApplicationInfo appInfo = {
      .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
      .pApplicationName = "MFApplication",
      .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
      .pEngineName = "MFRenderEngine",
      .engineVersion = VK_MAKE_VERSION(1, 0, 0),
      .apiVersion = VK_API_VERSION_1_0
    };
    
    // Getting the required extensions
    uint32_t extCount = 0;
    const char** exts = glfwGetRequiredInstanceExtensions(&extCount);

    VkInstanceCreateInfo info = {
      .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
      .pApplicationInfo = &appInfo,
      .enabledExtensionCount = extCount,
      .ppEnabledExtensionNames = exts
    };

#ifdef _DEBUG 
    const char** dbgExtsWExts = calloc(extCount + 1, sizeof(char*));
    for(int i = 0; i < extCount; i++) {
      dbgExtsWExts[i] = exts[i];
    }

    dbgExtsWExts[extCount] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;

    info.enabledLayerCount = 1;
    info.ppEnabledLayerNames = dbgLyrs;
    info.enabledExtensionCount = extCount + 1;
    info.ppEnabledExtensionNames = dbgExtsWExts;
#endif

    CHECKRESULT(vkCreateInstance(&info, NULL, &state->instance))
  }
  // Debug messenger
  {
#ifdef _DEBUG
    VkDebugUtilsMessengerCreateInfoEXT info = {
      .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
      .messageType =  VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | 
                      VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | 
                      VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
      .messageSeverity =  VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
                          VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                          VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT,
      .pfnUserCallback = dbgCallbck,
      .pUserData = NULL
    };

    PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(state->instance, "vkCreateDebugUtilsMessengerEXT");

    if(vkCreateDebugUtilsMessengerEXT) {
      CHECKRESULT(vkCreateDebugUtilsMessengerEXT(state->instance, &info, NULL, &state->dbgMssngr))
    }

#endif
  }
}

void mfVkBckndDeinit(MFVkBckndState* state) {
#ifdef _DEBUG
  PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(state->instance, "vkDestroyDebugUtilsMessengerEXT");
  
  if(vkDestroyDebugUtilsMessengerEXT) {
    vkDestroyDebugUtilsMessengerEXT(state->instance, state->dbgMssngr, NULL);
  }
#endif
  vkDestroyInstance(state->instance, NULL);
}
