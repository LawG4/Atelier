
#include <string>
#include "atelier/atelier.h"

Atelier::result Atelier::VkCompletedInstance::pre_surface_default_init()
{
    // First we need to try and initialize this current default instance
    Atelier::VkMutableInstanceCreateInfo instance_create;
    if (VkMutableInstanceCreateInfo::create_default(instance_create) != k_success) {
        Log::error("Failed to generate default instance create info");
        return -1;
    }
    if (init_from_mutable_instance(instance_create) != k_success) {
        Log::error("Failed to create a instance from default create info");
        return -2;
    }

    // Fetch all of the physical devices available to us
    uint32_t physical_device_count = 0;
    if (vkEnumeratePhysicalDevices(m_handle, &physical_device_count, nullptr) != VK_SUCCESS) {
        Log::error("Failed to enumerate physical devices");
        return -3;
    }
    if (physical_device_count == 0) {
        Log::error("Failed to find any physical devices. You might not have Vulkan");
        return -4;
    }
    std::vector<VkPhysicalDevice> devs(physical_device_count, VK_NULL_HANDLE);
    if (vkEnumeratePhysicalDevices(m_handle, &physical_device_count, devs.data()) != VK_SUCCESS) {
        Log::error("Failed to retrieve physical devices");
        return -5;
    }

    // Now for all the physical devices, we found, create a default logical device for it
    m_physical_devices.reserve(physical_device_count);
    for (VkPhysicalDevice physical : devs) {
        auto dev_info = VkMutableDeviceCreateInfo();
        if (VkMutableDeviceCreateInfo::create_default(dev_info, m_handle, physical) != k_success) {
            Log::error("Failed to create default device info");
            return -6;
        }

        auto& out_dev = m_physical_devices.emplace_back();
        if (out_dev.init_from_mutable_device(dev_info) != k_success) {
            Log::error("Failed to initialize device from info");
            return -7;
        }

        auto& out_logical = out_dev.m_devices.emplace_back();
        if (out_logical.init_from_mutable_device(dev_info) != k_success) {
            Log::error("Failed to get the logical device created");
            return -8;
        }
    }

    return k_success;
}
