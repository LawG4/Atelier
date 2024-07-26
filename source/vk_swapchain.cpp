#include "atelier/atelier_vk_completed.h"
#include "atelier/atelier_vk_mutable.h"
using namespace Atelier;

result VkCompletedSwapchain::CreateInfo::create_default_from_win32(VkCompletedDevice& device,
                                                                   VkCompletedWin32Surface& surf)
{
    uint32_t count = 0;
    VkPhysicalDevice physical = device.m_physical->m_handle;
    memset(&m_info, 0, sizeof(VkSwapchainCreateInfoKHR));
    m_parent_device = &device;
    m_parent_surface = &surf;
    m_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    m_info.clipped = VK_TRUE;
    m_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    m_info.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    m_info.surface = surf.m_handle;

    // Apparently this can be higher than 1, but I can't see how you'd use that?
    m_info.imageArrayLayers = 1;

    // Get the supported present modes
    if (vkGetPhysicalDeviceSurfacePresentModesKHR(physical, surf.m_handle, &count, nullptr) != VK_SUCCESS) {
        Log::error("failed to get device surface present modes");
        return -1;
    }
    m_supported_present_modes.resize(count);
    if (vkGetPhysicalDeviceSurfacePresentModesKHR(physical, surf.m_handle, &count,
                                                  m_supported_present_modes.data()) != VK_SUCCESS) {
        Log::error("failed to get device surface present modes after counting");
        return -2;
    }

    // FIFO is guaranteed by the spec to be supported, what if the device is not to spec? Just check
    m_info.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    auto fifo = std::find(m_supported_present_modes.begin(), m_supported_present_modes.end(), m_info.presentMode);
    if (fifo == m_supported_present_modes.end()) m_info.presentMode = m_supported_present_modes[0];

    // Next get the supported formats
    if (vkGetPhysicalDeviceSurfaceFormatsKHR(physical, surf.m_handle, &count, nullptr) != VK_SUCCESS) {
        Log::error("Failed to get device surface formats");
        return -3;
    }
    m_supported_formats.resize(count);
    if (vkGetPhysicalDeviceSurfaceFormatsKHR(physical, surf.m_handle, &count, m_supported_formats.data()) !=
        VK_SUCCESS) {
        Log::error("Failed to get device surface formats after counting");
        return -4;
    }

    // Just pop the first format into the selected field
    m_info.imageFormat = m_supported_formats[0].format;
    m_info.imageColorSpace = m_supported_formats[0].colorSpace;

    // Begin queues stuff
    //

    // Now we're going to go through all the devices created queues and check if it has support
    for (const auto& queue : device.m_queues) {
        VkBool32 support = VK_TRUE;
        if (vkGetPhysicalDeviceSurfaceSupportKHR(physical, queue.first, surf.m_handle, &support) != VK_SUCCESS) {
            Log::error("Failed to get device surface support");
            return -5;
        }
        if (support == VK_TRUE) m_supported_queue_indicies.push_back(queue.first);
    }

    // Check for the support for at least one supported queue
    if (m_supported_queue_indicies.size() == 0) {
        Log::error("No queues are supporting the surface");
        return -6;
    }

    // We select the first queue with graphics support. If we never encounter one with graphics, then take the
    // first supported queue. We do this to avoid annoying queue ownership transfers when finishing up graphics
    for (uint32_t queue_index : m_supported_queue_indicies) {
        if (device.m_queues[queue_index].props.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            m_selected_queue_indicies.push_back(queue_index);
            break;
        }
    }
    if (m_selected_queue_indicies.size() == 0) m_selected_queue_indicies.push_back(m_supported_queue_indicies[0]);
    m_info.pQueueFamilyIndices = m_selected_queue_indicies.data();
    m_info.queueFamilyIndexCount = m_selected_queue_indicies.size();
    //
    // End queues stuff

    // Get the Surface capabilities
    if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical, surf.m_handle, &m_surface_caps) != VK_SUCCESS) {
        Log::error("Failed to get surface capabilities");
        return -7;
    }

    // Frames in flight should aim for 3, but maybe we need to bound between the supported range
    m_info.minImageCount = 3;
    if (m_info.minImageCount > m_surface_caps.maxImageCount) m_info.minImageCount = m_surface_caps.maxImageCount;
    if (m_info.minImageCount < m_surface_caps.minImageCount) m_info.minImageCount = m_surface_caps.minImageCount;

    // TODO: We should probably look into handling when the current extent isn't defined
    if (m_surface_caps.currentExtent.width != std::numeric_limits<uint32_t>::max() &&
        m_surface_caps.currentExtent.height != std::numeric_limits<uint32_t>::max()) {
        m_info.imageExtent = m_surface_caps.currentExtent;
    } else {
        Log::warn("Couldn't determine swapchain extent from capabilities");
    }

    // Image flags, in general we only need the device local bit
    m_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    m_info.imageUsage &= m_surface_caps.supportedUsageFlags;

    return k_success;
}

