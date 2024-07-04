#include "atelier/atelier.h"
using namespace Atelier;

result VkCompletedInstance::init_from_mutable_instance(const VkMutableInstanceCreateInfo& info)
{
    // We leave the true instance untouched until success on all the vulkan calls
    VkInstance instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT messenger = VK_NULL_HANDLE;
    if (info.create_instance(instance) != k_success) {
        Log::error("Failed to create instance");
        return -1;
    }
    if (info.create_messenger(messenger, instance) != k_success) {
        Log::error("Failed while creating debug messenger");
        return -2;
    }

    // Yay we can store the vulkan devices
    m_handle = instance;
    if (messenger != VK_NULL_HANDLE) m_messengers.push_back(messenger);

    // Copy any enabled layers and extensions
    m_enabled_extensions.reserve(info.ext_selected.size());
    for (const auto s : info.ext_selected) {
        m_enabled_extensions.push_back(std::string(s));
    }
    m_enabled_layers.reserve(info.layer_selected.size());
    for (const auto s : info.layer_selected) {
        m_enabled_layers.push_back(std::string(s));
    }

    return k_success;
}

void Atelier::VkCompletedInstance::shutdown()
{
    // Shutdown all of the devices
    for (auto& dev : m_physical_devices) {
        dev.shutdown(m_handle);
    }

    // If we have a debug messenger, we need to destroy them
    if (m_messengers.size() > 0) {
        auto destroy_msg =
          (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_handle, "vkDestroyDebugUtilsMessengerEXT");
        if (destroy_msg != nullptr) {
            for (auto& msg : m_messengers) {
                destroy_msg(m_handle, msg, nullptr);
            }
        }
        m_messengers.clear();
    }

    // Free the handle
    if (m_handle != VK_NULL_HANDLE) {
        vkDestroyInstance(m_handle, nullptr);
        m_handle = VK_NULL_HANDLE;
    }
}

result VkMutableInstanceCreateInfo::create_instance(VkInstance& instance, VkAllocationCallbacks* alloc) const
{
    VkApplicationInfo app = {VK_STRUCTURE_TYPE_APPLICATION_INFO};
    app.apiVersion = api_version;
    app.pApplicationName = application_name.empty() ? nullptr : application_name.c_str();
    app.pEngineName = engine_name.empty() ? nullptr : engine_name.c_str();
    app.applicationVersion = application_version;
    app.engineVersion = engine_version;

    VkInstanceCreateInfo info = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    info.pApplicationInfo = &app;
    info.ppEnabledExtensionNames = ext_selected.data();
    info.enabledExtensionCount = ext_selected.size();
    info.ppEnabledLayerNames = layer_selected.data();
    info.enabledLayerCount = layer_selected.size();

    // Just create a debug create info on the stack, we'll only attach it to the pnext chain if validation is
    // enabled
    VkDebugUtilsMessengerCreateInfoEXT pnext = {VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
    pnext.pfnUserCallback = debug_callback;
    pnext.messageSeverity =
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    pnext.messageType =
      VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    if (validation_layer_enabled && validation_utils_enabled) {
        info.pNext = &pnext;
    }

    return vkCreateInstance(&info, alloc, &instance) == VK_SUCCESS ? k_success : -1;
}

static constexpr char s_validation_layer_name[] = "VK_LAYER_KHRONOS_validation";
static constexpr char s_debug_utils_ext_name[] = "VK_EXT_debug_utils";
static constexpr const char* s_surface_ext[] = {"VK_KHR_surface", "VK_KHR_win32_surface"};
static constexpr uint32_t s_surface_count = sizeof(s_surface_ext) / sizeof(char*);

// Default callback for handling debug messages
static VkBool32 s_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                           VkDebugUtilsMessageTypeFlagsEXT messageTypes,
                           const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
    switch (messageSeverity) {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            Log::warn("Vulkan message : \n%s", pCallbackData->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
            Log::error("Vulkan message : \n%s", pCallbackData->pMessage);
            break;
        default:
            break;
    }
    return VK_FALSE;
}

result VkMutableInstanceCreateInfo::create_default(VkMutableInstanceCreateInfo& inst)
{
    inst.engine_name = "Atelier";
    uint32_t count = 0;

    // Get extensions supported
    if (vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr) != VK_SUCCESS) return -1;
    inst.ext_props.resize(count);
    if (vkEnumerateInstanceExtensionProperties(nullptr, &count, inst.ext_props.data()) != VK_SUCCESS) return -2;

    // Get layers supported
    if (vkEnumerateInstanceLayerProperties(&count, nullptr) != VK_SUCCESS) return -3;
    inst.layer_props.resize(count);
    if (vkEnumerateInstanceLayerProperties(&count, inst.layer_props.data()) != VK_SUCCESS) return -4;

    // Screw it we'll just enable all of the known surface extensions, theres no downside in enabling extra ones.
    // Plus the instance is the one which is in charge of determining which surfaces are exposed
    for (size_t i = 0; i < s_surface_count; i++) {
        for (const auto& ext : inst.ext_props) {
            if (strcmp(s_surface_ext[i], ext.extensionName) == 0) {
                inst.ext_selected.push_back(s_surface_ext[i]);
                break;
            }
        }
    }

    // Now lets see if we can add debug validation layers
#ifndef NDEBUG
    for (const auto& layer : inst.layer_props) {
        if (strcmp(s_validation_layer_name, layer.layerName) == 0) {
            inst.validation_layer_enabled = true;
            inst.layer_selected.push_back(s_validation_layer_name);
            break;
        }
    }

    // Only enable validation debug utils messenger when the layer is present
    if (!inst.validation_layer_enabled) return k_success;
    uint32_t found_ext_names = 0;
    for (const auto& layer : inst.ext_props) {
        if (strcmp(s_debug_utils_ext_name, layer.extensionName) == 0) {
            inst.validation_utils_enabled = true;
            inst.debug_callback = s_callback;
            inst.ext_selected.push_back(s_debug_utils_ext_name);
            break;
        }
    }

#endif

    // All good
    return k_success;
}

result VkMutableInstanceCreateInfo::create_messenger(VkDebugUtilsMessengerEXT& msg, VkInstance instance,
                                                     VkAllocationCallbacks* alloc) const
{
    msg = VK_NULL_HANDLE;  // reset handle to null
#ifndef NDEBUG
    if (!validation_layer_enabled || !validation_utils_enabled) return k_success;
    if (debug_callback == nullptr) return -1;

    // Get the create function pointer
    auto create_callback =
      (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    // if (create_callback == nullptr) return -2;

    // Construct callback
    VkDebugUtilsMessengerCreateInfoEXT info = {VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
    info.pfnUserCallback = debug_callback;
    info.messageSeverity =
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    info.messageType =
      VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

    // Call the creation
    if (create_callback(instance, &info, alloc, &msg) != VK_SUCCESS) return -3;
#endif
    return k_success;
}
