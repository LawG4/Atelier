#include "atelier/atelier_vk.h"
using namespace Atelier;

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

    // Fetch the device queues
    std::vector<VkQueue> queues;
    if (info.fetch_queues(queues, device) != k_success) {
        Log::error("Failed to extract the queues");
        return -2;
    }

    // Pop these into the completed queue. Remember we can have multiple queues per queue info
    m_queues.reserve(queues.size());
    uint32_t queue_indx = 0;
    for (size_t qi = 0; qi < info.queue_infos.size(); qi++) {
        for (size_t i = 0; i < info.queue_infos[qi].queueCount; i++) {
            auto& out_queue = m_queues.emplace_back();
            out_queue.handle = queues[queue_indx++];
            out_queue.family_indx = info.queue_infos[qi].queueFamilyIndex;
            out_queue.props = info.queue_props[out_queue.family_indx];
            out_queue.present_support = false;
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

void Atelier::VkCompletedPhysicalDevice::shutdown(VkInstance instance_handle)
{
    for (auto& logical : m_devices) {
        logical.shutdown(instance_handle);
    }
}

void Atelier::VkCompletedDevice::shutdown(VkInstance instance_handle)
{
    // Wait for the device to finalized
    if (m_handle == VK_NULL_HANDLE) return;
    vkDeviceWaitIdle(m_handle);
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

result VkMutableDeviceCreateInfo::fetch_queues(std::vector<VkQueue>& queues, VkDevice device) const
{
    // Count how many queues we have
    uint32_t queue_count = 0;
    for (const auto& queue_inf : queue_infos) {
        queue_count += queue_inf.queueCount;
    }

    // Fetch all of the queues, we could have multiple of them per family index
    uint32_t queue_indx = 0;
    queues.resize(queue_count, VK_NULL_HANDLE);
    for (const auto& queue_inf : queue_infos) {
        for (uint32_t i = 0; i < queue_inf.queueCount; i++) {
            vkGetDeviceQueue(device, queue_inf.queueFamilyIndex, i, &queues[queue_indx++]);
        }
    }

    return k_success;
}