result VkCompletedSwapchain::init_from_create_info(VkCompletedSwapchain::CreateInfo& info)
{
    m_info = info;  // Take a copy of info for the
    if (vkCreateSwapchainKHR(m_info.m_parent_device->m_handle, &m_info.m_info, nullptr, &m_handle) != VK_SUCCESS) {
        Log::error("Failed to create the swapchain");
        return -1;
    }

    // Retrieve the image views
    if (vkGetSwapchainImagesKHR(m_info.m_parent_device->m_handle, m_handle, &m_length, nullptr) != VK_SUCCESS) {
        Log::error("Failed to get swapchain length");
        return -2;
    }
    m_image_handles.resize(m_length, VK_NULL_HANDLE);
    if (vkGetSwapchainImagesKHR(m_info.m_parent_device->m_handle, m_handle, &m_length, m_image_handles.data()) !=
        VK_SUCCESS) {
        Log::error("Failed to get swapchain images");
        return -3;
    }

    // Get the image views
    VkImageViewCreateInfo view = {};
    view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    view.subresourceRange.baseMipLevel = 0;
    view.subresourceRange.levelCount = 1;
    view.subresourceRange.baseArrayLayer = 0;
    view.subresourceRange.layerCount = 1;
    view.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    view.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    view.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    view.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    view.format = m_info.m_info.imageFormat;

    m_view_handles.resize(m_length, VK_NULL_HANDLE);
    for (size_t i = 0; i < m_length; i++) {
        view.image = m_image_handles[i];
        if (vkCreateImageView(m_info.m_parent_device->m_handle, &view, nullptr, &m_view_handles[i]) !=
            VK_SUCCESS) {
            Log::error("Failed to create image view for swapchain");
            return -4;
        }
    }

    return k_success;
}

void VkCompletedSwapchain::shutdown(VkCompletedState& vk)
{
    if (m_info.m_parent_device == nullptr) return;
    if (m_info.m_parent_device->m_handle == nullptr) return;
    auto dev = m_info.m_parent_device->m_handle;

    for (auto& view : m_view_handles) {
        vkDestroyImageView(dev, view, nullptr);
        view = VK_NULL_HANDLE;
    }

    vkDestroySwapchainKHR(dev, m_handle, nullptr);
    m_handle = VK_NULL_HANDLE;
    m_info = VkCompletedSwapchain::CreateInfo();
}

int32_t VkCompletedSwapchain::select_preferred_gfx_family(QueueCriteria criteria)
{
    if (m_info.m_parent_device == nullptr) return -1;
    if (criteria != QueueCriteria::k_none && criteria != QueueCriteria::k_gfx_present_overlap &&
        criteria != QueueCriteria::k_gfx_present_no_overlap) {
        // Can't use this selection criteria
        Log::error("Invalid selection criteria passed");
        return -2;
    }

    const auto& present_qs = m_info.m_selected_queue_indicies;
    const auto* dev = m_info.m_parent_device;
    for (const auto& q : dev->m_queues) {
        // We are only interested in graphics queues
        if ((q.second.props.queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0) continue;
        if (criteria == QueueCriteria::k_none) return q.first;  // No criteria take first gfx queue

        // Was this swapchain built with the ability to accept work from this queue? In other words is this queue
        // capable of presenting to this swapchain
        bool supports_present = std::find(present_qs.begin(), present_qs.end(), q.first) != present_qs.end();
        if (supports_present && (criteria == QueueCriteria::k_gfx_present_overlap)) return q.first;
        if (!supports_present && (criteria == QueueCriteria::k_gfx_present_no_overlap)) return q.first;
    }

    return -3;  // Made it here without finding one, so exit
}
