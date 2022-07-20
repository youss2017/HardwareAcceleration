#include "ImplementationContext.hpp"
#include <map>

// From https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkResult.html

/*
    VK_SUCCESS = 0,
    VK_NOT_READY = 1,
    VK_TIMEOUT = 2,
    VK_EVENT_SET = 3,
    VK_EVENT_RESET = 4,
    VK_INCOMPLETE = 5,
    VK_ERROR_OUT_OF_HOST_MEMORY = -1,
    VK_ERROR_OUT_OF_DEVICE_MEMORY = -2,
    VK_ERROR_INITIALIZATION_FAILED = -3,
    VK_ERROR_DEVICE_LOST = -4,
    VK_ERROR_MEMORY_MAP_FAILED = -5,
    VK_ERROR_LAYER_NOT_PRESENT = -6,
    VK_ERROR_EXTENSION_NOT_PRESENT = -7,
    VK_ERROR_FEATURE_NOT_PRESENT = -8,
    VK_ERROR_INCOMPATIBLE_DRIVER = -9,
    VK_ERROR_TOO_MANY_OBJECTS = -10,
    VK_ERROR_FORMAT_NOT_SUPPORTED = -11,
    VK_ERROR_FRAGMENTED_POOL = -12,
    VK_ERROR_UNKNOWN = -13,
    VK_ERROR_OUT_OF_POOL_MEMORY = -1000069000,
    VK_ERROR_INVALID_EXTERNAL_HANDLE = -1000072003,
    VK_ERROR_FRAGMENTATION = -1000161000,
    VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS = -1000257000,
    VK_PIPELINE_COMPILE_REQUIRED = 1000297000,
    VK_ERROR_SURFACE_LOST_KHR = -1000000000,
    VK_ERROR_NATIVE_WINDOW_IN_USE_KHR = -1000000001,
    VK_SUBOPTIMAL_KHR = 1000001003,
    VK_ERROR_OUT_OF_DATE_KHR = -1000001004,
    VK_ERROR_INCOMPATIBLE_DISPLAY_KHR = -1000003001,
    VK_ERROR_VALIDATION_FAILED_EXT = -1000011001,
    VK_ERROR_INVALID_SHADER_NV = -1000012000,
    VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR = -1000023000,
    VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR = -1000023001,
    VK_ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR = -1000023002,
    VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR = -1000023003,
    VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR = -1000023004,
    VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR = -1000023005,
    VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT = -1000158000,
    VK_ERROR_NOT_PERMITTED_KHR = -1000174001,
    VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT = -1000255000,
    VK_THREAD_IDLE_KHR = 1000268000,
    VK_THREAD_DONE_KHR = 1000268001,
    VK_OPERATION_DEFERRED_KHR = 1000268002,
    VK_OPERATION_NOT_DEFERRED_KHR = 1000268003,
    VK_ERROR_COMPRESSION_EXHAUSTED_EXT = -1000338000,
*/

