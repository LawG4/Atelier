/**
 * @brief We split Vulkan objects. We make a mutable version of the create infos for user editing of the values.
 * Then we have the completed object which we use to place into global state tracking
 */
#pragma once
#include "atelier_base.h"
#include "vulkan/vulkan_core.h"

#include <string>
#include <unordered_map>
#include <vector>

namespace Atelier
{

/**
 * @brief Stores all the Vulkan objects in a list together such that they keep track of their parents
 */
struct VkCompletedState {
    std::vector<struct VkCompletedInstance> m_instances;
    std::vector<struct VkCompletedDevice> m_devices;
    std::vector<struct VkCompletedWin32Surface> m_surfaces;

    // Shuts down everything in the completed vulkan state
    result shutdown();

    // Attempts to create the default of all handles available in the time before a surface is shown to the user
    result pre_surface_default_init();
};

struct VkCompletedInstance {
    VkCompletedInstance() = default;
    VkInstance m_handle = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT m_messenger = VK_NULL_HANDLE;
    std::vector<struct VkCompletedPhysicalDevice> m_physical_devices;
    std::vector<std::string> m_enabled_extensions;
    std::vector<std::string> m_enabled_layers;

    // Shuts down this instance and all of the child vulkan objects inside the passed VkCompletedState
    void shutdown(VkCompletedState& vk);

    // Attempts to only initialize the instance put none of the child vulkan devices
    result init_from_mutable_instance(const struct VkMutableInstanceCreateInfo& info);
};

struct VkCompletedPhysicalDevice {
    VkCompletedPhysicalDevice() = default;
    VkPhysicalDevice m_handle = VK_NULL_HANDLE;
    VkCompletedInstance* m_parent = nullptr;
    VkPhysicalDeviceProperties m_device_properties = {};

    // Tries to shut down all of the logical devices and the resets the resources contained as a child
    void shutdown(VkCompletedState& vk);

    // Attempts to initialize the physical device by re-fetching all info
    result init_from_instance(VkInstance instance, VkPhysicalDevice device);

    // Attempts to initialize the physical device, but not the logical device
    result init_from_mutable_device(const struct VkMutableDeviceCreateInfo& info);
};

struct VkCompletedDevice {
    VkDevice m_handle = VK_NULL_HANDLE;
    VkCompletedInstance* m_parent = nullptr;
    VkCompletedPhysicalDevice* m_physical = nullptr;

    std::vector<std::string> m_enabled_extensions;
    std::unordered_map<uint32_t, struct VkCompletedQueue> m_queues;
    std::vector<struct VkCompletedSwapchain> m_swaps;

    // Shuts down all of the child vulkan objects in order
    void shutdown(VkCompletedState& vk);

    // Attempts to initialize the logical device from the provided info
    result init_from_mutable_device(const struct VkMutableDeviceCreateInfo& info);
};

/**
 * @brief Defines the criteria used when selecting a queue family index for some form of work
 */
enum class QueueCriteria : uint32_t {
    k_none,                    // No criteria, just choose the first one
    k_gfx_present_overlap,     // Select graphics queue which must overlap with present
    k_gfx_present_no_overlap,  // Select graphics queue which must NOT overlap with present
    k_present_gfx_no_overlap,  // Select present queue which must NOT overlap with graphics
    k_present_gfx_overlap,     // Select present queue which must overlap with graphics
};

struct VkCompletedQueue {
    VkCompletedQueue() = default;
    std::vector<VkQueue> m_handle;
    uint32_t family_indx = 0;
    VkQueueFamilyProperties props = {};
};

/**
 * @brief Base surface which is only used as an interface to other structs
 */
struct VkCompletedSurface {
    enum class Type : uint32_t { k_unknown, k_win32 };

    VkCompletedSurface() = default;
    VkSurfaceKHR m_handle = VK_NULL_HANDLE;
    VkCompletedInstance* m_parent = nullptr;
    Type m_type = Type::k_unknown;
};

/**
 * @brief Represents the information that a win32 surface can have. The surface must be owned by the OS window, but
 * it also needs a way to inform the window if it's being destroyed, this way the window knows not to send updates
 * to this address anymore
 */
struct VkCompletedWin32Surface : VkCompletedSurface {
    VkCompletedWin32Surface() = default;
    void* m_win32_window_handle = nullptr;

    void shutdown(VkCompletedState& vk);

    result init_from_win32_handles(struct VkCompletedInstance& inst, void* win32_instance_handle,
                                   void* win32_window_handle);
};

struct VkCompletedSwapchain {
    struct CreateInfo {
        CreateInfo() = default;
        VkSwapchainCreateInfoKHR m_info = {};
        VkCompletedDevice* m_parent_device = nullptr;
        VkCompletedSurface* m_parent_surface = nullptr;
        std::vector<VkSurfaceFormatKHR> m_supported_formats;
        std::vector<VkPresentModeKHR> m_supported_present_modes;
        std::vector<uint32_t> m_supported_queue_indicies;
        std::vector<uint32_t> m_selected_queue_indicies;
        VkSurfaceCapabilitiesKHR m_surface_caps = {};  // Technically derived from surface but needs device handle

        result create_default_from_win32(VkCompletedDevice& device, VkCompletedWin32Surface& surf);
    };

    VkCompletedSwapchain() = default;
    VkSwapchainKHR m_handle = VK_NULL_HANDLE;
    struct CreateInfo m_info;
    uint32_t m_length = 0;
    std::vector<VkImage> m_image_handles;
    std::vector<VkImageView> m_view_handles;

    void shutdown(VkCompletedState& vk);

    // Attempt to initialize the swapchain from required information
    result init_from_create_info(CreateInfo& info);

    // The graphics queue you select might depend on the availability of present queues enabled in your swapchain.
    // returns negative if the queue index matching the criteria couldn't be found
    int32_t select_preferred_gfx_family(QueueCriteria criteria);
};

}  // namespace Atelier
