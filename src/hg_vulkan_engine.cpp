#include "hg_vulkan_engine.h"

#include "hg_pch.h"
#include "hg_utils.h"
#include "hg_load.h"

#include <algorithm>
#include <array>
#include <format>
#include <fstream>
#include <iostream>
#include <span>
#include <vector>

namespace hg {

#ifdef NDEBUG
inline constexpr std::array<const char*, 0> ValidationLayers = {};
#else
inline constexpr std::array ValidationLayers = {"VK_LAYER_KHRONOS_validation"};
#endif
constexpr std::array DeviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    VK_EXT_SHADER_OBJECT_EXTENSION_NAME,
    VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
};

static vk::Bool32 debug_callback(
    const vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
    const vk::DebugUtilsMessageTypeFlagsEXT,
    const vk::DebugUtilsMessengerCallbackDataEXT* callback_data, void*
) {
    std::cout << std::format("{}\n", callback_data->pMessage);
    if (severity & vk::DebugUtilsMessageSeverityFlagBitsEXT::eError)
        ERROR("Vulkan error");
    return VK_FALSE;
}

const vk::DebugUtilsMessengerCreateInfoEXT DebugUtilsMessengerCreateInfo = {
    .messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose
                     | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning
                     | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
    .messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral
                 | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation
                 | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
    .pfnUserCallback = debug_callback,
};

[[nodiscard]] static Result<std::vector<const char*>> get_required_instance_extensions() {
    u32 glfw_extension_count = 0;
    const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
    if (glfw_extensions == nullptr)
        return Err::GlfwFailure;

    auto required_extensions = ok<std::vector<const char*>>();
    required_extensions->reserve(glfw_extension_count + 1);
    for (usize i = 0; i < glfw_extension_count; ++i)
        required_extensions->emplace_back(glfw_extensions[i]);
#ifndef NDEBUG
    required_extensions->emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

    return required_extensions;
}

[[nodiscard]] static Result<bool> check_instance_extension_availability(const std::span<const char* const> required_extensions) {
    ASSERT(!required_extensions.empty());

    const auto extensions = vk::enumerateInstanceExtensionProperties();
    if (extensions.result != vk::Result::eSuccess)
        return Err::VulkanFailure;

    return ok(std::ranges::all_of(required_extensions, [&](const char* required) {
        return std::ranges::any_of(extensions.value, [&](const VkExtensionProperties& extension) {
            return strcmp(required, extension.extensionName) == 0;
        });
    }));
}

static Result<vk::Instance> init_instance() {
    const vk::ApplicationInfo app_info = {
        .pApplicationName = "Hurdy Gurdy",
        .applicationVersion = 0,
        .pEngineName = "Hurdy Gurdy",
        .engineVersion = 0,
        .apiVersion = VK_API_VERSION_1_3,
    };

    const auto required_extensions = get_required_instance_extensions();
    if (required_extensions.has_err())
        return required_extensions.err();

    const auto extensions_available = check_instance_extension_availability(*required_extensions);
    if (extensions_available.has_err())
        return extensions_available.err();
    if (!*extensions_available)
        return Err::VulkanExtensionsUnavailable;

    const auto instance = vk::createInstance({
        .pNext = &DebugUtilsMessengerCreateInfo,
        .pApplicationInfo = &app_info,
        .enabledLayerCount = to_u32(ValidationLayers.size()),
        .ppEnabledLayerNames = ValidationLayers.data(),
        .enabledExtensionCount = to_u32(required_extensions->size()),
        .ppEnabledExtensionNames = required_extensions->data(),
    });
    if (instance.result != vk::Result::eSuccess)
        return Err::CouldNotCreateVkInstance;

    ASSERT(instance.value != nullptr);
    return ok(instance.value);
}

static Result<u32> find_queue_family(const vk::PhysicalDevice gpu) {
    ASSERT(gpu != nullptr);

    const auto queue_families = gpu.getQueueFamilyProperties();
    const auto queue_family = std::ranges::find_if(queue_families, [](const vk::QueueFamilyProperties family) {
        return static_cast<bool>(family.queueFlags & (vk::QueueFlagBits::eGraphics | vk::QueueFlagBits::eCompute));
    });
    if (queue_family == queue_families.end())
        return Err::VkQueueFamilyUnavailable;

    return ok(static_cast<u32>(queue_family - queue_families.begin()));
}

static Result<vk::PhysicalDevice> find_gpu(const Engine& engine) {
    ASSERT(engine.instance != nullptr);

    const auto gpus = engine.instance.enumeratePhysicalDevices();
    if (gpus.result != vk::Result::eSuccess)
        return Err::VkPhysicalDevicesUnavailable;

    for (const auto gpu : gpus.value) {
        const auto features = gpu.getFeatures();
        if (features.sampleRateShading != vk::True || features.samplerAnisotropy != vk::True)
            continue;

        const auto extensions = gpu.enumerateDeviceExtensionProperties();
        if (extensions.result != vk::Result::eSuccess)
            return Err::VulkanFailure;

        if (!std::ranges::all_of(DeviceExtensions, [&](const char* required) {
            return std::ranges::any_of(extensions.value, [&](const vk::ExtensionProperties extension) {
                return strcmp(required, extension.extensionName);
            });
        }))
            continue;

        if (find_queue_family(gpu).has_err())
            continue;

        ASSERT(gpu != nullptr);
        return ok(gpu);
    }

    return Err::VkPhysicalDevicesUnsuitable;
}