std::string HA::GetStringFromResult(VkResult result)
{
    // List of Mappings
    static std::map<VkResult, std::string> Mappings;
    if (Mappings.size() == 0)
    {
        Mappings.insert({VK_SUCCESS, "VK_SUCCESS"});
        Mappings.insert({VK_NOT_READY, "VK_NOT_READY"});
        Mappings.insert({VK_TIMEOUT, "VK_TIMEOUT"});
        Mappings.insert({VK_EVENT_SET, "VK_EVENT_SET"});
        Mappings.insert({VK_EVENT_RESET, "VK_EVENT_RESET"});
        Mappings.insert({VK_INCOMPLETE, "VK_INCOMPLETE"});
        Mappings.insert({VK_ERROR_OUT_OF_HOST_MEMORY, "VK_ERROR_OUT_OF_HOST_MEMORY"});
        Mappings.insert({VK_ERROR_OUT_OF_DEVICE_MEMORY, "VK_ERROR_OUT_OF_DEVICE_MEMORY"});
        Mappings.insert({VK_ERROR_INITIALIZATION_FAILED, "VK_ERROR_INITIALIZATION_FAILED"});
        Mappings.insert({VK_ERROR_DEVICE_LOST, "VK_ERROR_DEVICE_LOST"});
        Mappings.insert({VK_ERROR_MEMORY_MAP_FAILED, "VK_ERROR_MEMORY_MAP_FAILED"});
        Mappings.insert({VK_ERROR_LAYER_NOT_PRESENT, "VK_ERROR_LAYER_NOT_PRESENT"});
        Mappings.insert({VK_ERROR_EXTENSION_NOT_PRESENT, "VK_ERROR_EXTENSION_NOT_PRESENT"});
        Mappings.insert({VK_ERROR_FEATURE_NOT_PRESENT, "VK_ERROR_FEATURE_NOT_PRESENT"});
        Mappings.insert({VK_ERROR_INCOMPATIBLE_DRIVER, "VK_ERROR_INCOMPATIBLE_DRIVER"});
        Mappings.insert({VK_ERROR_TOO_MANY_OBJECTS, "VK_ERROR_TOO_MANY_OBJECTS"});
        Mappings.insert({VK_ERROR_FORMAT_NOT_SUPPORTED, "VK_ERROR_FORMAT_NOT_SUPPORTED"});
        Mappings.insert({VK_ERROR_FRAGMENTED_POOL, "VK_ERROR_FRAGMENTED_POOL"});
        Mappings.insert({VK_ERROR_UNKNOWN, "VK_ERROR_UNKNOWN"});
        Mappings.insert({VK_ERROR_OUT_OF_POOL_MEMORY, "VK_ERROR_OUT_OF_POOL_MEMORY"});
        Mappings.insert({VK_ERROR_INVALID_EXTERNAL_HANDLE, "VK_ERROR_INVALID_EXTERNAL_HANDLE"});
        Mappings.insert({VK_ERROR_FRAGMENTATION, "VK_ERROR_FRAGMENTATION"});
        Mappings.insert({VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS, "VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS"});
        Mappings.insert({VK_PIPELINE_COMPILE_REQUIRED, "VK_PIPELINE_COMPILE_REQUIRED"});
        Mappings.insert({VK_ERROR_SURFACE_LOST_KHR, "VK_ERROR_SURFACE_LOST_KHR"});
        Mappings.insert({VK_ERROR_NATIVE_WINDOW_IN_USE_KHR, "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR"});
        Mappings.insert({VK_SUBOPTIMAL_KHR, "VK_SUBOPTIMAL_KHR"});
        Mappings.insert({VK_ERROR_OUT_OF_DATE_KHR, "VK_ERROR_OUT_OF_DATE_KHR"});
        Mappings.insert({VK_ERROR_INCOMPATIBLE_DISPLAY_KHR, "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR"});
        Mappings.insert({VK_ERROR_VALIDATION_FAILED_EXT, "VK_ERROR_VALIDATION_FAILED_EXT"});
        Mappings.insert({VK_ERROR_INVALID_SHADER_NV, "VK_ERROR_INVALID_SHADER_NV"});
        Mappings.insert({VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT, "VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT"});
        Mappings.insert({VK_ERROR_NOT_PERMITTED_KHR, "VK_ERROR_NOT_PERMITTED_KHR"});
        Mappings.insert({VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT, "VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT"});
        Mappings.insert({VK_THREAD_IDLE_KHR, "VK_THREAD_IDLE_KHR"});
        Mappings.insert({VK_THREAD_DONE_KHR, "VK_THREAD_DONE_KHR"});
        Mappings.insert({VK_OPERATION_DEFERRED_KHR, "VK_OPERATION_DEFERRED_KHR"});
        Mappings.insert({VK_OPERATION_NOT_DEFERRED_KHR, "VK_OPERATION_NOT_DEFERRED_KHR"});
    }
    try
    {
        return Mappings[result];
    }
    catch (...)
    {
        return "[VkResult Unknown]";
    }
}

