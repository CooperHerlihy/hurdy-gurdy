#include "vulkan_internal.hpp"

#include "internal.hpp"
#include "hg/error.hpp"
#include "hg/utility.hpp"
#include "hg/array.hpp"

#include <imgui.h>
#include <backends/imgui_impl_vulkan.h>

#include <algorithm>

namespace hg {
using namespace vulkan;

static const char* deviceExtensions[]{
    "VK_KHR_swapchain",
};

#ifdef HG_VK_DEBUG_MESSENGER

static VkBool32 debugCallback(
    const VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    const VkDebugUtilsMessageTypeFlagsEXT type,
    const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
    void* userData)
{
    static_cast<void>(type);
    static_cast<void>(userData);

    if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
    {
        static_cast<void>(fprintf(stderr, "Vulkan Error: %s\n", callbackData->pMessage));
        abort();
    } else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
    {
        static_cast<void>(fprintf(stderr, "Vulkan Warning: %s\n", callbackData->pMessage));
    } else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
    {
        static_cast<void>(fprintf(stderr, "Vulkan Info: %s\n", callbackData->pMessage));
    } else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
    {
        static_cast<void>(fprintf(stderr, "Vulkan Verbose: %s\n", callbackData->pMessage));
    } else {
        static_cast<void>(fprintf(stderr, "Vulkan Unknown: %s\n", callbackData->pMessage));
    }
    return VK_FALSE;
}

static const VkDebugUtilsMessengerCreateInfoEXT debugUtilsMessengerInfo{
    VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
    nullptr,
    0,
    VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
    VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
    VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
    VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
    VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
    VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
    debugCallback,
    nullptr,
};

#endif

namespace vulkan {

VulkanState vk;

const char* vkResultToStr(VkResult result)
{
    switch (result)
    {
        case VK_SUCCESS:
            return "VK_SUCCESS";
        case VK_NOT_READY:
            return "VK_NOT_READY";
        case VK_TIMEOUT:
            return "VK_TIMEOUT";
        case VK_EVENT_SET:
            return "VK_EVENT_SET";
        case VK_EVENT_RESET:
            return "VK_EVENT_RESET";
        case VK_INCOMPLETE:
            return "VK_INCOMPLETE";
        case VK_ERROR_OUT_OF_HOST_MEMORY:
            return "VK_ERROR_OUT_OF_HOST_MEMORY";
        case VK_ERROR_OUT_OF_DEVICE_MEMORY:
            return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
        case VK_ERROR_INITIALIZATION_FAILED:
            return "VK_ERROR_INITIALIZATION_FAILED";
        case VK_ERROR_DEVICE_LOST:
            return "VK_ERROR_DEVICE_LOST";
        case VK_ERROR_MEMORY_MAP_FAILED:
            return "VK_ERROR_MEMORY_MAP_FAILED";
        case VK_ERROR_LAYER_NOT_PRESENT:
            return "VK_ERROR_LAYER_NOT_PRESENT";
        case VK_ERROR_EXTENSION_NOT_PRESENT:
            return "VK_ERROR_EXTENSION_NOT_PRESENT";
        case VK_ERROR_FEATURE_NOT_PRESENT:
            return "VK_ERROR_FEATURE_NOT_PRESENT";
        case VK_ERROR_INCOMPATIBLE_DRIVER:
            return "VK_ERROR_INCOMPATIBLE_DRIVER";
        case VK_ERROR_TOO_MANY_OBJECTS:
            return "VK_ERROR_TOO_MANY_OBJECTS";
        case VK_ERROR_FORMAT_NOT_SUPPORTED:
            return "VK_ERROR_FORMAT_NOT_SUPPORTED";
        case VK_ERROR_FRAGMENTED_POOL:
            return "VK_ERROR_FRAGMENTED_POOL";
        case VK_ERROR_UNKNOWN:
            return "VK_ERROR_UNKNOWN";
        case VK_ERROR_VALIDATION_FAILED:
            return "VK_ERROR_VALIDATION_FAILED";
        case VK_ERROR_OUT_OF_POOL_MEMORY:
            return "VK_ERROR_OUT_OF_POOL_MEMORY";
        case VK_ERROR_INVALID_EXTERNAL_HANDLE:
            return "VK_ERROR_INVALID_EXTERNAL_HANDLE";
        case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS:
            return "VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS";
        case VK_ERROR_FRAGMENTATION:
            return "VK_ERROR_FRAGMENTATION";
        case VK_PIPELINE_COMPILE_REQUIRED:
            return "VK_PIPELINE_COMPILE_REQUIRED";
        case VK_ERROR_NOT_PERMITTED:
            return "VK_ERROR_NOT_PERMITTED";
        case VK_ERROR_SURFACE_LOST_KHR:
            return "VK_ERROR_SURFACE_LOST_KHR";
        case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
            return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
        case VK_SUBOPTIMAL_KHR:
            return "VK_SUBOPTIMAL_KHR";
        case VK_ERROR_OUT_OF_DATE_KHR:
            return "VK_ERROR_OUT_OF_DATE_KHR";
        case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
            return "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
        case VK_ERROR_INVALID_SHADER_NV:
            return "VK_ERROR_INVALID_SHADER_NV";
        case VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR:
            return "VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR";
        case VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR:
            return "VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR";
        case VK_ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR:
            return "VK_ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR";
        case VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR:
            return "VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR";
        case VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR:
            return "VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR";
        case VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR:
            return "VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR";
        case VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT:
            return "VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT";
        case VK_ERROR_PRESENT_TIMING_QUEUE_FULL_EXT:
            return "VK_ERROR_PRESENT_TIMING_QUEUE_FULL_EXT";
        case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT:
            return "VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT";
        case VK_THREAD_IDLE_KHR:
            return "VK_THREAD_IDLE_KHR";
        case VK_THREAD_DONE_KHR:
            return "VK_THREAD_DONE_KHR";
        case VK_OPERATION_DEFERRED_KHR:
            return "VK_OPERATION_DEFERRED_KHR";
        case VK_OPERATION_NOT_DEFERRED_KHR:
            return "VK_OPERATION_NOT_DEFERRED_KHR";
        case VK_ERROR_INVALID_VIDEO_STD_PARAMETERS_KHR:
            return "VK_ERROR_INVALID_VIDEO_STD_PARAMETERS_KHR";
        case VK_ERROR_COMPRESSION_EXHAUSTED_EXT:
            return "VK_ERROR_COMPRESSION_EXHAUSTED_EXT";
        case VK_INCOMPATIBLE_SHADER_BINARY_EXT:
            return "VK_INCOMPATIBLE_SHADER_BINARY_EXT";
        case VK_PIPELINE_BINARY_MISSING_KHR:
            return "VK_PIPELINE_BINARY_MISSING_KHR";
        case VK_ERROR_NOT_ENOUGH_SPACE_KHR:
            return "VK_ERROR_NOT_ENOUGH_SPACE_KHR";
        case VK_RESULT_MAX_ENUM:
            return "VK_RESULT_MAX_ENUM";
    }
    return "Unrecognized Vulkan result";
}

static u32 vkFormatToSize(VkFormat format)
{
    switch (format)
    {
        case VK_FORMAT_UNDEFINED:
            return 0;

        case VK_FORMAT_R8_UNORM:
        case VK_FORMAT_R8_SNORM:
        case VK_FORMAT_R8_USCALED:
        case VK_FORMAT_R8_SSCALED:
        case VK_FORMAT_R8_UINT:
        case VK_FORMAT_R8_SINT:
        case VK_FORMAT_R8_SRGB:
        case VK_FORMAT_A8_UNORM:
        case VK_FORMAT_R8_BOOL_ARM:
            return 1;

        case VK_FORMAT_R4G4_UNORM_PACK8: return 1;
        case VK_FORMAT_R4G4B4A4_UNORM_PACK16:
        case VK_FORMAT_B4G4R4A4_UNORM_PACK16:
        case VK_FORMAT_R5G6B5_UNORM_PACK16:
        case VK_FORMAT_B5G6R5_UNORM_PACK16:
        case VK_FORMAT_R5G5B5A1_UNORM_PACK16:
        case VK_FORMAT_B5G5R5A1_UNORM_PACK16:
        case VK_FORMAT_A1R5G5B5_UNORM_PACK16:
        case VK_FORMAT_A4R4G4B4_UNORM_PACK16:
        case VK_FORMAT_A4B4G4R4_UNORM_PACK16:
        case VK_FORMAT_A1B5G5R5_UNORM_PACK16:
            return 2;

        case VK_FORMAT_R16_UNORM:
        case VK_FORMAT_R16_SNORM:
        case VK_FORMAT_R16_USCALED:
        case VK_FORMAT_R16_SSCALED:
        case VK_FORMAT_R16_UINT:
        case VK_FORMAT_R16_SINT:
        case VK_FORMAT_R16_SFLOAT:
            return 2;

        case VK_FORMAT_R8G8_UNORM:
        case VK_FORMAT_R8G8_SNORM:
        case VK_FORMAT_R8G8_USCALED:
        case VK_FORMAT_R8G8_SSCALED:
        case VK_FORMAT_R8G8_UINT:
        case VK_FORMAT_R8G8_SINT:
        case VK_FORMAT_R8G8_SRGB:
            return 2;

        case VK_FORMAT_R8G8B8_UNORM:
        case VK_FORMAT_R8G8B8_SNORM:
        case VK_FORMAT_R8G8B8_USCALED:
        case VK_FORMAT_R8G8B8_SSCALED:
        case VK_FORMAT_R8G8B8_UINT:
        case VK_FORMAT_R8G8B8_SINT:
        case VK_FORMAT_R8G8B8_SRGB:
        case VK_FORMAT_B8G8R8_UNORM:
        case VK_FORMAT_B8G8R8_SNORM:
        case VK_FORMAT_B8G8R8_USCALED:
        case VK_FORMAT_B8G8R8_SSCALED:
        case VK_FORMAT_B8G8R8_UINT:
        case VK_FORMAT_B8G8R8_SINT:
        case VK_FORMAT_B8G8R8_SRGB:
            return 3;

        case VK_FORMAT_R8G8B8A8_UNORM:
        case VK_FORMAT_R8G8B8A8_SNORM:
        case VK_FORMAT_R8G8B8A8_USCALED:
        case VK_FORMAT_R8G8B8A8_SSCALED:
        case VK_FORMAT_R8G8B8A8_UINT:
        case VK_FORMAT_R8G8B8A8_SINT:
        case VK_FORMAT_R8G8B8A8_SRGB:
        case VK_FORMAT_B8G8R8A8_UNORM:
        case VK_FORMAT_B8G8R8A8_SNORM:
        case VK_FORMAT_B8G8R8A8_USCALED:
        case VK_FORMAT_B8G8R8A8_SSCALED:
        case VK_FORMAT_B8G8R8A8_UINT:
        case VK_FORMAT_B8G8R8A8_SINT:
        case VK_FORMAT_B8G8R8A8_SRGB:
        case VK_FORMAT_A8B8G8R8_UNORM_PACK32:
        case VK_FORMAT_A8B8G8R8_SNORM_PACK32:
        case VK_FORMAT_A8B8G8R8_USCALED_PACK32:
        case VK_FORMAT_A8B8G8R8_SSCALED_PACK32:
        case VK_FORMAT_A8B8G8R8_UINT_PACK32:
        case VK_FORMAT_A8B8G8R8_SINT_PACK32:
        case VK_FORMAT_A8B8G8R8_SRGB_PACK32:
        case VK_FORMAT_B10G11R11_UFLOAT_PACK32:
        case VK_FORMAT_E5B9G9R9_UFLOAT_PACK32:
            return 4;

        case VK_FORMAT_A2R10G10B10_UNORM_PACK32:
        case VK_FORMAT_A2R10G10B10_SNORM_PACK32:
        case VK_FORMAT_A2R10G10B10_USCALED_PACK32:
        case VK_FORMAT_A2R10G10B10_SSCALED_PACK32:
        case VK_FORMAT_A2R10G10B10_UINT_PACK32:
        case VK_FORMAT_A2R10G10B10_SINT_PACK32:
        case VK_FORMAT_A2B10G10R10_UNORM_PACK32:
        case VK_FORMAT_A2B10G10R10_SNORM_PACK32:
        case VK_FORMAT_A2B10G10R10_USCALED_PACK32:
        case VK_FORMAT_A2B10G10R10_SSCALED_PACK32:
        case VK_FORMAT_A2B10G10R10_UINT_PACK32:
        case VK_FORMAT_A2B10G10R10_SINT_PACK32:
            return 4;

        case VK_FORMAT_R16G16_UNORM:
        case VK_FORMAT_R16G16_SNORM:
        case VK_FORMAT_R16G16_USCALED:
        case VK_FORMAT_R16G16_SSCALED:
        case VK_FORMAT_R16G16_UINT:
        case VK_FORMAT_R16G16_SINT:
        case VK_FORMAT_R16G16_SFLOAT:
            return 4;

        case VK_FORMAT_R16G16B16_UNORM:
        case VK_FORMAT_R16G16B16_SNORM:
        case VK_FORMAT_R16G16B16_USCALED:
        case VK_FORMAT_R16G16B16_SSCALED:
        case VK_FORMAT_R16G16B16_UINT:
        case VK_FORMAT_R16G16B16_SINT:
        case VK_FORMAT_R16G16B16_SFLOAT:
            return 6;

        case VK_FORMAT_R16G16B16A16_UNORM:
        case VK_FORMAT_R16G16B16A16_SNORM:
        case VK_FORMAT_R16G16B16A16_USCALED:
        case VK_FORMAT_R16G16B16A16_SSCALED:
        case VK_FORMAT_R16G16B16A16_UINT:
        case VK_FORMAT_R16G16B16A16_SINT:
        case VK_FORMAT_R16G16B16A16_SFLOAT:
            return 8;

        case VK_FORMAT_R32_UINT:
        case VK_FORMAT_R32_SINT:
        case VK_FORMAT_R32_SFLOAT:
            return 4;

        case VK_FORMAT_R32G32_UINT:
        case VK_FORMAT_R32G32_SINT:
        case VK_FORMAT_R32G32_SFLOAT:
            return 8;

        case VK_FORMAT_R32G32B32_UINT:
        case VK_FORMAT_R32G32B32_SINT:
        case VK_FORMAT_R32G32B32_SFLOAT:
            return 12;

        case VK_FORMAT_R32G32B32A32_UINT:
        case VK_FORMAT_R32G32B32A32_SINT:
        case VK_FORMAT_R32G32B32A32_SFLOAT:
            return 16;

        case VK_FORMAT_R64_UINT:
        case VK_FORMAT_R64_SINT:
        case VK_FORMAT_R64_SFLOAT:
            return 8;

        case VK_FORMAT_R64G64_UINT:
        case VK_FORMAT_R64G64_SINT:
        case VK_FORMAT_R64G64_SFLOAT:
            return 16;

        case VK_FORMAT_R64G64B64_UINT:
        case VK_FORMAT_R64G64B64_SINT:
        case VK_FORMAT_R64G64B64_SFLOAT:
            return 24;

        case VK_FORMAT_R64G64B64A64_UINT:
        case VK_FORMAT_R64G64B64A64_SINT:
        case VK_FORMAT_R64G64B64A64_SFLOAT:
            return 32;

        case VK_FORMAT_D16_UNORM: return 2;
        case VK_FORMAT_X8_D24_UNORM_PACK32: return 4;
        case VK_FORMAT_D32_SFLOAT: return 4;
        case VK_FORMAT_S8_UINT: return 1;
        case VK_FORMAT_D16_UNORM_S8_UINT: return 3;
        case VK_FORMAT_D24_UNORM_S8_UINT: return 4;
        case VK_FORMAT_D32_SFLOAT_S8_UINT: return 5;

        case VK_FORMAT_BC1_RGB_UNORM_BLOCK:
        case VK_FORMAT_BC1_RGB_SRGB_BLOCK:
        case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:
        case VK_FORMAT_BC1_RGBA_SRGB_BLOCK:
            return 8;

        case VK_FORMAT_BC2_UNORM_BLOCK:
        case VK_FORMAT_BC2_SRGB_BLOCK:
        case VK_FORMAT_BC3_UNORM_BLOCK:
        case VK_FORMAT_BC3_SRGB_BLOCK:
            return 16;

        case VK_FORMAT_BC4_UNORM_BLOCK:
        case VK_FORMAT_BC4_SNORM_BLOCK:
        case VK_FORMAT_BC5_UNORM_BLOCK:
        case VK_FORMAT_BC5_SNORM_BLOCK:
            return 16;

        case VK_FORMAT_BC6H_UFLOAT_BLOCK:
        case VK_FORMAT_BC6H_SFLOAT_BLOCK:
        case VK_FORMAT_BC7_UNORM_BLOCK:
        case VK_FORMAT_BC7_SRGB_BLOCK:
            return 16;

        case VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK:
        case VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK:
            return 8;

        case VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK:
        case VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK:
            return 8;

        case VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK:
        case VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK:
            return 16;

        case VK_FORMAT_EAC_R11_UNORM_BLOCK:
        case VK_FORMAT_EAC_R11_SNORM_BLOCK:
            return 8;

        case VK_FORMAT_EAC_R11G11_UNORM_BLOCK:
        case VK_FORMAT_EAC_R11G11_SNORM_BLOCK:
            return 16;

        case VK_FORMAT_ASTC_4x4_UNORM_BLOCK:
        case VK_FORMAT_ASTC_4x4_SRGB_BLOCK:
        case VK_FORMAT_ASTC_5x4_UNORM_BLOCK:
        case VK_FORMAT_ASTC_5x4_SRGB_BLOCK:
        case VK_FORMAT_ASTC_5x5_UNORM_BLOCK:
        case VK_FORMAT_ASTC_5x5_SRGB_BLOCK:
        case VK_FORMAT_ASTC_6x5_UNORM_BLOCK:
        case VK_FORMAT_ASTC_6x5_SRGB_BLOCK:
        case VK_FORMAT_ASTC_6x6_UNORM_BLOCK:
        case VK_FORMAT_ASTC_6x6_SRGB_BLOCK:
        case VK_FORMAT_ASTC_8x5_UNORM_BLOCK:
        case VK_FORMAT_ASTC_8x5_SRGB_BLOCK:
        case VK_FORMAT_ASTC_8x6_UNORM_BLOCK:
        case VK_FORMAT_ASTC_8x6_SRGB_BLOCK:
        case VK_FORMAT_ASTC_8x8_UNORM_BLOCK:
        case VK_FORMAT_ASTC_8x8_SRGB_BLOCK:
        case VK_FORMAT_ASTC_10x5_UNORM_BLOCK:
        case VK_FORMAT_ASTC_10x5_SRGB_BLOCK:
        case VK_FORMAT_ASTC_10x6_UNORM_BLOCK:
        case VK_FORMAT_ASTC_10x6_SRGB_BLOCK:
        case VK_FORMAT_ASTC_10x8_UNORM_BLOCK:
        case VK_FORMAT_ASTC_10x8_SRGB_BLOCK:
        case VK_FORMAT_ASTC_10x10_UNORM_BLOCK:
        case VK_FORMAT_ASTC_10x10_SRGB_BLOCK:
        case VK_FORMAT_ASTC_12x10_UNORM_BLOCK:
        case VK_FORMAT_ASTC_12x10_SRGB_BLOCK:
        case VK_FORMAT_ASTC_12x12_UNORM_BLOCK:
        case VK_FORMAT_ASTC_12x12_SRGB_BLOCK:
            return 16;

        case VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG:
        case VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG:
            return 8;
        case VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG:
        case VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG:
            return 8;
        case VK_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG:
        case VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG:
            return 8;
        case VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG:
        case VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG:
            return 8;

        case VK_FORMAT_G8B8G8R8_422_UNORM:
        case VK_FORMAT_B8G8R8G8_422_UNORM:
        case VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM:
        case VK_FORMAT_G8_B8R8_2PLANE_420_UNORM:
        case VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM:
        case VK_FORMAT_G8_B8R8_2PLANE_422_UNORM:
        case VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM:
        case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16:
        case VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16:
        case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16:
        case VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16:
        case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16:
        case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16:
        case VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16:
        case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16:
        case VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16:
        case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16:
        case VK_FORMAT_G16B16G16R16_422_UNORM:
        case VK_FORMAT_B16G16R16G16_422_UNORM:
        case VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM:
        case VK_FORMAT_G16_B16R16_2PLANE_420_UNORM:
        case VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM:
        case VK_FORMAT_G16_B16R16_2PLANE_422_UNORM:
        case VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM:
            return 0;

        default:
            HG_WARN("Unrecognized Vulkan format\n");
            return 0;
    }
}

} // namespace vulkan

u32 formatToSize(Format format)
{
    return vkFormatToSize(formatToVk(format));
}

namespace vulkan {

static VkInstance createInstance(Span<StringView> extensions)
{
    if (extensions.count > 0)
        HG_ASSERT(extensions.data != nullptr);

    ArenaScope scratch = getScratch();

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Hurdy Gurdy Application",
    appInfo.applicationVersion = 0;
    appInfo.pEngineName = "Hurdy Gurdy Engine";
    appInfo.engineVersion = 0;
    appInfo.apiVersion = VK_API_VERSION_1_3;

    VkInstanceCreateInfo instanceInfo{};
    instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
#ifdef HG_VK_DEBUG_MESSENGER
    instanceInfo.pNext = &debugUtilsMessengerInfo;
#endif
    instanceInfo.flags = 0;
    instanceInfo.pApplicationInfo = &appInfo;

#ifdef HG_VK_DEBUG_MESSENGER
    const char* layers[]{
        "VK_LAYER_KHRONOS_validation",
    };
    instanceInfo.enabledLayerCount = static_cast<u32>(size(layers));
    instanceInfo.ppEnabledLayerNames = layers;
#endif

    const char** extCStrs = scratch.alloc<const char*>(extensions.count);
    for (u32 i = 0; i < extensions.count; ++i)
    {
        extCStrs[i] = cString(scratch, extensions[i]);
    }
    instanceInfo.enabledExtensionCount = (u32)extensions.count;
    instanceInfo.ppEnabledExtensionNames = extCStrs;

    VkInstance instance = nullptr;
    VkResult result = vkCreateInstance(&instanceInfo, nullptr, &instance);
    if (instance == nullptr)
    {
        setError("Failed to create Vulkan instance: %s", vkResultToStr(result));
    }

    return instance;
}

#ifdef HG_VK_DEBUG_MESSENGER
static VkDebugUtilsMessengerEXT createDebugUtilsMessenger()
{
    HG_ASSERT(vk.instance != nullptr);

    VkDebugUtilsMessengerEXT messenger = nullptr;
    VkResult result = vkCreateDebugUtilsMessengerEXT(vk.instance, &debugUtilsMessengerInfo, nullptr, &messenger);
    if (messenger == nullptr)
    {
        setError("Failed to create Vulkan debug messenger: %s", vkResultToStr(result));
    }

    return messenger;
}
#endif

static bool findQueueFamily(VkPhysicalDevice gpu, u32* queueFamily, VkQueueFlags queueFlags)
{
    HG_ASSERT(gpu != nullptr);
    HG_ASSERT(queueFamily != nullptr);

    ArenaScope scratch = getScratch();

    u32 familyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(gpu, &familyCount, nullptr);
    VkQueueFamilyProperties* families = scratch.alloc<VkQueueFamilyProperties>(familyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(gpu, &familyCount, families);

    for (u32 i = 0; i < familyCount; ++i)
    {
        if (families[i].queueFlags & queueFlags)
        {
            *queueFamily = i;
            return true;
        }
    }

    setError("Could not find Vulkan queue family");
    *queueFamily = (u32)-1;
    return false;
}

static VkPhysicalDevice findPhysicalDevice()
{
    HG_ASSERT(vk.instance != nullptr);

    ArenaScope scratch = getScratch();

    u32 gpuCount;
    vkEnumeratePhysicalDevices(vk.instance, &gpuCount, nullptr);
    VkPhysicalDevice* gpus = scratch.alloc<VkPhysicalDevice>(gpuCount);
    vkEnumeratePhysicalDevices(vk.instance, &gpuCount, gpus);

    ArrayTemp<VkExtensionProperties> extProps{scratch, 0, 0};

    for (u32 i = 0; i < gpuCount; ++i)
    {
        VkPhysicalDevice gpu = gpus[i];
        u32 family;

        u32 propCount = 0;
        vkEnumerateDeviceExtensionProperties(gpu, nullptr, &propCount, nullptr);
        extProps.resize(propCount);
        vkEnumerateDeviceExtensionProperties(gpu, nullptr, &propCount, extProps.vals);

        for (u32 j = 0; j < static_cast<u32>(size(deviceExtensions)); j++)
        {
            for (u32 k = 0; k < propCount; k++)
            {
                if (strcmp(deviceExtensions[j], extProps[k].extensionName) == 0)
                    goto nextExt;
            }
            goto nextGpu;
nextExt:
            continue;
        }

        if (!findQueueFamily(gpu, &family,
                VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_TRANSFER_BIT | VK_QUEUE_COMPUTE_BIT))
            goto nextGpu;

        return gpu;

nextGpu:
        continue;
    }

    setError("Could not find suitable gpu");
    return nullptr;
}

static VkDevice createDevice()
{
    HG_ASSERT(vk.physicalDevice != nullptr);
    HG_ASSERT(vk.queueFamily != (u32)-1);

    VkPhysicalDeviceDescriptorIndexingFeatures descriptorIndexingFeature{};
    descriptorIndexingFeature.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
    descriptorIndexingFeature.pNext = nullptr;
    descriptorIndexingFeature.shaderInputAttachmentArrayDynamicIndexing = VK_TRUE;
    descriptorIndexingFeature.shaderUniformTexelBufferArrayDynamicIndexing = VK_TRUE;
    descriptorIndexingFeature.shaderStorageTexelBufferArrayDynamicIndexing = VK_TRUE;
    descriptorIndexingFeature.shaderUniformBufferArrayNonUniformIndexing = VK_TRUE;
    descriptorIndexingFeature.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
    descriptorIndexingFeature.shaderStorageBufferArrayNonUniformIndexing = VK_TRUE;
    descriptorIndexingFeature.shaderStorageImageArrayNonUniformIndexing = VK_TRUE;
    descriptorIndexingFeature.shaderInputAttachmentArrayNonUniformIndexing = VK_TRUE;
    descriptorIndexingFeature.shaderUniformTexelBufferArrayNonUniformIndexing = VK_TRUE;
    descriptorIndexingFeature.shaderStorageTexelBufferArrayNonUniformIndexing = VK_TRUE;
    descriptorIndexingFeature.descriptorBindingUniformBufferUpdateAfterBind = VK_TRUE;
    descriptorIndexingFeature.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE;
    descriptorIndexingFeature.descriptorBindingStorageImageUpdateAfterBind = VK_TRUE;
    descriptorIndexingFeature.descriptorBindingStorageBufferUpdateAfterBind = VK_TRUE;
    descriptorIndexingFeature.descriptorBindingUniformTexelBufferUpdateAfterBind = VK_TRUE;
    descriptorIndexingFeature.descriptorBindingStorageTexelBufferUpdateAfterBind = VK_TRUE;
    descriptorIndexingFeature.descriptorBindingUpdateUnusedWhilePending = VK_TRUE;
    descriptorIndexingFeature.descriptorBindingPartiallyBound = VK_TRUE;
    descriptorIndexingFeature.descriptorBindingVariableDescriptorCount = VK_TRUE;
    descriptorIndexingFeature.runtimeDescriptorArray = VK_TRUE;

    VkPhysicalDeviceDynamicRenderingFeatures dynamicRenderingFeature{};
    dynamicRenderingFeature.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES;
    dynamicRenderingFeature.pNext = &descriptorIndexingFeature;
    dynamicRenderingFeature.dynamicRendering = VK_TRUE;

    VkPhysicalDeviceSynchronization2Features synchronization2Feature{};
    synchronization2Feature.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES;
    synchronization2Feature.pNext = &dynamicRenderingFeature;
    synchronization2Feature.synchronization2 = VK_TRUE;

    VkPhysicalDeviceFeatures features{};

    VkDeviceQueueCreateInfo queueInfo{};
    queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueInfo.queueFamilyIndex = vk.queueFamily;
    queueInfo.queueCount = 1;
    f32 queuePriority = 1.0f;
    queueInfo.pQueuePriorities = &queuePriority;

    VkDeviceCreateInfo deviceInfo{};
    deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceInfo.pNext = &synchronization2Feature;
    deviceInfo.queueCreateInfoCount = 1;
    deviceInfo.pQueueCreateInfos = &queueInfo;
    deviceInfo.enabledExtensionCount = static_cast<u32>(size(deviceExtensions));
    deviceInfo.ppEnabledExtensionNames = deviceExtensions;
    deviceInfo.pEnabledFeatures = &features;

    VkDevice device = nullptr;
    VkResult result = vkCreateDevice(vk.physicalDevice, &deviceInfo, nullptr, &device);
    if (device == nullptr)
    {
        setError("Could not create VkDevice: %s", vkResultToStr(result));
    }

    return device;
}

static VmaAllocator createVma()
{
    VmaAllocatorCreateInfo allocatorInfo{};
    allocatorInfo.physicalDevice = vk.physicalDevice;
    allocatorInfo.device = vk.device;
    allocatorInfo.instance = vk.instance;
    allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_3;

    VmaAllocator vma = nullptr;
    VkResult result = vmaCreateAllocator(&allocatorInfo, &vma);
    if (vma == nullptr)
    {
        setError("Could not create Vulkan memory allocator: %s", vkResultToStr(result));
    }

    return vma;
}

static VkDescriptorPool createBindlessDescriptorPool()
{
    VkDescriptorPoolSize sizes[]{
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, UINT16_MAX},
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, UINT16_MAX},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, UINT16_MAX},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, UINT16_MAX},
    };

    VkDescriptorPoolCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    info.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
    info.maxSets = 1;
    info.poolSizeCount = static_cast<u32>(size(sizes));
    info.pPoolSizes = sizes;

    VkDescriptorPool pool = nullptr;
    [[maybe_unused]] VkResult result = vkCreateDescriptorPool(vk.device, &info, nullptr, &pool);
    if (pool == nullptr)
        HG_PANIC("Could not create VkDescriptorPool: %s\n", vkResultToStr(result));

    return pool;
}