static Result<vk::Device> init_device(const Engine& engine) {
    ASSERT(engine.gpu != nullptr);
    ASSERT(engine.queue_family_index != UINT32_MAX);

    vk::PhysicalDeviceBufferAddressFeaturesEXT buffer_address_feature = {.pNext = nullptr, .bufferDeviceAddress = vk::True};
    vk::PhysicalDeviceDescriptorIndexingFeatures descriptor_indexing_features = {
        .pNext = &buffer_address_feature,
        // .shaderInputAttachmentArrayDynamicIndexing = true,
        // .shaderUniformTexelBufferArrayDynamicIndexing = true,
        // .shaderStorageTexelBufferArrayDynamicIndexing = true,
        // .shaderUniformBufferArrayNonUniformIndexing = true,
        .shaderSampledImageArrayNonUniformIndexing = true,
        // .shaderStorageBufferArrayNonUniformIndexing = true,
        // .shaderStorageImageArrayNonUniformIndexing = true,
        // .shaderInputAttachmentArrayNonUniformIndexing = true,
        // .shaderUniformTexelBufferArrayNonUniformIndexing = true,
        // .shaderStorageTexelBufferArrayNonUniformIndexing = true,
        // .descriptorBindingUniformBufferUpdateAfterBind = true,
        // .descriptorBindingSampledImageUpdateAfterBind = true,
        // .descriptorBindingStorageImageUpdateAfterBind = true,
        // .descriptorBindingStorageBufferUpdateAfterBind = true,
        // .descriptorBindingUniformTexelBufferUpdateAfterBind = true,
        // .descriptorBindingStorageTexelBufferUpdateAfterBind = true,
        // .descriptorBindingUpdateUnusedWhilePending = true,
        .descriptorBindingPartiallyBound = true,
        // .descriptorBindingVariableDescriptorCount = true,
        .runtimeDescriptorArray = true,
    };
    vk::PhysicalDeviceShaderObjectFeaturesEXT shader_object_feature = {.pNext = &descriptor_indexing_features, .shaderObject = vk::True};
    vk::PhysicalDeviceDynamicRenderingFeatures dynamic_rendering_feature = {.pNext = &shader_object_feature, .dynamicRendering = vk::True};
    vk::PhysicalDeviceSynchronization2Features synchronization2_feature = {.pNext = &dynamic_rendering_feature, .synchronization2 = vk::True};
    constexpr vk::PhysicalDeviceFeatures features = {
        .sampleRateShading = vk::True,
        .samplerAnisotropy = vk::True,
    };

    constexpr float queue_priority = 1.0f;
    const vk::DeviceQueueCreateInfo queue_info{
        .queueFamilyIndex = engine.queue_family_index,
        .queueCount = 1,
        .pQueuePriorities = &queue_priority
    };

    const auto device = engine.gpu.createDevice({
        .pNext = synchronization2_feature,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queue_info,
        .enabledLayerCount = to_u32(ValidationLayers.size()),
        .ppEnabledLayerNames = ValidationLayers.data(),
        .enabledExtensionCount = to_u32(DeviceExtensions.size()),
        .ppEnabledExtensionNames = DeviceExtensions.data(),
        .pEnabledFeatures = &features,
    });
    if (device.result != vk::Result::eSuccess)
        return Err::CouldNotCreateVkDevice;

    ASSERT(device.value != nullptr);
    return ok(device.value);
}

static Result<VmaAllocator> init_allocator(const Engine& engine) {
    ASSERT(engine.instance != nullptr);
    ASSERT(engine.gpu != nullptr);
    ASSERT(engine.device != nullptr);

    VmaAllocatorCreateInfo info = {};
    info.physicalDevice = engine.gpu;
    info.device = engine.device;
    info.instance = engine.instance;
    info.vulkanApiVersion = VK_API_VERSION_1_3;

    auto allocator = ok<VmaAllocator>(nullptr);
    const auto result = vmaCreateAllocator(&info, &*allocator);
    if (result != VK_SUCCESS)
        return Err::CouldNotCreateVmaAllocator;
    return allocator;
}

static bool s_engine_initialized = false;

Result<Engine> Engine::create() {
    if (s_engine_initialized)
        ERROR("Cannot initialize more than one engine");

    auto engine = ok<Engine>();

    const auto glfw_success = glfwInit();
    if (glfw_success == GLFW_FALSE)
        return Err::CouldNotInitializeGlfw;
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    VULKAN_HPP_DEFAULT_DISPATCHER.init();

    const auto instance = init_instance();
    if (instance.has_err())
        return instance.err();
    engine->instance = *instance;

    VULKAN_HPP_DEFAULT_DISPATCHER.init(engine->instance);

    const auto messenger = engine->instance.createDebugUtilsMessengerEXT(DebugUtilsMessengerCreateInfo);
    if (messenger.result != vk::Result::eSuccess)
        return Err::CouldNotCreateVkDebugUtilsMessenger;
    engine->debug_messenger = messenger.value;

    const auto gpu = find_gpu(*engine);
    if (gpu.has_err())
        return gpu.err();
    engine->gpu = *gpu;

    const auto queue_family = find_queue_family(engine->gpu);
    if (queue_family.has_err())
        return queue_family.err();
    engine->queue_family_index = *queue_family;

    const auto device = init_device(*engine);
    if (device.has_err())
        return device.err();
    engine->device = *device;

    VULKAN_HPP_DEFAULT_DISPATCHER.init(engine->device);

    engine->queue = engine->device.getQueue(engine->queue_family_index, 0);
    if (engine->queue == nullptr)
        return Err::VkQueueUnavailable;

    const auto allocator = init_allocator(*engine);
    if (allocator.has_err())
        return allocator.err();
    engine->allocator = *allocator;

    const auto pool = engine->device.createCommandPool({
        .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        .queueFamilyIndex = engine->queue_family_index,
    });
    if (pool.result != vk::Result::eSuccess)
        return Err::CouldNotCreateVkCommandPool;
    engine->command_pool = pool.value;

    const auto transient_pool = engine->device.createCommandPool({
        .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        .queueFamilyIndex = engine->queue_family_index,
    });
    if (transient_pool.result != vk::Result::eSuccess)
        return Err::CouldNotCreateVkCommandPool;
    engine->single_time_command_pool = transient_pool.value;

    s_engine_initialized = true;
    ASSERT(engine->instance != nullptr);
    ASSERT(engine->debug_messenger != nullptr);
    ASSERT(engine->gpu != nullptr);
    ASSERT(engine->device != nullptr);
    ASSERT(engine->allocator != nullptr);
    ASSERT(engine->queue_family_index != UINT32_MAX);
    ASSERT(engine->queue != nullptr);
    ASSERT(engine->command_pool != nullptr);
    ASSERT(engine->single_time_command_pool != nullptr);
    return engine;
}

void Engine::destroy() const {
    if (!s_engine_initialized)
        ERROR("Cannot destroy uninitialized engine");

    ASSERT(single_time_command_pool != nullptr);
    device.destroyCommandPool(single_time_command_pool);
    ASSERT(command_pool != nullptr);
    device.destroyCommandPool(command_pool);

    ASSERT(allocator != nullptr);
    vmaDestroyAllocator(allocator);

    ASSERT(device != nullptr);
    device.destroy();

    ASSERT(debug_messenger != nullptr);
    instance.destroyDebugUtilsMessengerEXT(debug_messenger);

    ASSERT(instance != nullptr);
    instance.destroy();

    s_engine_initialized = false;
}