/*
Success Codes
VK_SUCCESS Command successfully completed
VK_NOT_READY A fence or query has not yet completed
VK_TIMEOUT A wait operation has not completed in the specified time
VK_EVENT_SET An event is signaled
VK_EVENT_RESET An event is unsignaled
VK_INCOMPLETE A return array was too small for the result
VK_SUBOPTIMAL_KHR A swapchain no longer matches the surface properties exactly, but can still be used to present to the surface successfully.
VK_THREAD_IDLE_KHR A deferred operation is not complete but there is currently no work for this thread to do at the time of this call.
VK_THREAD_DONE_KHR A deferred operation is not complete but there is no work remaining to assign to additional threads.
VK_OPERATION_DEFERRED_KHR A deferred operation was requested and at least some of the work was deferred.
VK_OPERATION_NOT_DEFERRED_KHR A deferred operation was requested and no operations were deferred.
VK_PIPELINE_COMPILE_REQUIRED A requested pipeline creation would have required compilation, but the application requested compilation to not be performed.

Error codes
A host memory allocation has failed.
A device memory allocation has failed.
Initialization of an object could not be completed for implementation-specific reasons.
The logical or physical device has been lost. See Lost Device
Mapping of a memory object has failed.
A requested layer is not present or could not be loaded.
A requested extension is not supported.
A requested feature is not supported.
The requested version of Vulkan is not supported by the driver or is otherwise incompatible for implementation-specific reasons.
Too many objects of the type have already been created.
A requested format is not supported on this device.
A pool allocation has failed due to fragmentation of the pool’s memory. This must only be returned if no attempt to allocate host or device memory was made to accommodate the new allocation. This should be returned in preference to VK_ERROR_OUT_OF_POOL_MEMORY, but only if the implementation is certain that the pool allocation failure was due to fragmentation.
A surface is no longer available.
The requested window is already in use by Vulkan or another API in a manner which prevents it from being used again.
A surface has changed in such a way that it is no longer compatible with the swapchain, and further presentation requests using the swapchain will fail. Applications must query the new surface properties and recreate their swapchain if they wish to continue presenting to the surface.
The display used by a swapchain does not use the same presentable image layout, or is incompatible in a way that prevents sharing an image.
One or more shaders failed to compile or link. More details are reported back to the application via VK_EXT_debug_report if enabled.
A pool memory allocation has failed. This must only be returned if no attempt to allocate host or device memory was made to accommodate the new allocation. If the failure was definitely due to fragmentation of the pool, VK_ERROR_FRAGMENTED_POOL should be returned instead.
An external handle is not a valid handle of the specified type.
A descriptor pool creation has failed due to fragmentation.
A buffer creation failed because the requested address is not available.
A buffer creation or memory allocation failed because the requested address is not available. A shader group handle assignment failed because the requested shader group handle information is no longer valid.
An operation on a swapchain created with VK_FULL_SCREEN_EXCLUSIVE_APPLICATION_CONTROLLED_EXT failed as it did not have exlusive full-screen access. This may occur due to implementation-dependent reasons, outside of the application’s control.
An image creation failed because internal resources required for compression are exhausted. This must only be returned when fixed-rate compression is requested.
The requested VkImageUsageFlags are not supported.
The requested video picture layout is not supported.
A video profile operation specified via VkVideoProfileKHR::videoCodecOperation is not supported.
Format parameters in a requested VkVideoProfileKHR chain are not supported.
Codec-specific parameters in a requested VkVideoProfileKHR chain are not supported.
The specified video Std header version is not supported.
An unknown error has occurred; either the application has provided invalid input, or an implementation failure has occurred.
*/

