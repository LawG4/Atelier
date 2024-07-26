/**
 * @brief Start up the windowing loop and messaging loop of the application
 */
#include <Windows.h>
#include "atelier/atelier.h"

int wWinMain(_In_ HINSTANCE instance_handle, _In_opt_ HINSTANCE pre_instance, _In_ PWSTR p_cmd_line,
             _In_ int n_cmd_show)
{
    // Logger initialization
    Atelier::Log::init();

    // Register the window classes so we can create instances of the different window types
    if (Atelier::Window::register_window_classes(instance_handle) != Atelier::k_success) {
        Atelier::Log::error("Failed to register window class");
        return -1;
    }

    // Fetch as much vulkan information as we can pre window being shown
    auto complete_vk = Atelier::VkCompletedState();
    if (complete_vk.pre_surface_default_init() != Atelier::k_success) {
        Atelier::Log::error("Failed to do vulkan pre surface startup");
        return -1;
    }

    // A place to keep all of the information in the main thread
    auto main_window = Atelier::Window();
    if (Atelier::Window::create_main_window(&main_window, instance_handle) != Atelier::k_success) {
        Atelier::Log::error("Failed when constructing main window");
        return -1;
    }

    // Show the window to the screen, while the animation is playing we can append the additional vulkan stuff
    ShowWindow(main_window.window_handle, n_cmd_show);
    auto& surface = complete_vk.m_surfaces.emplace_back();
    surface.init_from_win32_handles(complete_vk.m_instances[0], instance_handle, main_window.window_handle);

    // For now just select the first device we find
    auto& selected_device = complete_vk.m_devices[0];

    // Create a swapchain targeting the device and surface
    auto& swap = selected_device.m_swaps.emplace_back();
    auto swap_info = Atelier::VkCompletedSwapchain::CreateInfo();
    swap_info.create_default_from_win32(selected_device, surface);
    swap.init_from_create_info(swap_info);

    // Select the queue family for graphics, we start with the first queue which supports graphics. but we prefer
    // any queues which also support presenting to the selected surface. This way we don't have to manage queue
    // ownership transfers between the two
    int32_t gfx_queue_index = swap.select_preferred_gfx_family(Atelier::QueueCriteria::k_gfx_present_overlap);
    bool gfx_queue_supports_present = true;
    if (gfx_queue_index < 0) {
        gfx_queue_index = swap.select_preferred_gfx_family(Atelier::QueueCriteria::k_none);
        gfx_queue_supports_present = false;
    }
    if (gfx_queue_index < 0) {
        Atelier::Log::error("Failed to select a graphics somehow");
        return -1;
    }

    VkQueue present_queue = selected_device.m_queues[swap.m_info.m_selected_queue_indicies[0]].m_handle[0];
    VkQueue gfx_queue = present_queue;  // TODO double check of course

    VkCommandPool pool = VK_NULL_HANDLE;
    VkCommandPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_info.queueFamilyIndex = swap.m_info.m_selected_queue_indicies[0];
    vkCreateCommandPool(selected_device.m_handle, &pool_info, nullptr, &pool);

    VkCommandBuffer buffer = VK_NULL_HANDLE;
    VkCommandBufferAllocateInfo buffer_info = {};
    buffer_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    buffer_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    buffer_info.commandPool = pool;
    buffer_info.commandBufferCount = 1;
    vkAllocateCommandBuffers(selected_device.m_handle, &buffer_info, &buffer);

    VkFence buffer_sync_fence = VK_NULL_HANDLE;
    VkFenceCreateInfo fence = {};
    fence.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    vkCreateFence(selected_device.m_handle, &fence, nullptr, &buffer_sync_fence);

    VkSemaphore image_freed_from_swap = VK_NULL_HANDLE, image_finished_rendering = VK_NULL_HANDLE;
    VkSemaphoreCreateInfo semaphore_info = {};
    semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    vkCreateSemaphore(selected_device.m_handle, &semaphore_info, nullptr, &image_freed_from_swap);
    vkCreateSemaphore(selected_device.m_handle, &semaphore_info, nullptr, &image_finished_rendering);

    VkRenderPass render_pass_handle = VK_NULL_HANDLE;
    VkRenderPassCreateInfo pass_info = {};
    pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    VkAttachmentDescription attachment = {};
    attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    attachment.format = swap.m_info.m_info.imageFormat;
    attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    pass_info.pAttachments = &attachment;
    pass_info.attachmentCount = 1;

    VkSubpassDescription desc = {};
    VkAttachmentReference ref = {};
    ref.attachment = 0;
    ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    desc.pColorAttachments = &ref;
    desc.colorAttachmentCount = 1;
    pass_info.pSubpasses = &desc;
    pass_info.subpassCount = 1;
    vkCreateRenderPass(selected_device.m_handle, &pass_info, nullptr, &render_pass_handle);

    std::vector<VkFramebuffer> framebuffers(swap.m_length, VK_NULL_HANDLE);
    VkFramebufferCreateInfo fb = {};
    fb.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    fb.width = swap.m_info.m_info.imageExtent.width;
    fb.height = swap.m_info.m_info.imageExtent.height;
    fb.renderPass = render_pass_handle;
    fb.layers = 1;
    fb.attachmentCount = 1;
    for (uint32_t i = 0; i < swap.m_length; i++) {
        fb.pAttachments = &swap.m_view_handles[i];
        vkCreateFramebuffer(selected_device.m_handle, &fb, nullptr, &framebuffers[i]);
    }

    // Next enter into the windowing loop. In order to stop us from blocking the main thread, I like to do the peak
    // message instead. We don't listen to a specific window handle so that we can get all the messages in one go
    while (main_window.should_continue) {
        // Look into that message which might have happened or not
        MSG out_msg;
        BOOL peak_res = PeekMessageW(&out_msg, nullptr, 0, 0, PM_REMOVE);
        if (peak_res == 0) continue;  // No messages available

        // Have we received the demand to quit? Either from the OS or the main window
        if (out_msg.message == WM_QUIT) break;
        TranslateMessage(&out_msg);
        DispatchMessageW(&out_msg);

        // Wait for the fence to be signaled, this means the command buffer is free
        vkWaitForFences(selected_device.m_handle, 1, &buffer_sync_fence, VK_TRUE, (uint64_t)-1);
        vkResetFences(selected_device.m_handle, 1, &buffer_sync_fence);

        // Command buffer is free, so reset the pool
        // I believe it is still best practice to do this according to standards
        vkResetCommandPool(selected_device.m_handle, pool, 0);
        VkCommandBufferBeginInfo begin = {};
        begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        vkBeginCommandBuffer(buffer, &begin);

        // Fetch the swapchain
        uint32_t swap_index = 0;
        vkAcquireNextImageKHR(selected_device.m_handle, swap.m_handle, (uint64_t)-1, image_freed_from_swap,
                              VK_NULL_HANDLE, &swap_index);

        VkRenderPassBeginInfo render_pass = {};
        render_pass.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        VkClearValue clear_col = {1.0, 0.0, 0.0, 1.0};
        render_pass.pClearValues = &clear_col;
        render_pass.clearValueCount = 1;
        render_pass.renderArea.offset = {0, 0};
        render_pass.renderArea.extent = swap.m_info.m_info.imageExtent;
        render_pass.renderPass = render_pass_handle;
        render_pass.framebuffer = framebuffers[swap_index];
        vkCmdBeginRenderPass(buffer, &render_pass, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdEndRenderPass(buffer);

        // Done recording buffer
        vkEndCommandBuffer(buffer);

        // Submit Graphics Work.
        VkSubmitInfo gfx_submit = {};
        gfx_submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        gfx_submit.pCommandBuffers = &buffer;
        gfx_submit.commandBufferCount = 1;
        gfx_submit.pWaitSemaphores = &image_freed_from_swap;
        gfx_submit.waitSemaphoreCount = 1;
        VkPipelineStageFlags signal_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        gfx_submit.pSignalSemaphores = &image_finished_rendering;
        gfx_submit.signalSemaphoreCount = 1;
        gfx_submit.pWaitDstStageMask = &signal_stage;
        vkQueueSubmit(gfx_queue, 1, &gfx_submit, buffer_sync_fence);

        // Present to the screen
        VkPresentInfoKHR present = {};
        present.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        present.pSwapchains = &swap.m_handle;
        present.swapchainCount = 1;
        present.pWaitSemaphores = &image_finished_rendering;
        present.waitSemaphoreCount = 1;
        present.pImageIndices = &swap_index;
        vkQueuePresentKHR(present_queue, &present);
    }

    // Shut down everything
    complete_vk.shutdown();
    Atelier::Log::shutdown();

    return 0;
}