static VkDescriptorSetLayout createBindlessDescriptorLayout()
{
    VkDescriptorSetLayoutBinding bindings[DescriptorType_count]{};
    VkDescriptorBindingFlags flags[DescriptorType_count]{};
    for (u32 i = 0; i < DescriptorType_count; ++i)
    {
        bindings[i].binding = i;
        bindings[i].descriptorType = descriptorTypeToVk(static_cast<DescriptorType>(i));
        bindings[i].descriptorCount = UINT16_MAX;
        bindings[i].stageFlags = VK_SHADER_STAGE_ALL;
        flags[i] = VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT
                 | VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;
    }

    VkDescriptorSetLayoutBindingFlagsCreateInfo flagsInfo{};
    flagsInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
    flagsInfo.bindingCount = static_cast<u32>(size(bindings));
    flagsInfo.pBindingFlags = flags;

    VkDescriptorSetLayoutCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    info.pNext = &flagsInfo;
    info.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
    info.bindingCount = static_cast<u32>(size(bindings));
    info.pBindings = bindings;

    VkDescriptorSetLayout layout = nullptr;
    [[maybe_unused]] VkResult result = vkCreateDescriptorSetLayout(vk.device, &info, nullptr, &layout);
    if (layout == nullptr)
        HG_PANIC("Could not create bindless VkDescriptorSetLayout: %s\n", vkResultToStr(result));

    return layout;
}