Result<Window> Window::create(const Engine& engine, const bool fullscreen, const i32 width, const i32 height) {
    ASSERT(engine.instance != nullptr);
    ASSERT(engine.device != nullptr);
    ASSERT(engine.command_pool != nullptr);
    if (!fullscreen) {
        ASSERT(width > 0);
        ASSERT(height > 0);
    }

    auto window = ok<Window>();

    if (fullscreen) {
        const auto monitor = glfwGetPrimaryMonitor();
        if (monitor == nullptr)
            return Err::GlfwFailure;

        const auto video_mode = glfwGetVideoMode(monitor);
        if (video_mode == nullptr)
            return Err::GlfwFailure;

        window->m_window = glfwCreateWindow(video_mode->width, video_mode->width, "Hurdy Gurdy", monitor, nullptr);
        if (window->m_window == nullptr)
            return Err::GlfwFailure;
    } else {
        window->m_window = glfwCreateWindow(width, height, "Hurdy Gurdy", nullptr, nullptr);
        if (window->m_window == nullptr)
            return Err::GlfwFailure;
    }

    VkSurfaceKHR surface = VK_NULL_HANDLE;
    const auto surface_result = glfwCreateWindowSurface(engine.instance, window->m_window, nullptr, &surface);
    if (surface_result != VK_SUCCESS)
        return Err::GlfwFailure;
    window->m_surface = surface;

    const auto window_result = window->resize(engine);
    if (window_result.has_err())
        return window_result.err();

    const vk::CommandBufferAllocateInfo cmd_info{
        .commandPool = engine.command_pool,
        .commandBufferCount = MaxFramesInFlight,
    };
    const auto cmd_result = engine.device.allocateCommandBuffers(&cmd_info, window->m_command_buffers.data());
    if (cmd_result != vk::Result::eSuccess)
        return Err::CouldNotAllocateVkCommandBuffers;

    for (auto& fence : window->m_frame_finished_fences) {
        const auto new_fence = engine.device.createFence({.flags = vk::FenceCreateFlagBits::eSignaled});
        if (new_fence.result != vk::Result::eSuccess)
            return Err::CouldNotCreateVkFence;
        fence = new_fence.value;
    }
    for (auto& semaphore : window->m_image_available_semaphores) {
        const auto new_semaphore = engine.device.createSemaphore({});
        if (new_semaphore.result != vk::Result::eSuccess)
            return Err::CouldNotCreateVkSemaphore;
        semaphore = new_semaphore.value;
    }
    for (auto& semaphore : window->m_ready_to_present_semaphores) {
        const auto new_semaphore = engine.device.createSemaphore({});
        if (new_semaphore.result != vk::Result::eSuccess)
            return Err::CouldNotCreateVkSemaphore;
        semaphore = new_semaphore.value;
    }

    ASSERT(window->m_window != nullptr);
    ASSERT(window->m_surface != nullptr);
    ASSERT(window->m_extent != nullptr);
    ASSERT(window->m_swapchain != nullptr);
    for (const auto image : window->m_swapchain_images) {
        ASSERT(image != nullptr);
    }
    for (const auto cmd : window->m_command_buffers) {
        ASSERT(cmd != nullptr);
    }
    for (const auto fence : window->m_frame_finished_fences) {
        ASSERT(fence != nullptr);
    }
    for (const auto semaphore : window->m_image_available_semaphores) {
        ASSERT(semaphore != nullptr);
    }
    for (const auto semaphore : window->m_ready_to_present_semaphores) {
        ASSERT(semaphore != nullptr);
    }
    return window;
}

void Window::destroy(const Engine& engine) const {
    ASSERT(engine.device != nullptr);

    for (const auto fence : m_frame_finished_fences) {
        ASSERT(fence != nullptr);
        engine.device.destroyFence(fence);
    }
    for (const auto semaphore : m_image_available_semaphores) {
        ASSERT(semaphore != nullptr);
        engine.device.destroySemaphore(semaphore);
    }
    for (const auto semaphore : m_ready_to_present_semaphores) {
        ASSERT(semaphore != nullptr);
        engine.device.destroySemaphore(semaphore);
    }

    ASSERT(engine.command_pool != nullptr);
    for (const auto cmd : m_command_buffers) {
        ASSERT(cmd != nullptr);
    }
    engine.device.freeCommandBuffers(engine.command_pool, to_u32(m_command_buffers.size()), m_command_buffers.data());

    ASSERT(m_swapchain != nullptr);
    engine.device.destroySwapchainKHR(m_swapchain);

    ASSERT(m_surface != nullptr);
    engine.instance.destroySurfaceKHR(m_surface);

    ASSERT(m_window != nullptr);
    glfwDestroyWindow(m_window);
}

Result<void> Window::resize(const Engine& engine) {
    ASSERT(engine.gpu != nullptr);
    ASSERT(engine.device != nullptr);

    const auto [surface_result, surface_capabilities] = engine.gpu.getSurfaceCapabilitiesKHR(m_surface);
    if (surface_result != vk::Result::eSuccess)
        return Err::VulkanFailure;
    if (surface_capabilities.currentExtent.width <= 0 || surface_capabilities.currentExtent.height <= 0)
        return Err::InvalidWindowSize;

    const auto [present_mode_result, present_modes] = engine.gpu.getSurfacePresentModesKHR(m_surface);
    if (present_mode_result != vk::Result::eSuccess)
        return Err::VulkanFailure;

    const auto new_swapchain = engine.device.createSwapchainKHR({
        .surface = m_surface,
        .minImageCount = surface_capabilities.maxImageCount == 0
                         ? MaxSwapchainImages
                         : std::min(surface_capabilities.minImageCount + 1, surface_capabilities.maxImageCount),
        .imageFormat = SwapchainImageFormat,
        .imageColorSpace = SwapchainColorSpace,
        .imageExtent = surface_capabilities.currentExtent,
        .imageArrayLayers = 1,
        .imageUsage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst,
        .preTransform = surface_capabilities.currentTransform,
        .presentMode = std::ranges::any_of(present_modes, [](const vk::PresentModeKHR mode) { return mode == vk::PresentModeKHR::eMailbox; })
            // ? vk::PresentModeKHR::eMailbox
            ? vk::PresentModeKHR::eFifo
            : vk::PresentModeKHR::eFifo,
        .clipped = vk::True,
        .oldSwapchain = m_swapchain,
    });
    if (new_swapchain.result != vk::Result::eSuccess)
        return Err::CouldNotCreateVkSwapchain;

    const auto wait_result = engine.queue.waitIdle();
    if (wait_result != vk::Result::eSuccess)
        return Err::CouldNotWaitForVkQueue;

    if (m_swapchain != nullptr) {
        engine.device.destroySwapchainKHR(m_swapchain);
    }
    m_swapchain = new_swapchain.value;
    m_extent = surface_capabilities.currentExtent;

    const vk::Result image_count_result = engine.device.getSwapchainImagesKHR(m_swapchain, &m_image_count, nullptr);
    if (image_count_result != vk::Result::eSuccess)
        return Err::VkSwapchainImagesUnavailable;
    const vk::Result image_result = engine.device.getSwapchainImagesKHR(m_swapchain, &m_image_count, m_swapchain_images.data());
    if (image_result != vk::Result::eSuccess)
        return Err::VkSwapchainImagesUnavailable;

    ASSERT(m_swapchain != nullptr);
    for (const auto image : m_swapchain_images) {
        ASSERT(image != nullptr);
    }
    return ok();
}

