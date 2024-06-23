
#include <string>
#include "atelier/atelier.h"
static const char* known_good_surface_ext_names[] = {"VK_KHR_surface", "VK_KHR_win32_surface"};
static const size_t known_good_count = sizeof(known_good_surface_ext_names) / sizeof(char*);

static void worker(Atelier::VkPreSurfaceInfo* info)
{
    // Check we have an info and if we do lock it in place until we leave the scope
    if (info == nullptr) return;
    std::lock_guard<std::mutex> lg(info->mut);
    uint32_t count;

    // Instance version : TODO what if built against api version 1.0
    vkEnumerateInstanceVersion(&info->max_api_ver);

    // Get the instance extensions supported
    if (vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr) != VK_SUCCESS) return;
    info->extensions.resize(count);
    if (vkEnumerateInstanceExtensionProperties(nullptr, &count, info->extensions.data()) != VK_SUCCESS) return;

    // Get the instance layers supported
    if (vkEnumerateInstanceLayerProperties(&count, nullptr) != VK_SUCCESS) return;
    info->layers.resize(count);
    if (vkEnumerateInstanceLayerProperties(&count, info->layers.data()) != VK_SUCCESS) return;

    // Create the most basic vulkan instance
    VkApplicationInfo app_info = {};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.apiVersion = VK_MAKE_API_VERSION(1, 0, 0, 0);

    VkInstanceCreateInfo inst_info = {};
    inst_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    inst_info.pApplicationInfo = &app_info;

    // Since this instance is being to look at surface capabilities, we're going to enable ALL available instance
    // extensions which were found
    std::vector<const char*> surface_extensions;
    for (size_t i = 0; i < known_good_count; i++) {
        for (const auto& ext_prop : info->extensions) {
            // Check if one of these extensions match
            if (strcmp(ext_prop.extensionName, known_good_surface_ext_names[i]) == 0) {
                surface_extensions.push_back(known_good_surface_ext_names[i]);
                break;
            }
        }
    }
    inst_info.ppEnabledExtensionNames = surface_extensions.data();
    inst_info.enabledExtensionCount = surface_extensions.size();

    // Actually create the instance
    if (vkCreateInstance(&inst_info, nullptr, &info->instance) != VK_SUCCESS) return;

    // Cool, now we need to get the physical devices
    vkEnumeratePhysicalDevices(info->instance, &count, nullptr);
    std::vector<VkPhysicalDevice> devices(count, VK_NULL_HANDLE);
    vkEnumeratePhysicalDevices(info->instance, &count, devices.data());
    info->devices.reserve(count);

    // Get the properties of each physical device
    for (const auto dev : devices) {  // no reference needed as opaque handle
        // New device on the per device list
        auto& out = info->devices.emplace_back();
        out.device = dev;

        // Get the properties
        vkGetPhysicalDeviceProperties(out.device, &out.props);

        // Get the extensions
        vkEnumerateDeviceExtensionProperties(out.device, nullptr, &count, nullptr);
        out.extensions.resize(count);
        vkEnumerateDeviceExtensionProperties(out.device, nullptr, &count, out.extensions.data());
    }

    // Cool done.
}

std::thread Atelier::VkPreSurfaceInfo::kick_worker(Atelier::VkPreSurfaceInfo& info)
{
    std::thread t(worker, &info);
    return t;
}