static Frame createFrame()
{
    Frame frame{};

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = vk.queueFamily;

    [[maybe_unused]]
    VkResult poolResult = vkCreateCommandPool(vk.device, &poolInfo, nullptr, &frame.cmdPool);
    if (frame.cmdPool == nullptr)
        HG_PANIC("Could not create Vulkan command pool: %s\n", vkResultToStr(poolResult));

    VkCommandBufferAllocateInfo cmdInfo{};
    cmdInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdInfo.commandPool = frame.cmdPool;
    cmdInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdInfo.commandBufferCount = 1;

    [[maybe_unused]]
    VkResult cmdResult = vkAllocateCommandBuffers(vk.device, &cmdInfo, &frame.cmd);
    if (frame.cmd == nullptr)
        HG_PANIC("Could not create Vulkan command buffer: %s\n", vkResultToStr(cmdResult));

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    [[maybe_unused]]
    VkResult fenceResult = vkCreateFence(vk.device, &fenceInfo, nullptr, &frame.fence);
    if (frame.fence == nullptr)
        HG_PANIC("Could not create Vulkan fence: %s\n", vkResultToStr(fenceResult));

    return frame;
}

GpuDescriptor createBufferDescriptor(
    DescriptorType type,
    const GpuBuffer& buffer,
    u64 offset,
    u64 range)
{
    HG_ASSERT(type < DescriptorType_count);

    ArenaScope scratch = getScratch();

    GpuDescriptor desc = vk.descriptorPools[type].alloc();

    VkDescriptorBufferInfo bufferInfo{buffer.data->buffer, offset, range};

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = vk.bindlessSet;
    write.dstBinding = type;
    write.dstArrayElement = desc.idx();
    write.descriptorCount = 1;
    write.descriptorType = descriptorTypeToVk(type);
    write.pBufferInfo = &bufferInfo;
    write.pImageInfo = nullptr;
    write.pTexelBufferView = nullptr;

    vkUpdateDescriptorSets(vk.device, 1, &write, 0, nullptr);

    return desc;
}