Result<vk::CommandBuffer> Window::begin_frame(const Engine& engine) {
    ASSERT(!m_recording);
    ASSERT(current_cmd() != nullptr);
    ASSERT(is_frame_finished() != nullptr);
    ASSERT(is_image_available() != nullptr);
    ASSERT(engine.device != nullptr);

    const auto wait_result = engine.device.waitForFences({is_frame_finished()}, vk::True, 1'000'000'000);
    if (wait_result != vk::Result::eSuccess)
        return Err::CouldNotWaitForVkFence;
    const auto reset_result = engine.device.resetFences({is_frame_finished()});
    if (reset_result != vk::Result::eSuccess)
        return Err::VulkanFailure;

    const auto acquire_result = engine.device.acquireNextImageKHR(m_swapchain, UINT64_MAX, is_image_available(), nullptr);
    if (acquire_result.result != vk::Result::eSuccess)
        return Err::CouldNotAcquireVkSwapchainImage;
    m_current_image_index = acquire_result.value;

    const auto begin_result = current_cmd().begin({.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
    if (begin_result != vk::Result::eSuccess)
        return Err::CouldNotBeginVkCommandBuffer;
    m_recording = true;

    const vk::Viewport viewport = {
        0.0f, 0.0f,
        static_cast<f32>(m_extent.width), static_cast<f32>(m_extent.height),
        0.0f, 1.0f,
    };
    current_cmd().setViewportWithCount({viewport});
    const vk::Rect2D scissor = {{0, 0}, m_extent};
    current_cmd().setScissorWithCount({scissor});

    current_cmd().setRasterizerDiscardEnable(vk::False);
    current_cmd().setPrimitiveRestartEnable(vk::False);
    current_cmd().setPrimitiveTopology(vk::PrimitiveTopology::eTriangleList);
    current_cmd().setPolygonModeEXT(vk::PolygonMode::eFill);
    current_cmd().setFrontFace(vk::FrontFace::eCounterClockwise);
    current_cmd().setCullMode(vk::CullModeFlagBits::eNone);
    current_cmd().setDepthTestEnable(vk::True);
    current_cmd().setDepthWriteEnable(vk::True);
    current_cmd().setDepthCompareOp(vk::CompareOp::eLess);
    current_cmd().setDepthBiasEnable(vk::False);
    current_cmd().setStencilTestEnable(vk::False);
    current_cmd().setRasterizationSamplesEXT(vk::SampleCountFlagBits::e1);
    current_cmd().setSampleMaskEXT(vk::SampleCountFlagBits::e1, vk::SampleMask{0xff});
    current_cmd().setAlphaToCoverageEnableEXT(vk::False);
    current_cmd().setColorWriteMaskEXT(0, {
        vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
        vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA
    });
    current_cmd().setColorBlendEnableEXT(0, {vk::False});

    return ok(current_cmd());
}

Result<void> Window::end_frame(const Engine& engine) {
    ASSERT(m_recording);
    ASSERT(m_swapchain != nullptr);
    ASSERT(current_cmd() != nullptr);
    ASSERT(is_image_available() != nullptr);
    ASSERT(is_ready_to_present() != nullptr);
    ASSERT(engine.device != nullptr);

    const auto end_result = current_cmd().end();
    if (end_result != vk::Result::eSuccess)
        return Err::CouldNotEndVkCommandBuffer;
    m_recording = false;

    constexpr vk::PipelineStageFlags wait_stage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    vk::SubmitInfo submit_info = {
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &is_image_available(),
        .pWaitDstStageMask = &wait_stage,
        .commandBufferCount = 1,
        .pCommandBuffers = &current_cmd(),
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &is_ready_to_present(),
    };

    const auto submit_result = engine.queue.submit({submit_info}, is_frame_finished());
    if (submit_result != vk::Result::eSuccess)
        return Err::CouldNotSubmitVkCommandBuffer;

    const vk::PresentInfoKHR present_info = {
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &is_ready_to_present(),
        .swapchainCount = 1,
        .pSwapchains = &m_swapchain,
        .pImageIndices = &m_current_image_index,
    };
    const auto present_result = engine.queue.presentKHR(present_info);
    if (present_result == vk::Result::eErrorOutOfDateKHR)
        return Err::InvalidWindowSize;
    if (present_result != vk::Result::eSuccess)
        return Err::CouldNotPresentVkSwapchainImage;

    m_current_frame_index = (m_current_frame_index + 1) % MaxFramesInFlight;
    return ok();
}

Result<GpuBuffer> GpuBuffer::create_result(const Engine& engine, const Config& config) {
    ASSERT(engine.allocator != nullptr);
    ASSERT(config.size != 0);
    ASSERT(config.usage != vk::BufferUsageFlags{});

    VkBufferCreateInfo buffer_info = {};
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size = config.size;
    buffer_info.usage = static_cast<VkBufferUsageFlags>(config.usage);

    VmaAllocationCreateInfo alloc_info = {};
    if (config.memory_type == RandomAccess) {
        alloc_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
        alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
    } else if (config.memory_type == Staging) {
        alloc_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
        alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
    } else if (config.memory_type == DeviceLocal) {
        alloc_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
        alloc_info.flags = 0;
    } else {
        ERROR("Invalid buffer memory type");
    }

    VkBuffer buffer = nullptr;
    VmaAllocation allocation = nullptr;
    const auto buffer_result = vmaCreateBuffer(engine.allocator, &buffer_info, &alloc_info, &buffer, &allocation, nullptr);
    if (buffer_result != VK_SUCCESS) {
        return Err::CouldNotCreateGpuBuffer;
    }

    ASSERT(allocation != nullptr);
    ASSERT(buffer != nullptr);
    return ok<GpuBuffer>(allocation, buffer, config.memory_type);
}

Result<void> GpuBuffer::write_result(
    const Engine& engine, const void* data, const vk::DeviceSize size, const vk::DeviceSize offset
) const {
    ASSERT(engine.allocator != nullptr);
    ASSERT(allocation != nullptr);
    ASSERT(buffer != nullptr);
    ASSERT(data != nullptr);
    ASSERT(size != 0);
    if (memory_type == Staging)
        ASSERT(offset == 0);

    if (memory_type == RandomAccess || memory_type == Staging) {
        const auto copy_result = vmaCopyMemoryToAllocation(engine.allocator, data, allocation, offset, size);
        if (copy_result != VK_SUCCESS) {
            return Err::CouldNotWriteGpuBuffer;
        }
        return ok();
    }
    ASSERT(memory_type == DeviceLocal);

    const auto staging_buffer = create_result(engine, {size, vk::BufferUsageFlagBits::eTransferSrc, Staging});
    if (staging_buffer.has_err())
        return staging_buffer.err();
    defer(vmaDestroyBuffer(engine.allocator, staging_buffer->buffer, staging_buffer->allocation));
    const auto copy_result = vmaCopyMemoryToAllocation(engine.allocator, data, staging_buffer->allocation, 0, size);
    if (copy_result != VK_SUCCESS)
        return staging_buffer.err();

    auto submit = submit_single_time_commands(engine, [&](const vk::CommandBuffer cmd) {
        cmd.copyBuffer(staging_buffer->buffer, buffer, {vk::BufferCopy(offset, 0, size)});
    });
    if (submit.has_err())
        return Err::CouldNotWriteGpuBuffer;
    return ok();
}

Result<StagingGpuImage> StagingGpuImage::create(const Engine& engine, const Config& config) {
    ASSERT(engine.device != nullptr);
    ASSERT(engine.allocator != nullptr);
    ASSERT(config.extent.width > 0);
    ASSERT(config.extent.height > 0);
    ASSERT(config.extent.depth > 0);
    ASSERT(config.format != vk::Format::eUndefined);
    ASSERT(config.usage != vk::ImageUsageFlags{});
    ASSERT(config.sample_count != vk::SampleCountFlagBits{});
    ASSERT(config.mip_levels > 0);

    VkImageType dimensions = VK_IMAGE_TYPE_3D;
    if (config.extent.depth == 1) {
        dimensions = static_cast<VkImageType>(dimensions - 1);
        if (config.extent.height == 1)
            dimensions = static_cast<VkImageType>(dimensions - 1);
    }
    VkImageCreateInfo image_info = {};
    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.imageType = dimensions;
    image_info.format = static_cast<VkFormat>(config.format);
    image_info.extent = config.extent;
    image_info.mipLevels = config.mip_levels;
    image_info.arrayLayers = 1;
    image_info.samples = static_cast<VkSampleCountFlagBits>(config.sample_count);
    image_info.usage = static_cast<VkImageUsageFlags>(config.usage);

    VmaAllocationCreateInfo alloc_info = {};
    alloc_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
    alloc_info.flags = 0;

    VkImage image = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;
    const auto image_result = vmaCreateImage(engine.allocator, &image_info, &alloc_info, &image, &allocation, nullptr);
    if (image_result != VK_SUCCESS)
        return Err::CouldNotCreateGpuImage;

    ASSERT(allocation != nullptr);
    ASSERT(image != nullptr);
    return ok<StagingGpuImage>(allocation, image);
}

Result<void> StagingGpuImage::write(
    const Engine& engine, const Data& data, const vk::ImageLayout final_layout,
    const vk::ImageSubresourceRange& subresource
) const {
    ASSERT(engine.allocator != nullptr);
    ASSERT(allocation != nullptr);
    ASSERT(image != nullptr);
    ASSERT(data.ptr != nullptr);
    ASSERT(data.alignment > 0);
    ASSERT(data.extent.width > 0);
    ASSERT(data.extent.height > 0);
    ASSERT(data.extent.depth > 0);

    const VkDeviceSize size = data.extent.width * data.extent.height * data.extent.depth * data.alignment;

    const auto staging_buffer = GpuBuffer::create_result(engine, {
        size, vk::BufferUsageFlagBits::eTransferSrc, GpuBuffer::MemoryType::Staging
    });
    if (staging_buffer.has_err())
        return staging_buffer.err();
    defer(vmaDestroyBuffer(engine.allocator, staging_buffer->buffer, staging_buffer->allocation));
    const auto staging_write = staging_buffer->write_result(engine, data.ptr, size, 0);
    if (staging_write.has_err())
        return staging_write.err();

    const auto submit_result = submit_single_time_commands(engine, [&](const vk::CommandBuffer cmd) {
        BarrierBuilder(cmd)
            .add_image_barrier(image, subresource)
            .set_image_dst(vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eTransferWrite, vk::ImageLayout::eTransferDstOptimal)
            .build_and_run();

        const vk::BufferImageCopy2 copy_region = {
            .imageSubresource = {subresource.aspectMask, 0, 0, 1},
            .imageExtent = data.extent
        };
        cmd.copyBufferToImage2({
            .srcBuffer = staging_buffer->buffer,
            .dstImage = image,
            .dstImageLayout = vk::ImageLayout::eTransferDstOptimal,
            .regionCount = 1,
            .pRegions = &copy_region
        });

        BarrierBuilder(cmd)
            .add_image_barrier(image, subresource)
            .set_image_src(vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eTransferWrite, vk::ImageLayout::eTransferDstOptimal)
            .set_image_dst(vk::PipelineStageFlagBits2::eNone, vk::AccessFlagBits2::eNone, final_layout)
            .build_and_run();
    });
    if (submit_result.has_err())
        return Err::CouldNotWriteGpuImage;

    return ok();
}

Result<GpuImage> GpuImage::create_result(const Engine& engine, const Config& config) {
    ASSERT(engine.device != nullptr);
    ASSERT(engine.allocator != nullptr);
    ASSERT(config.extent.width > 0);
    ASSERT(config.extent.height > 0);
    ASSERT(config.extent.depth > 0);
    ASSERT(config.format != vk::Format::eUndefined);
    ASSERT(config.usage != vk::ImageUsageFlags{});
    ASSERT(config.aspect_flags != vk::ImageAspectFlagBits{});
    ASSERT(config.sample_count != vk::SampleCountFlagBits{});
    ASSERT(config.mip_levels > 0);

    const auto staging = StagingGpuImage::create(engine, {
        config.extent, config.format, config.usage, config.sample_count, config.mip_levels
    });
    if (staging.has_err())
        return Err::CouldNotCreateGpuImage;

    vk::ImageViewType dimensions = vk::ImageViewType::e3D;
    if (config.extent.depth == 1) {
        dimensions = static_cast<vk::ImageViewType>(static_cast<int>(dimensions) - 1);
        if (config.extent.height == 1)
            dimensions = static_cast<vk::ImageViewType>(static_cast<int>(dimensions) - 1);
    }
    const auto view = engine.device.createImageView({
        .image = staging->image,
        .viewType = dimensions,
        .format = config.format,
        .subresourceRange = {config.aspect_flags, 0, config.mip_levels, 0, 1},
    });
    if (view.result != vk::Result::eSuccess)
        return Err::CouldNotCreateGpuImageView;

    if (config.layout != vk::ImageLayout::eUndefined) {
        const auto submit_result = submit_single_time_commands(engine, [&](const vk::CommandBuffer cmd) {
            BarrierBuilder(cmd)
                .add_image_barrier(staging->image, {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1})
                .set_image_dst(vk::PipelineStageFlagBits2::eNone, vk::AccessFlagBits2::eNone, config.layout)
                .build_and_run();
        });
        if (submit_result.has_err())
            return Err::CouldNotCreateGpuImage;
    }

    ASSERT(staging->allocation != nullptr);
    ASSERT(staging->image != nullptr);
    ASSERT(view.value != nullptr);
    return ok<GpuImage>(staging->allocation, staging->image, view.value);
}

Result<GpuImage> GpuImage::create_cubemap(const Engine& engine, const std::filesystem::path path) {
    ASSERT(engine.device != nullptr);
    ASSERT(engine.allocator != nullptr);
    ASSERT(!path.empty());

    const auto data = ImageData::load(path);
    if (data.has_err())
        return data.err();

    const vk::Extent3D staging_extent = {to_u32(data->width), to_u32(data->height), 1};

    const auto staging_image = StagingGpuImage::create(engine, {
        .extent = staging_extent, 
        .format = vk::Format::eR8G8B8A8Srgb, 
        .usage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc
    });
    if (staging_image.has_err())
        return staging_image.err();
    defer(staging_image->destroy(engine));
    const auto staging_write = staging_image->write(engine,
        {data->pixels.get(), 4, staging_extent},
        vk::ImageLayout::eTransferSrcOptimal
    );
    if (staging_write.has_err())
        return staging_write.err();

    const vk::Extent3D extent = {staging_extent.width / 4, staging_extent.height / 3, 1};

    VkImageCreateInfo image_info = {};
    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.imageType = VK_IMAGE_TYPE_2D;
    image_info.format = VK_FORMAT_R8G8B8A8_SRGB;
    image_info.extent = extent;
    image_info.mipLevels = 1;
    image_info.arrayLayers = 1;
    image_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    image_info.arrayLayers = 6;
    image_info.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

    VmaAllocationCreateInfo alloc_info = {};
    alloc_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
    alloc_info.flags = 0;

    VkImage image = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;
    const auto image_result = vmaCreateImage(engine.allocator, &image_info, &alloc_info, &image, &allocation, nullptr);
    if (image_result != VK_SUCCESS)
        return Err::CouldNotCreateGpuImage;

    const auto view = engine.device.createImageView({
        .image = image,
        .viewType = vk::ImageViewType::eCube,
        .format = vk::Format::eR8G8B8A8Srgb,
        .subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 6},
    });
    if (view.result != vk::Result::eSuccess)
        return Err::CouldNotCreateGpuImageView;

    const auto submit_result = submit_single_time_commands(engine, [&](const vk::CommandBuffer cmd) {
        BarrierBuilder(cmd)
            .add_image_barrier(image, {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 6})
            .set_image_dst(vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eTransferWrite, vk::ImageLayout::eTransferDstOptimal)
            .build_and_run();

        std::array copies = {
            vk::ImageCopy2{
                .srcSubresource = {vk::ImageAspectFlagBits::eColor, 0, 0, 1},
                .srcOffset = {data->width * 2 / 4, data->height * 1 / 3, 0},
                .dstSubresource = {vk::ImageAspectFlagBits::eColor, 0, 0, 1},
                .extent = extent,
            },
            vk::ImageCopy2{
                .srcSubresource = {vk::ImageAspectFlagBits::eColor, 0, 0, 1},
                .srcOffset = {data->width * 0 / 4, data->height * 1 / 3, 0},
                .dstSubresource = {vk::ImageAspectFlagBits::eColor, 0, 1, 1},
                .extent = extent,
            },
            vk::ImageCopy2{
                .srcSubresource = {vk::ImageAspectFlagBits::eColor, 0, 0, 1},
                .srcOffset = {data->width * 1 / 4, data->height * 0 / 3, 0},
                .dstSubresource = {vk::ImageAspectFlagBits::eColor, 0, 2, 1},
                .extent = extent,
            },
            vk::ImageCopy2{
                .srcSubresource = {vk::ImageAspectFlagBits::eColor, 0, 0, 1},
                .srcOffset = {data->width * 1 / 4, data->height * 2 / 3, 0},
                .dstSubresource = {vk::ImageAspectFlagBits::eColor, 0, 3, 1},
                .extent = extent,
            },
            vk::ImageCopy2{
                .srcSubresource = {vk::ImageAspectFlagBits::eColor, 0, 0, 1},
                .srcOffset = {data->width * 1 / 4, data->height * 1 / 3, 0},
                .dstSubresource = {vk::ImageAspectFlagBits::eColor, 0, 4, 1},
                .extent = extent,
            },
            vk::ImageCopy2{
                .srcSubresource = {vk::ImageAspectFlagBits::eColor, 0, 0, 1},
                .srcOffset = {data->width * 3 / 4, data->height * 1 / 3, 0},
                .dstSubresource = {vk::ImageAspectFlagBits::eColor, 0, 5, 1},
                .extent = extent,
            },
        };
        cmd.copyImage2({
            .srcImage = staging_image->image,
            .srcImageLayout = vk::ImageLayout::eTransferSrcOptimal,
            .dstImage = image,
            .dstImageLayout = vk::ImageLayout::eTransferDstOptimal,
            .regionCount = to_u32(copies.size()),
            .pRegions = copies.data(),
        });

        BarrierBuilder(cmd)
            .add_image_barrier(image, {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 6})
            .set_image_src(vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eTransferWrite, vk::ImageLayout::eTransferDstOptimal)
            .set_image_dst(vk::PipelineStageFlagBits2::eFragmentShader, vk::AccessFlagBits2::eShaderRead, vk::ImageLayout::eShaderReadOnlyOptimal)
            .build_and_run();
    });
    if (submit_result.has_err())
        return Err::CouldNotWriteGpuImage;

    ASSERT(allocation != nullptr);
    ASSERT(image != nullptr);
    ASSERT(view.value != nullptr);
    return ok<GpuImage>(allocation, image, view.value);
}

Result<void> GpuImage::generate_mipmaps_result(
    const Engine& engine,
    const u32 mip_levels,
    const vk::Extent3D extent,
    const vk::Format format,
    const vk::ImageLayout final_layout
) const {
    ASSERT(engine.gpu != nullptr);
    ASSERT(image != nullptr);
    ASSERT(mip_levels > 1);
    ASSERT(extent.width > 0);
    ASSERT(extent.height > 0);
    ASSERT(extent.depth > 0);
    ASSERT(format != vk::Format::eUndefined);
    ASSERT(final_layout != vk::ImageLayout::eUndefined);

    const auto format_properties = engine.gpu.getFormatProperties(format);
    if (!(format_properties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImageFilterLinear))
        return Err::CouldNotGenerateMipmaps;

    const auto submit_result = submit_single_time_commands(engine, [&](const vk::CommandBuffer cmd) {
        vk::Offset3D mip_offset = {to_i32(extent.width), to_i32(extent.height), to_i32(extent.depth)};

        BarrierBuilder(cmd)
            .add_image_barrier(image, {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1})
            .set_image_dst(vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eTransferRead, vk::ImageLayout::eTransferSrcOptimal)
            .build_and_run();

        for (u32 level = 0; level < mip_levels - 1; ++level) {
            BarrierBuilder(cmd)
                .add_image_barrier(image, {vk::ImageAspectFlagBits::eColor, level + 1, 1, 0, 1})
                .set_image_dst(vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eTransferWrite, vk::ImageLayout::eTransferDstOptimal)
                .build_and_run();

            vk::ImageBlit2 region = {
                .srcSubresource = {vk::ImageAspectFlagBits::eColor, level, 0, 1},
                .dstSubresource = {vk::ImageAspectFlagBits::eColor, level + 1, 0, 1},
            };
            region.srcOffsets[1] = mip_offset;
            if (mip_offset.x > 1)
                mip_offset.x /= 2;
            if (mip_offset.y > 1)
                mip_offset.y /= 2;
            if (mip_offset.z > 1)
                mip_offset.z /= 2;
            region.dstOffsets[1] = mip_offset;

            cmd.blitImage2({
                .srcImage = image,
                .srcImageLayout = vk::ImageLayout::eTransferSrcOptimal,
                .dstImage = image,
                .dstImageLayout = vk::ImageLayout::eTransferDstOptimal,
                .regionCount = 1,
                .pRegions = &region,
                .filter = vk::Filter::eLinear,
            });

            BarrierBuilder(cmd)
                .add_image_barrier(image, {vk::ImageAspectFlagBits::eColor, level + 1, 1, 0, 1})
                .set_image_src(vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eTransferWrite, vk::ImageLayout::eTransferDstOptimal)
                .set_image_dst(vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eTransferRead, vk::ImageLayout::eTransferSrcOptimal)
                .build_and_run();
        }

        BarrierBuilder(cmd)
            .add_image_barrier(image, {vk::ImageAspectFlagBits::eColor, 0, mip_levels, 0, 1})
            .set_image_src(vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eTransferRead, vk::ImageLayout::eTransferSrcOptimal)
            .set_image_dst(vk::PipelineStageFlagBits2::eNone, vk::AccessFlagBits2::eNone, final_layout)
            .build_and_run();
    });
    if (submit_result.has_err())
        return Err::CouldNotGenerateMipmaps;

    return ok();
}

Result<vk::Sampler> create_sampler_result(const Engine& engine, const SamplerConfig& config) {
    ASSERT(engine.device != nullptr);
    ASSERT(engine.gpu != nullptr);
    ASSERT(config.mip_levels >= 1);
    const auto limits = engine.gpu.getProperties().limits;

    vk::SamplerCreateInfo sampler_info = {
        .addressModeU = config.edge_mode,
        .addressModeV = config.edge_mode,
        .addressModeW = config.edge_mode,
        .anisotropyEnable = vk::True,
        .maxAnisotropy = limits.maxSamplerAnisotropy,
        .maxLod = static_cast<f32>(config.mip_levels),
        .borderColor = vk::BorderColor::eIntOpaqueBlack,
    };
    if (config.type == SamplerType::Linear) {
        sampler_info.magFilter = vk::Filter::eLinear;
        sampler_info.minFilter = vk::Filter::eLinear;
        sampler_info.mipmapMode = vk::SamplerMipmapMode::eLinear;
    } else if (config.type == SamplerType::Nearest) {
        sampler_info.magFilter = vk::Filter::eNearest;
        sampler_info.minFilter = vk::Filter::eNearest;
        sampler_info.mipmapMode = vk::SamplerMipmapMode::eNearest;
    } else {
        ERROR("Invalid sampler type");
    }

    const auto sampler = engine.device.createSampler(sampler_info);
    if (sampler.result != vk::Result::eSuccess)
        return Err::CouldNotCreateVkSampler;

    ASSERT(sampler.value != nullptr);
    return ok(sampler.value);
}

Result<vk::DescriptorPool> create_descriptor_pool(
    const Engine& engine, const u32 max_sets,
    const std::span<const vk::DescriptorPoolSize> descriptors
) {
    ASSERT(engine.device != nullptr);
    ASSERT(max_sets >= 1);
    ASSERT(!descriptors.empty());

    const auto pool = engine.device.createDescriptorPool({
        .maxSets = max_sets, 
        .poolSizeCount = to_u32(descriptors.size()), 
        .pPoolSizes = descriptors.data(),
    });
    if (pool.result != vk::Result::eSuccess)
        return Err::CouldNotCreateVkDescriptorPool;

    ASSERT(pool.value != nullptr);
    return ok(pool.value);
}

Result<vk::DescriptorSetLayout> create_descriptor_set_layout(
    const Engine& engine,
    const std::span<const vk::DescriptorSetLayoutBinding> bindings,
    const std::span<const vk::DescriptorBindingFlags> flags
) {
    ASSERT(engine.device != nullptr);
    ASSERT(!bindings.empty());
    if (!flags.empty())
        ASSERT(flags.size() == bindings.size());

    const vk::DescriptorSetLayoutBindingFlagsCreateInfo flag_info = {
        .bindingCount = to_u32(flags.size()),
        .pBindingFlags = flags.data(),
    };
    const auto layout = engine.device.createDescriptorSetLayout({
        .pNext = flags.empty() ? nullptr : &flag_info,
        .bindingCount = to_u32(bindings.size()),
        .pBindings = bindings.data(),
    });
    if (layout.result != vk::Result::eSuccess)
        return Err::CouldNotCreateVkDescriptorSetLayout;

    ASSERT(layout.value != nullptr);
    return ok(layout.value);
}

Result<void> allocate_descriptor_sets(
    const Engine& engine, const vk::DescriptorPool pool,
    const std::span<const vk::DescriptorSetLayout> layouts,
    const std::span<vk::DescriptorSet> out_sets
) {
    ASSERT(engine.device != nullptr);
    ASSERT(pool != nullptr);
    ASSERT(!layouts.empty());
    for (const auto layout : layouts) {
        ASSERT(layout != nullptr);
    }
    ASSERT(!out_sets.empty());
    for (const auto set : out_sets) {
        ASSERT(set == nullptr);
    }
    ASSERT(layouts.size() == out_sets.size());

    const vk::DescriptorSetAllocateInfo alloc_info = {
        .descriptorPool = pool,
        .descriptorSetCount = to_u32(layouts.size()),
        .pSetLayouts = layouts.data(),
    };
    const auto set_result = engine.device.allocateDescriptorSets(&alloc_info, out_sets.data());
    if (set_result != vk::Result::eSuccess)
        return Err::CouldNotAllocateVkDescriptorSets;

    for (const auto set : out_sets) {
        ASSERT(set != nullptr);
    }
    return ok();
}

void write_uniform_buffer_descriptor(
    const Engine& engine, const vk::DescriptorSet set, const u32 binding,
    const vk::Buffer buffer, const vk::DeviceSize size, const vk::DeviceSize offset,
    const u32 binding_array_index
) {
    ASSERT(engine.device != nullptr);
    ASSERT(set != nullptr);
    ASSERT(buffer != nullptr);
    ASSERT(size != 0);

    const vk::DescriptorBufferInfo buffer_info = {buffer, offset, size};
    const vk::WriteDescriptorSet descriptor_write = {
        .dstSet = set,
        .dstBinding = binding,
        .dstArrayElement = binding_array_index,
        .descriptorCount = 1,
        .descriptorType = vk::DescriptorType::eUniformBuffer,
        .pBufferInfo = &buffer_info,
    };
    engine.device.updateDescriptorSets({descriptor_write}, {});
}

void write_image_sampler_descriptor(
    const Engine& engine, const vk::DescriptorSet set, const u32 binding,
    const vk::Sampler sampler, const vk::ImageView view,
    const u32 binding_array_index
) {
    ASSERT(engine.device != nullptr);
    ASSERT(set != nullptr);
    ASSERT(sampler != nullptr);
    ASSERT(view != nullptr);

    const vk::DescriptorImageInfo image_info = {sampler, view, vk::ImageLayout::eShaderReadOnlyOptimal};
    const vk::WriteDescriptorSet descriptor_write = {
        .dstSet = set,
        .dstBinding = binding,
        .dstArrayElement = binding_array_index,
        .descriptorCount = 1,
        .descriptorType = vk::DescriptorType::eCombinedImageSampler,
        .pImageInfo = &image_info,
    };
    engine.device.updateDescriptorSets({descriptor_write}, {});
}

static Result<std::vector<char>> read_shader(const std::filesystem::path path) {
    ASSERT(!path.empty());

    auto file = std::ifstream{path, std::ios::ate | std::ios::binary};
    if (!file.is_open())
        return Err::ShaderFileNotFound;

    const usize code_size = file.tellg();
    auto code = ok<std::vector<char>>();
    code->resize(code_size);
    file.seekg(0);
    file.read(code->data(), static_cast<std::streamsize>(code->size()));
    file.close();

    if (code->empty())
        return Err::ShaderFileInvalid;

    ASSERT(!code->empty());
    return code;
}

Result<vk::ShaderEXT> create_unlinked_shader(const Engine& engine, const ShaderConfig& config) {
    ASSERT(engine.device != nullptr);
    ASSERT(!config.path.empty());
    ASSERT(config.code_type == vk::ShaderCodeTypeEXT::eSpirv && "binary shader code types untested");
    ASSERT(config.stage != vk::ShaderStageFlagBits{0});

    const auto code = read_shader(config.path);
    if (code.has_err())
        return code.err();

    const auto shader = engine.device.createShaderEXT({
        .flags = config.flags,
        .stage = config.stage,
        .nextStage = config.next_stage,
        .codeType = config.code_type,
        .codeSize = code->size(),
        .pCode = code->data(),
        .pName = "main",
        .setLayoutCount = to_u32(config.set_layouts.size()),
        .pSetLayouts = config.set_layouts.data(),
        .pushConstantRangeCount = to_u32(config.push_ranges.size()),
        .pPushConstantRanges = config.push_ranges.data(),
    });
    if (shader.result != vk::Result::eSuccess)
        return Err::CouldNotCreateVkShader;

    ASSERT(shader.value != nullptr);
    return ok(shader.value);
}

Result<void> create_linked_shaders(
    const Engine& engine,
    const std::span<vk::ShaderEXT> out_shaders,
    const std::span<const ShaderConfig> configs
) {
    ASSERT(engine.device != nullptr);
    ASSERT(out_shaders.size() >= 2);
    ASSERT(configs.size() >= 2);
    ASSERT(out_shaders.size() == configs.size());
    for (const auto shader : out_shaders) {
        ASSERT(shader == nullptr);
    }
    for (const auto& config : configs) {
        ASSERT(!config.path.empty());
        ASSERT(config.code_type == vk::ShaderCodeTypeEXT::eSpirv && "binary shader code types untested");
        ASSERT(config.stage != vk::ShaderStageFlagBits{0});
    }

    std::vector<std::vector<char>> codes = {};
    codes.reserve(configs.size());
    for (const auto& config : configs) {
        ASSERT(!config.path.empty());
        ASSERT(config.code_type == vk::ShaderCodeTypeEXT::eSpirv && "binary shader code types untested");
        ASSERT(config.stage != vk::ShaderStageFlagBits{0});

        const auto code = read_shader(config.path);
        if (code.has_err())
            return code.err();
        codes.emplace_back(std::move(*code));
    }

    std::vector<vk::ShaderCreateInfoEXT> shader_infos = {};
    shader_infos.reserve(configs.size());
    for (usize i = 0; i < codes.size(); ++i) {
        shader_infos.push_back({
            .flags = configs[i].flags | vk::ShaderCreateFlagBitsEXT::eLinkStage,
            .stage = configs[i].stage,
            .nextStage = configs[i].next_stage,
            .codeType = configs[i].code_type,
            .codeSize = codes[i].size(),
            .pCode = codes[i].data(),
            .pName = "main",
            .setLayoutCount = to_u32(configs[i].set_layouts.size()),
            .pSetLayouts = configs[i].set_layouts.data(),
            .pushConstantRangeCount = to_u32(configs[i].push_ranges.size()),
            .pPushConstantRanges = configs[i].push_ranges.data(),
        });
    }

    const auto shader_result = engine.device.createShadersEXT(to_u32(shader_infos.size()), shader_infos.data(), nullptr, out_shaders.data());
    if (shader_result != vk::Result::eSuccess)
        return Err::CouldNotCreateVkShader;

    for (const auto shader : out_shaders) {
        ASSERT(shader != nullptr);
    }
    return ok();
}

Result<vk::CommandBuffer> begin_single_time_commands(const Engine& engine) {
    ASSERT(engine.device != nullptr);
    ASSERT(engine.single_time_command_pool != nullptr);

    vk::CommandBufferAllocateInfo alloc_info = {
        .commandPool = engine.single_time_command_pool,
        .commandBufferCount = 1,
    };
    auto cmd = ok<vk::CommandBuffer>();
    const auto cmd_result = engine.device.allocateCommandBuffers(&alloc_info, &*cmd);
    if (cmd_result != vk::Result::eSuccess)
        return Err::CouldNotAllocateVkCommandBuffers;

    const auto begin_result = cmd->begin({.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
    if (begin_result != vk::Result::eSuccess)
        return Err::CouldNotBeginVkCommandBuffer;

    ASSERT(*cmd != nullptr);
    return cmd;
}

Result<void> end_single_time_commands(const Engine& engine, const vk::CommandBuffer cmd) {
    ASSERT(engine.device != nullptr);
    ASSERT(engine.single_time_command_pool != nullptr);
    ASSERT(cmd != nullptr);

    const auto end_result = cmd.end();
    if (end_result != vk::Result::eSuccess)
        return Err::CouldNotEndVkCommandBuffer;

    const auto fence = engine.device.createFence({});
    if (fence.result != vk::Result::eSuccess)
        return Err::CouldNotCreateVkFence;
    defer(engine.device.destroyFence(fence.value));

    vk::SubmitInfo submit_info = {
        .commandBufferCount = 1,
        .pCommandBuffers = &cmd,
    };
    const auto submit_result = engine.queue.submit({submit_info}, fence.value);
    if (submit_result != vk::Result::eSuccess)
        return Err::CouldNotSubmitVkCommandBuffer;

    const auto wait_result = engine.device.waitForFences({fence.value}, vk::True, UINT64_MAX);
    if (wait_result != vk::Result::eSuccess)
        return Err::CouldNotWaitForVkFence;

    engine.device.freeCommandBuffers(engine.single_time_command_pool, {cmd});
    return ok();
}

} // namespace hg
