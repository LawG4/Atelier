
#include <string>
#include "atelier/atelier.h"
#include "vulkan/vulkan_win32.h"
using namespace Atelier;

result VkCompletedWin32Surface::init_from_win32_handles(VkCompletedInstance& inst, void* win32_instance_handle,
                                                        void* win32_window_handle)
{
    VkWin32SurfaceCreateInfoKHR surface_info = {};
    surface_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    surface_info.hinstance = (HINSTANCE)win32_instance_handle;
    surface_info.hwnd = (HWND)win32_window_handle;

    if (vkCreateWin32SurfaceKHR(inst.m_handle, &surface_info, nullptr, &m_handle) != VK_SUCCESS) {
        Log::error("Failed to create a win32 surface khr handle");
        return -1;
    }

    // Attach parents
    m_parent = &inst;
    m_win32_window_handle = win32_window_handle;
    m_type = VkCompletedSurface::Type::k_win32;

    return k_success;
}

void VkCompletedWin32Surface::shutdown(VkCompletedState& vk)
{
    // Shutdown any derived surfaces
    for (auto& dev : vk.m_devices) {
        for (auto& swap : dev.m_swaps)
            if (swap.m_info.m_parent_surface == this) swap.shutdown(vk);
    }

    // TODO inform the parent win32 object that the surface is being detached
    vkDestroySurfaceKHR(m_parent->m_handle, m_handle, nullptr);
    m_handle = VK_NULL_HANDLE;
}