GpuDescriptor createImageDescriptor(
    DescriptorType type,
    const GpuView& imageView,
    GpuLayout imageLayout)
{
    HG_ASSERT(type < DescriptorType_count);

    ArenaScope scratch = getScratch();

    GpuDescriptor desc = vk.descriptorPools[type].alloc();

    VkDescriptorImageInfo imageInfo{
        imageView.data->sampler,
        imageView.data->view,
        gpuLayoutToVk(imageLayout)
    };

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = vk.bindlessSet;
    write.dstBinding = type;
    write.dstArrayElement = desc.idx();
    write.descriptorCount = 1;
    write.descriptorType = descriptorTypeToVk(type);
    write.pBufferInfo = nullptr;
    write.pImageInfo = &imageInfo;
    write.pTexelBufferView = nullptr;

    vkUpdateDescriptorSets(vk.device, 1, &write, 0, nullptr);

    return desc;
}

void descriptorDestroy(GpuDescriptor desc, DescriptorType type)
{
    if (desc != nullHandle)
        vk.descriptorPools[type].free(desc);
}

static VkSampler samplerCreate(SamplerInfo* desc)
{
    VkSamplerCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    info.magFilter = gpuFilterToVk(desc->filter);
    info.minFilter = gpuFilterToVk(desc->filter);
    info.mipmapMode = desc->filter == GpuFilter_linear
        ? VK_SAMPLER_MIPMAP_MODE_LINEAR
        : VK_SAMPLER_MIPMAP_MODE_NEAREST;
    info.addressModeU = gpuSamplerAddressModeToVk(desc->mode);
    info.addressModeV = gpuSamplerAddressModeToVk(desc->mode);
    info.addressModeW = gpuSamplerAddressModeToVk(desc->mode);
    info.mipLodBias = 0.0f;
    info.minLod = 0.0f;
    info.maxLod = 1000.0f;
    info.borderColor = gpuSamplerBorderToVk(desc->border);

    VkSampler sampler = nullptr;
    [[maybe_unused]]     VkResult result = vkCreateSampler(vk.device, &info, nullptr, &sampler);
    if (sampler == nullptr)
        HG_PANIC("Could not create VkSampler: %s\n", vkResultToStr(result));

    return sampler;
}

