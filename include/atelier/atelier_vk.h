/**
 * @brief We split Vulkan objects. We make a mutable version of the create infos for user editing of the values.
 * Then we have the completed object which we use to place into global state tracking
 */
#pragma once
#include "atelier_base.h"
#include "vulkan/vulkan_core.h"

#include <string>
#include <vector>
namespace Atelier
{

//
// Creation helpers
//

/**
 * @brief The VkInstanceCreateInfo structure has it's array members being accessed via a const qualifier, which
 * means we can't edit or append to those fields via a default approach. I decided to use STL containers to avoid
 * forcing a user to also pass an additional allocator around. I need to fix this in the C implementation
 */
struct VkMutableInstanceCreateInfo {
    VkMutableInstanceCreateInfo() = default;
    uint32_t api_version = VK_MAKE_API_VERSION(0, 1, 0, 0);
    std::string application_name;
    uint32_t application_version = 0;
    std::string engine_name;
    uint32_t engine_version = 0;
    std::vector<VkExtensionProperties> ext_props;
    std::vector<const char*> ext_selected;
    std::vector<VkLayerProperties> layer_props;
    std::vector<const char*> layer_selected;
    PFN_vkDebugUtilsMessengerCallbackEXT debug_callback = nullptr;
    bool validation_layer_enabled = false;
    bool validation_utils_enabled = false;

    // Creates a default create info with default video extensions, and debug utils when available
    static result create_default(VkMutableInstanceCreateInfo& inst);

    // Uses the given create info to try to create a vulkan instance handle
    result create_instance(VkInstance& instance, VkAllocationCallbacks* alloc = nullptr) const;

    // Attempts to create a debug utils messenger object. On release mode, or when not enabled it will just give
    // the user a VK_NULL_HANDLE. This only reports an error when a messenger SHOULD be created but fails
    result create_messenger(VkDebugUtilsMessengerEXT& msg, VkInstance instance,
                            VkAllocationCallbacks* alloc = nullptr) const;
};

/**
 * @brief Similar to the VkMutableInstanceCreateInfo, this struct is used to supply STL replacement for previously
 * immutable const pointers inside VkDeviceCreateInfo
 */
struct VkMutableDeviceCreateInfo {
    VkMutableDeviceCreateInfo() = default;
    VkPhysicalDevice physical_device = VK_NULL_HANDLE;
    VkPhysicalDeviceProperties device_properties = {};
    std::vector<VkExtensionProperties> ext_props;
    std::vector<const char*> ext_selected;
    std::vector<VkQueueFamilyProperties> queue_props;
    std::vector<VkDeviceQueueCreateInfo> queue_infos;
    std::vector<float> queue_priorities;

    static result create_default(VkMutableDeviceCreateInfo& dev, VkInstance instance, VkPhysicalDevice physical);
    result create_device(VkDevice& Device, const VkAllocationCallbacks* alloc = nullptr) const;
    result fetch_queues(std::vector<VkQueue>& queues, VkDevice device) const;
};

//
// Completed Tiered Handlers
//

struct VkCompletedInstance {
    VkCompletedInstance() = default;
    VkInstance m_handle = VK_NULL_HANDLE;
    std::vector<std::string> m_enabled_extensions;
    std::vector<std::string> m_enabled_layers;
    std::vector<VkDebugUtilsMessengerEXT> m_messengers;
    std::vector<struct VkCompletedPhysicalDevice> m_physical_devices;

    // Shuts down all of the child vulkan objects in order
    void shutdown();

    // Attempts to only initialize the instance put none of the child vulkan devices
    result init_from_mutable_instance(const VkMutableInstanceCreateInfo& info);

    // Attempts to create the default of all handles available in the time before a surface is shown to the user
    result pre_surface_default_init();
};

struct VkCompletedPhysicalDevice {
    VkCompletedPhysicalDevice() = default;
    VkPhysicalDevice m_handle = VK_NULL_HANDLE;
    VkPhysicalDeviceProperties m_device_properties = {};
    std::vector<struct VkCompletedDevice> m_devices;

    // Shuts down all of the child logical devices and clears properties
    void shutdown(VkInstance instance_handle);

    // Attempts to initialize the physical device, but not the logical device
    result init_from_mutable_device(const VkMutableDeviceCreateInfo& info);
};

struct VkCompletedDevice {
    VkDevice m_handle = VK_NULL_HANDLE;
    std::vector<std::string> m_enabled_extensions;
    std::vector<struct VkCompletedQueue> m_queues;

    // Shuts down all of the child vulkan objects in order
    void shutdown(VkInstance instance_handle);

    // Attempts to initialize the logical device from the provided info
    result init_from_mutable_device(const VkMutableDeviceCreateInfo& info);
};

struct VkCompletedQueue {
    VkQueue handle = VK_NULL_HANDLE;
    uint32_t family_indx = 0;
    VkQueueFamilyProperties props = {};
    bool present_support = false;
};

}  // namespace Atelier