std::string HA::GetDescriptionFromResult(VkResult result)
{
    std::map<VkResult, std::string> Mappings;
    if(Mappings.size() == 0) {
        Mappings.insert({VK_SUCCESS, "Command successfully completed"});
        Mappings.insert({VK_NOT_READY, "A fence or query has not yet completed"});
        Mappings.insert({VK_TIMEOUT, "A wait operation has not completed in the specified time"});
        Mappings.insert({VK_EVENT_SET, "An event is signaled"});
        Mappings.insert({VK_EVENT_RESET, "An event is unsignaled"});
        Mappings.insert({VK_INCOMPLETE, "A return array was too small for the result"});
        Mappings.insert({VK_SUBOPTIMAL_KHR, "A swapchain no longer matches the surface properties exactly, but can still be used to present to the surface successfully."});
        Mappings.insert({VK_THREAD_IDLE_KHR, "A deferred operation is not complete but there is currently no work for this thread to do at the time of this call."});
        Mappings.insert({VK_THREAD_DONE_KHR, "A deferred operation is not complete but there is no work remaining to assign to additional threads."});
        Mappings.insert({VK_OPERATION_DEFERRED_KHR, "A deferred operation was requested and at least some of the work was deferred."});
        Mappings.insert({VK_OPERATION_NOT_DEFERRED_KHR, "A deferred operation was requested and no operations were deferred."});
        Mappings.insert({VK_PIPELINE_COMPILE_REQUIRED, "A requested pipeline creation would have required compilation, but the application requested compilation to not be performed."});
        Mappings.insert({VK_ERROR_OUT_OF_HOST_MEMORY, "A host memory allocation has failed."});
        Mappings.insert({VK_ERROR_OUT_OF_DEVICE_MEMORY, "A device memory allocation has failed."});
        Mappings.insert({VK_ERROR_INITIALIZATION_FAILED, "Initialization of an object could not be completed for implementation-specific reasons."});
        Mappings.insert({VK_ERROR_DEVICE_LOST, "The logical or physical device has been lost. See Lost Device"});
        Mappings.insert({VK_ERROR_MEMORY_MAP_FAILED, "Mapping of a memory object has failed."});
        Mappings.insert({VK_ERROR_LAYER_NOT_PRESENT, "A requested layer is not present or could not be loaded."});
        Mappings.insert({VK_ERROR_EXTENSION_NOT_PRESENT, "A requested extension is not supported."});
        Mappings.insert({VK_ERROR_FEATURE_NOT_PRESENT, "A requested feature is not supported."});
        Mappings.insert({VK_ERROR_INCOMPATIBLE_DRIVER, "The requested version of Vulkan is not supported by the driver or is otherwise incompatible for implementation-specific reasons."});
        Mappings.insert({VK_ERROR_TOO_MANY_OBJECTS, "Too many objects of the type have already been created."});
        Mappings.insert({VK_ERROR_FORMAT_NOT_SUPPORTED, "A requested format is not supported on this device."});
        Mappings.insert({VK_ERROR_FRAGMENTED_POOL, "A pool allocation has failed due to fragmentation of the pool’s memory. This must only be returned if no attempt to allocate host or device memory was made to accommodate the new allocation. This should be returned in preference to VK_ERROR_OUT_OF_POOL_MEMORY, but only if the implementation is certain that the pool allocation failure was due to fragmentation."});
        Mappings.insert({VK_ERROR_SURFACE_LOST_KHR, "A surface is no longer available."});
        Mappings.insert({VK_ERROR_NATIVE_WINDOW_IN_USE_KHR, "The requested window is already in use by Vulkan or another API in a manner which prevents it from being used again."});
        Mappings.insert({VK_ERROR_OUT_OF_DATE_KHR, "A surface has changed in such a way that it is no longer compatible with the swapchain, and further presentation requests using the swapchain will fail. Applications must query the new surface properties and recreate their swapchain if they wish to continue presenting to the surface."});
        Mappings.insert({VK_ERROR_INCOMPATIBLE_DISPLAY_KHR, "The display used by a swapchain does not use the same presentable image layout, or is incompatible in a way that prevents sharing an image."});
        Mappings.insert({VK_ERROR_INVALID_SHADER_NV, "One or more shaders failed to compile or link. More details are reported back to the application via VK_EXT_debug_report if enabled."});
        Mappings.insert({VK_ERROR_OUT_OF_POOL_MEMORY, "A pool memory allocation has failed. This must only be returned if no attempt to allocate host or device memory was made to accommodate the new allocation. If the failure was definitely due to fragmentation of the pool, VK_ERROR_FRAGMENTED_POOL should be returned instead."});
        Mappings.insert({VK_ERROR_INVALID_EXTERNAL_HANDLE, "An external handle is not a valid handle of the specified type."});
        Mappings.insert({VK_ERROR_FRAGMENTATION, "A descriptor pool creation has failed due to fragmentation."});
        Mappings.insert({VK_ERROR_INVALID_DEVICE_ADDRESS_EXT, "A buffer creation failed because the requested address is not available."});
        Mappings.insert({VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS, "A buffer creation or memory allocation failed because the requested address is not available. A shader group handle assignment failed because the requested shader group handle information is no longer valid."});
        Mappings.insert({VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT, "An operation on a swapchain created with VK_FULL_SCREEN_EXCLUSIVE_APPLICATION_CONTROLLED_EXT failed as it did not have exlusive full-screen access. This may occur due to implementation-dependent reasons, outside of the application’s control."});
        Mappings.insert({VK_ERROR_UNKNOWN, "An unknown error has occurred; either the application has provided invalid input, or an implementation failure has occurred."});
    }
    try {
        return Mappings[result];
    }
    catch(...) {
        return "Description not found.";
    }
}
