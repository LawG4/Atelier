/**
 * @brief We split Vulkan objects. We make a mutable version of the create infos for user editing of the values.
 * Then we have the completed object which we use to place into global state tracking.
 *
 */
#pragma once
#include "atelier_base.h"
#include "vulkan/vulkan_core.h"

#include <string>
#include <vector>
namespace Atelier
{

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
};

}  // namespace Atelier
