#include "atelier/atelier_vk_completed.h"
#include "atelier/atelier_vk_mutable.h"
using namespace Atelier;

result VkCompletedPhysicalDevice::init_from_instance(VkInstance instance, VkPhysicalDevice device)
{
    // TODO: There's still some stuff we can do with getting physical device props_2 and using the instance to
    // check if available
    (void)instance;
    m_handle = device;
    vkGetPhysicalDeviceProperties(device, &m_device_properties);
    return k_success;
}

result VkCompletedPhysicalDevice::init_from_mutable_device(const VkMutableDeviceCreateInfo& info)
{
    m_handle = info.physical_device;
    m_device_properties = info.device_properties;
    return k_success;
}

result VkCompletedDevice::init_from_mutable_device(const VkMutableDeviceCreateInfo& info)
{
    // Try and initialize the vulkan handle for a logical device, but not touching the original
    VkDevice device = VK_NULL_HANDLE;
    if (info.create_device(device) != k_success) {
        Log::error("Failed to create a logical device");
        return -1;
    }

    // We track the queues for each family as a group. The user can in theory create multiple queues targeting the
    // same family
    m_queues.reserve(info.queue_infos.size());
    for (const auto& qi : info.queue_infos) {
        // Copy the info about the specific queue family
        auto& out_queue = m_queues[qi.queueFamilyIndex];
        out_queue.family_indx = qi.queueFamilyIndex;
        out_queue.props = info.queue_props[qi.queueFamilyIndex];

        // Fetch all of the handles that the user specified for creation
        out_queue.m_handle.resize(qi.queueCount, VK_NULL_HANDLE);
        for (size_t i = 0; i < qi.queueCount; i++) {
            vkGetDeviceQueue(device, qi.queueFamilyIndex, i, &out_queue.m_handle[i]);
        }
    }

    // Success attach the device handle and extensions
    m_handle = device;
    m_enabled_extensions.reserve(info.ext_selected.size());
    for (const char* s : info.ext_selected) {
        m_enabled_extensions.push_back(std::string(s));
    }
    return k_success;
}

void Atelier::VkCompletedPhysicalDevice::shutdown(VkCompletedState& vk)
{
    // In most cleanup the logical devices are cleared up separately but just in
    for (auto& logical : vk.m_devices) {
        logical.shutdown(vk);
    }
    m_handle = VK_NULL_HANDLE;
}

void Atelier::VkCompletedDevice::shutdown(VkCompletedState& vk)
{
    // Wait for the device to finalized
    if (m_handle == VK_NULL_HANDLE) return;
    vkDeviceWaitIdle(m_handle);

    // We need to delete all derived device objects
    for (auto& swapchain : vk.m_swapchains) {
        if (swapchain.m_info.m_parent_device == this) {
            swapchain.shutdown(vk);
        }
    }

    vkDestroyDevice(m_handle, nullptr);
    m_handle = VK_NULL_HANDLE;
}

result VkMutableDeviceCreateInfo::create_device(VkDevice& device, const VkAllocationCallbacks* alloc) const
{
    if (this->physical_device == VK_NULL_HANDLE) return -1;
    if (this->queue_infos.size() == 0) return -1;

    VkDeviceCreateInfo info = {VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    info.ppEnabledExtensionNames = ext_selected.data();
    info.enabledExtensionCount = ext_selected.size();
    info.pQueueCreateInfos = queue_infos.data();
    info.queueCreateInfoCount = queue_infos.size();

    if (vkCreateDevice(physical_device, &info, alloc, &device) != VK_SUCCESS) return -1;
    return k_success;
}

result VkMutableDeviceCreateInfo::create_default(VkMutableDeviceCreateInfo& dev, VkInstance instance,
                                                 VkPhysicalDevice physical)
{
    if (instance == VK_NULL_HANDLE || physical == VK_NULL_HANDLE) return -1;

    dev.physical_device = physical;
    vkGetPhysicalDeviceProperties(physical, &dev.device_properties);
    uint32_t count = 0;

    // Get the extensions supported via the physical device
    if (vkEnumerateDeviceExtensionProperties(physical, nullptr, &count, nullptr) != VK_SUCCESS) return -2;
    dev.ext_props.resize(count);
    if (vkEnumerateDeviceExtensionProperties(physical, nullptr, &count, dev.ext_props.data()) != VK_SUCCESS)
        return -3;

    // By default we want to append the VkSwapchain extension for displaying
    for (const auto& exts : dev.ext_props) {
        if (strcmp(exts.extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0) {
            dev.ext_selected.push_back(exts.extensionName);
            break;
        }
    }

    // Get the queue properties
    vkGetPhysicalDeviceQueueFamilyProperties(physical, &count, nullptr);
    dev.queue_props.resize(count);
    vkGetPhysicalDeviceQueueFamilyProperties(physical, &count, dev.queue_props.data());

    // From here, make a queue info for each queue which exists. We assume the user only wants one queue per family
    // index
    dev.queue_priorities.push_back(1.0f);
    dev.queue_infos.reserve(count);
    for (size_t i = 0; i < count; i++) {
        auto& queue = dev.queue_infos.emplace_back();
        queue.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue.pQueuePriorities = &dev.queue_priorities[0];
        queue.queueFamilyIndex = i;
        queue.queueCount = 1;
        queue.pNext = nullptr;
    }

    return k_success;
}