VkSampler samplerGet(
    GpuFilter filter,
    GpuSamplerEdgeMode addressMode,
    GpuSamplerBorder borderColor)
{
    SamplerInfo desc = {filter, addressMode, borderColor};
    VkSampler* sampler = vk.samplers.get(desc);
    if (sampler == nullptr)
    {
        sampler = vk.samplers.add(desc, samplerCreate(&desc));
    }
    return *sampler;
}

} // namespace vulkan

namespace internal {

void* getVulkanInstance()
{
    return vk.instance;
}

bool initGpu()
{
    ArenaScope scratch = getScratch();

    if (!loadVulkan())
        goto loadFailed;

    {
        Span<StringView> exts = getPlatformVulkanExtensions(scratch);
#ifdef HG_VK_DEBUG_MESSENGER
        [[maybe_unused]]
        bool extended = scratch.extend(exts.data, exts.count, exts.count + 1);
        HG_ASSERT(extended);
        ++exts.count;
        exts[exts.count - 1] = "VK_EXT_debug_utils";
#endif
        vk.instance = createInstance(exts);
        if (vk.instance == nullptr)
            goto instanceFailed;
    }
    if (!loadVulkanInstanceFuncs(vk.instance))
        goto loadInstanceFailed;

#ifdef HG_VK_DEBUG_MESSENGER
    vk.debugMessenger = createDebugUtilsMessenger();
    if (vk.debugMessenger == nullptr)
        goto debugMessengerFailed;
#endif

    vk.physicalDevice = findPhysicalDevice();
    if (vk.physicalDevice == nullptr)
        goto physicalDeviceFailed;

    findQueueFamily(vk.physicalDevice, &vk.queueFamily,
        VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_TRANSFER_BIT | VK_QUEUE_COMPUTE_BIT);
    if (vk.queueFamily == (u32)-1)
        goto queueFamilyFailed;

    vk.device = createDevice();
    if (vk.device == nullptr)
        goto deviceFailed;

    if (!loadVulkanDeviceFuncs(vk.device))
        goto loadDeviceFailed;

    vkGetDeviceQueue(vk.device, vk.queueFamily, 0, &vk.queue);
    if (vk.queue == nullptr)
        goto queueFailed;

    vk.vma = createVma();
    if (vk.vma == nullptr)
        goto vmaFailed;

    {
        VkCommandPoolCreateInfo cmdPoolInfo{};
        cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        cmdPoolInfo.queueFamilyIndex = vk.queueFamily;

        [[maybe_unused]]         VkResult result = vkCreateCommandPool(vk.device, &cmdPoolInfo, nullptr, &vk.cmdPool);
        if (vk.cmdPool == nullptr)
            HG_PANIC("Could not create Vulkan command pool: %s\n", vkResultToStr(result));
    }

    vk.bindlessPool = createBindlessDescriptorPool();
    vk.bindlessLayout = createBindlessDescriptorLayout();
    {
        VkDescriptorSetAllocateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        info.descriptorPool = vk.bindlessPool;
        info.descriptorSetCount = 1;
        info.pSetLayouts = &vk.bindlessLayout;

        [[maybe_unused]]         VkResult result = vkAllocateDescriptorSets(vk.device, &info, &vk.bindlessSet);
        if (vk.bindlessSet == nullptr)
            HG_PANIC("Could not allocate bindless VkDescriptorSet: %s\n", vkResultToStr(result));
    }

    for (u32 i = 0; i < DescriptorType_count; ++i)
    {
        vk.descriptorPools[i] = HandlePool{};
    }

    vk.samplers = Map<SamplerInfo, VkSampler>(
        2 *
        GpuFilter_count *
        GpuSamplerEdgeMode_count *
        GpuSamplerBorder_count);

    vk.frameCount = 2;
    vk.currentFrame = 0;
    vk.frames = heapAlloc<Frame>(vk.frameCount);
    for (u32 i = 0; i < vk.frameCount; ++i)
    {
        new (vk.frames + i) Frame{createFrame()};
    }

    return true;

vmaFailed:
queueFailed:
    vkDestroyDevice(vk.device, nullptr);
loadDeviceFailed:
deviceFailed:
queueFamilyFailed:
physicalDeviceFailed:
#ifdef HG_VK_DEBUG_MESSENGER
    vkDestroyDebugUtilsMessengerEXT(vk.instance, vk.debugMessenger, nullptr);
debugMessengerFailed:
#endif
loadInstanceFailed:
    vkDestroyInstance(vk.instance, nullptr);
instanceFailed:
    unloadVulkan();
loadFailed:
    return false;
}

void deinitGpu()
{
    for (u32 i = 0; i < vk.frameCount; ++i)
    {
        vkDestroyFence(vk.device, vk.frames[i].fence, nullptr);
        vkDestroyCommandPool(vk.device, vk.frames[i].cmdPool, nullptr);
        vk.frames[i].swapchains = {};
    }
    heapFree(vk.frames, vk.frameCount);

    vk.samplers.forEach([](const SamplerInfo&, VkSampler& sampler)
    {
        vkDestroySampler(vk.device, sampler, nullptr);
    });
    vk.samplers = {};

    for (u32 i = 0; i < DescriptorType_count; ++i)
    {
        vk.descriptorPools[i].reset();
    }

    vkDestroyDescriptorSetLayout(vk.device, vk.bindlessLayout, nullptr);
    vkDestroyDescriptorPool(vk.device, vk.bindlessPool, nullptr);

    vkDestroyCommandPool(vk.device, vk.cmdPool, nullptr);

    vmaDestroyAllocator(vk.vma);

    vkDestroyDevice(vk.device, nullptr);

#ifdef HG_VK_DEBUG_MESSENGER
    vkDestroyDebugUtilsMessengerEXT(vk.instance, vk.debugMessenger, nullptr);
#endif

    vkDestroyInstance(vk.instance, nullptr);

    unloadVulkan();
}

Swapchain::Swapchain() noexcept
    : data{nullptr}
{}

Swapchain::~Swapchain() noexcept = default;
Swapchain::Swapchain(Swapchain&& other) noexcept = default;
Swapchain& Swapchain::operator=(Swapchain&& other) noexcept = default;

u32 Swapchain::width() const
{
    return data ? data->width : 0;
}

u32 Swapchain::height() const
{
    return data ? data->height : 0;
}

Format Swapchain::format() const
{
    return data ? data->format : Format_undefined;
}

GpuView* Swapchain::currentView() const
{
    return (data && data->imageIdx < data->images.count)
        ? &data->views[data->imageIdx]
        : nullptr;
}

u32 Swapchain::imageCount() const
{
    return data ? static_cast<u32>(data->images.count) : 0;
}

SwapchainData::~SwapchainData()
{
    for (GpuImage& image : images)
    {
        image.data->image = nullptr;
    }
    for (VkSemaphore semaphore : readyToPresent)
    {
        vkDestroySemaphore(vk.device, semaphore, nullptr);
    }
    for (VkSemaphore semaphore : imageAvailable)
    {
        vkDestroySemaphore(vk.device, semaphore, nullptr);
    }
    vkDestroySwapchainKHR(vk.device, swapchain, nullptr);
    if (surface != nullptr)
        vkDestroySurfaceKHR(vk.instance, surface, nullptr);
}

static Format findSwapchainFormat(VkSurfaceKHR surface)
{
    HG_ASSERT(surface != nullptr);

    ArenaScope scratch = getScratch();

    u32 formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(
        vk.physicalDevice, surface, &formatCount, nullptr);
    VkSurfaceFormatKHR* formats = scratch.alloc<VkSurfaceFormatKHR>(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(
        vk.physicalDevice, surface, &formatCount, formats);

    for (u32 i = 0; i < formatCount; ++i)
    {
        if (formats[i].format == VK_FORMAT_R8G8B8A8_SRGB)
            return Format_r8g8b8a8_srgb;
        if (formats[i].format == VK_FORMAT_B8G8R8A8_SRGB)
            return Format_b8g8r8a8_srgb;
    }
    HG_PANIC("No supported swapchain formats\n");
}

static GpuPresentMode findSwapchainPresentMode(
    VkSurfaceKHR surface,
    GpuPresentMode desiredMode)
{
    HG_ASSERT(surface != nullptr);

    ArenaScope scratch = getScratch();

    if (desiredMode == GpuPresentMode_fifo)
        return desiredMode;

    u32 modeCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(
        vk.physicalDevice, surface, &modeCount, nullptr);
    VkPresentModeKHR* presentModes = scratch.alloc<VkPresentModeKHR>(modeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(
        vk.physicalDevice, surface, &modeCount, presentModes);

    for (u32 i = 0; i < modeCount; ++i)
    {
        if (presentModes[i] == presentModeToVk(desiredMode))
            return desiredMode;
    }
    return GpuPresentMode_fifo;
}

void Swapchain::resize(u32 newWidth, u32 newHeight)
{
    HG_ASSERT(data != nullptr);

    ArenaScope scratch = getScratch();

    vkQueueWaitIdle(vk.queue);

    for (u32 i = 0; i < data->images.count; ++i)
    {
        if (data->views[i].data != nullptr)
            vkDestroyImageView(vk.device, data->views[i].data->view, nullptr);

        if (data->readyToPresent[i] != nullptr)
        {
            vkDestroySemaphore(vk.device, data->readyToPresent[i], nullptr);
            data->readyToPresent[i] = nullptr;
        }
    }

    for (u32 i = 0; i < vk.frameCount; ++i)
    {
        if (data->imageAvailable[i] != nullptr)
        {
            vkDestroySemaphore(vk.device, data->imageAvailable[i], nullptr);
            data->imageAvailable[i] = nullptr;
        }
    }

    VkSwapchainKHR oldSwapchain = data->swapchain;

    data->width = newWidth;
    data->height = newHeight;

    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
        vk.physicalDevice, data->surface, &capabilities);

    if (capabilities.currentExtent.width != (u32)-1)
        data->width = capabilities.currentExtent.width;
    if (capabilities.currentExtent.height != (u32)-1)
        data->height = capabilities.currentExtent.height;

    if (data->width != 0 && data->height != 0)
    {
        VkSwapchainCreateInfoKHR swapchainInfo{};
        swapchainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        swapchainInfo.surface = data->surface;
        swapchainInfo.minImageCount =
            std::min(capabilities.minImageCount, capabilities.maxImageCount - 1) + 1;
        swapchainInfo.imageFormat = formatToVk(data->format);
        swapchainInfo.imageExtent = {data->width, data->height};
        swapchainInfo.imageArrayLayers = 1;
        swapchainInfo.imageUsage = data->imageUsage;
        swapchainInfo.preTransform = capabilities.currentTransform;
        swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        swapchainInfo.presentMode = presentModeToVk(data->presentMode);
        swapchainInfo.clipped = VK_TRUE;
        swapchainInfo.oldSwapchain = oldSwapchain;

        [[maybe_unused]]
        VkResult result = vkCreateSwapchainKHR(
            vk.device, &swapchainInfo, nullptr, &data->swapchain);
        if (data->swapchain == nullptr)
            HG_PANIC("Failed to create swapchain: %s\n",
                vkResultToStr(result));

        u32 swapImageCount;
        vkGetSwapchainImagesKHR(
            vk.device, data->swapchain, &swapImageCount, nullptr);

        if (data->images.count != swapImageCount)
        {
            data->images.resize(swapImageCount);
            data->views.resize(swapImageCount);
            data->readyToPresent.resize(swapImageCount);
        }

        VkImage* swapImages = scratch.alloc<VkImage>(swapImageCount);
        vkGetSwapchainImagesKHR(
            vk.device, data->swapchain, &swapImageCount, swapImages);

        for (u32 i = 0; i < data->images.count; ++i)
        {
            if (data->images[i].data == nullptr)
                data->images[i].data = makeUnique<GpuImageData>();
            data->images[i].data->image = swapImages[i];
            data->images[i].data->dimensions = 2;
            data->images[i].data->format = data->format;
            data->images[i].data->width = data->width;
            data->images[i].data->height = data->height;
            data->images[i].data->depth = 1;
            data->images[i].data->mipLevels = 1;
            data->images[i].data->arrayLayers = 1;
            data->images[i].data->msaaSamples = 1;

            VkImageViewCreateInfo viewInfo{};
            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image = swapImages[i];
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.format = formatToVk(data->format);
            viewInfo.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

            if (data->views[i].data == nullptr)
                data->views[i].data = makeUnique<GpuViewData>();

            [[maybe_unused]]
            VkResult viewResult = vkCreateImageView(
                vk.device, &viewInfo, nullptr, &data->views[i].data->view);
            if (data->views[i].data->view == nullptr)
                HG_PANIC("Could not create VkImageView: %s\n",
                    vkResultToStr(viewResult));

            data->views[i].data->image = data->images[i].data;
            data->views[i].data->type = GpuViewType_2D;
            data->views[i].data->aspectFlags = GpuAspect_color;
            data->views[i].data->baseMipLevel = 0;
            data->views[i].data->levelCount = 1;
            data->views[i].data->baseArrayLayer = 0;
            data->views[i].data->layerCount = 1;

            VkSemaphoreCreateInfo semaphoreInfo{};
            semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

            [[maybe_unused]]
            VkResult readyResult = vkCreateSemaphore(
                vk.device, &semaphoreInfo, nullptr, &data->readyToPresent[i]);
            if (data->readyToPresent[i] == nullptr)
                HG_PANIC("Could not create VkSemaphore: %s\n",
                    vkResultToStr(readyResult));
        }

        for (u32 i = 0; i < vk.frameCount; ++i)
        {
            VkSemaphoreCreateInfo semaphoreInfo{};
            semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

            [[maybe_unused]]
            VkResult availableResult = vkCreateSemaphore(
                vk.device, &semaphoreInfo, nullptr, &data->imageAvailable[i]);
            if (data->imageAvailable[i] == nullptr)
                HG_PANIC("Could not create VkSemaphore: %s\n",
                    vkResultToStr(availableResult));
        }
    }
    else
    {
        data->swapchain = nullptr;
    }

    data->imageIdx = (u32)-1;

    vkDestroySwapchainKHR(vk.device, oldSwapchain, nullptr);
}

Swapchain Swapchain::create(
    void* surface,
    u32 width,
    u32 height,
    GpuPresentMode preferredPresentMode,
    GpuImageUsageFlags imageUsage)
{
    Swapchain swap{};
    swap.data = makeUnique<SwapchainData>();
    swap.data->surface = static_cast<VkSurfaceKHR>(surface);
    swap.data->format = findSwapchainFormat(swap.data->surface);
    swap.data->presentMode = findSwapchainPresentMode(swap.data->surface, preferredPresentMode);
    swap.data->imageUsage = imageUsage;
    swap.data->imageAvailable = Array<VkSemaphore>{vk.frameCount, vk.frameCount};

    for (u32 i = 0; i < vk.frameCount; ++i)
    {
        swap.data->imageAvailable[i] = nullptr;
    }

    swap.resize(width, height);
    return swap;
}

void initImGuiGpu(
    const Swapchain& swap,
    Format colorFormat,
    Format depthFormat,
    Format stencilFormat)
{
    HG_ASSERT(colorFormat != Format_undefined);

    ArenaScope scratch = getScratch();

    VkFormat colorVkFormat = formatToVk(colorFormat);
    VkFormat depthVkFormat = formatToVk(depthFormat);
    VkFormat stencilVkFormat = formatToVk(stencilFormat);

    ImGui_ImplVulkan_InitInfo imguiInfo{};
    imguiInfo.Instance = vk.instance;
    imguiInfo.PhysicalDevice = vk.physicalDevice;
    imguiInfo.Device = vk.device;
    imguiInfo.QueueFamily = vk.queueFamily;
    imguiInfo.Queue = vk.queue;
    imguiInfo.DescriptorPoolSize = 1000;
    imguiInfo.MinImageCount = swap.imageCount();
    imguiInfo.ImageCount = swap.imageCount();
    imguiInfo.MinAllocationSize = 1 << 20;
    imguiInfo.UseDynamicRendering = true;
    imguiInfo.PipelineInfoMain.PipelineRenderingCreateInfo.sType
        = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
    imguiInfo.PipelineInfoMain.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
    imguiInfo.PipelineInfoMain.PipelineRenderingCreateInfo.pColorAttachmentFormats = &colorVkFormat;
    imguiInfo.PipelineInfoMain.PipelineRenderingCreateInfo.depthAttachmentFormat = depthVkFormat;
    imguiInfo.PipelineInfoMain.PipelineRenderingCreateInfo.stencilAttachmentFormat = stencilVkFormat;

    ImGui_ImplVulkan_Init(&imguiInfo);
}

void deinitImGuiGpu()
{
    ImGui_ImplVulkan_Shutdown();
}

void beginImGuiFrameGpu()
{
    ImGui_ImplVulkan_NewFrame();
}

} // namespace internal

void* createImGuiTexture(const GpuView& view, GpuLayout layout)
{
    return ImGui_ImplVulkan_AddTexture(view.data->sampler, view.data->view, gpuLayoutToVk(layout));
}

void destroyImGuiTexture(void* texture)
{
    ImGui_ImplVulkan_RemoveTexture(static_cast<VkDescriptorSet>(texture));
}

void renderImGui(GpuCmd* cmd)
{
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), reinterpret_cast<VkCommandBuffer>(cmd));
}

} // namespace hg
