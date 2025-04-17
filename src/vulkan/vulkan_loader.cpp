#include <tuple>

#include "vulkan_loader.h"

#include "../util/log/log.h"

#include "../util/util_string.h"
#include "../util/util_win32_compat.h"

namespace dxvk::vk {

#ifdef _WIN32
  static std::pair<HMODULE, PFN_vkGetInstanceProcAddr> loadVulkanLibrary() {
#else
  static std::pair<void *, PFN_vkGetInstanceProcAddr> loadVulkanLibrary() {
#endif
#ifdef __APPLE__
    static const std::array<const char*, 5> dllNames = {{
#else
    static const std::array<const char*, 2> dllNames = {{
#endif
#ifdef _WIN32
      "winevulkan.dll",
      "vulkan-1.dll",
#else
#ifdef __APPLE__
      "libvukan.dylib",
      "libvukan.1.dylib",
      "vulkan.framework/vulkan",
      "libMoltenVK.dylib",
      "MoltenVK.framework/MoltenVK",
#else
      "libvulkan.so",
      "libvulkan.so.1",
#endif
#endif
    }};

    for (auto dllName : dllNames) {
#ifndef _WIN32
      void *library = dlopen(dllName, RTLD_NOW | RTLD_LOCAL);

      if (!library)
        continue;

      auto proc = dlsym(library, "vkGetInstanceProcAddr");
#else
      HMODULE library = LoadLibraryA(dllName);

      if (!library)
        continue;

      auto proc = GetProcAddress(library, "vkGetInstanceProcAddr");
#endif

      if (!proc) {
#ifdef _WIN32
        FreeLibrary(library);
#else
        dlclose(library);
#endif

        continue;
      }

      Logger::info(str::format("Vulkan: Found vkGetInstanceProcAddr in ", dllName, " @ 0x", std::hex, reinterpret_cast<uintptr_t>(proc)));
      return std::make_pair(library, reinterpret_cast<PFN_vkGetInstanceProcAddr>(proc));
    }

    Logger::err("Vulkan: vkGetInstanceProcAddr not found");
    return { };
  }

  LibraryLoader::LibraryLoader() {
    std::tie(m_library, m_getInstanceProcAddr) = loadVulkanLibrary();
  }

  LibraryLoader::LibraryLoader(PFN_vkGetInstanceProcAddr loaderProc) {
    m_getInstanceProcAddr = loaderProc;
  }

  LibraryLoader::~LibraryLoader() {
    if (m_library)
#ifdef _WIN32
      FreeLibrary(m_library);
#else
      dlclose(m_library);
#endif
  }

  PFN_vkVoidFunction LibraryLoader::sym(VkInstance instance, const char* name) const {
    return m_getInstanceProcAddr(instance, name);
  }

  PFN_vkVoidFunction LibraryLoader::sym(const char* name) const {
    return sym(nullptr, name);
  }

  bool LibraryLoader::valid() const {
    return m_getInstanceProcAddr != nullptr;
  }
  
  
  InstanceLoader::InstanceLoader(const Rc<LibraryLoader>& library, bool owned, VkInstance instance)
  : m_library(library), m_instance(instance), m_owned(owned) { }
  
  
  PFN_vkVoidFunction InstanceLoader::sym(const char* name) const {
    return m_library->sym(m_instance, name);
  }
  
  
  DeviceLoader::DeviceLoader(const Rc<InstanceLoader>& library, bool owned, VkDevice device)
  : m_library(library)
  , m_getDeviceProcAddr(reinterpret_cast<PFN_vkGetDeviceProcAddr>(
      m_library->sym("vkGetDeviceProcAddr"))),
    m_device(device), m_owned(owned) { }
  
  
  PFN_vkVoidFunction DeviceLoader::sym(const char* name) const {
    return m_getDeviceProcAddr(m_device, name);
  }
  
  
  LibraryFn::LibraryFn() { }
  LibraryFn::LibraryFn(PFN_vkGetInstanceProcAddr loaderProc)
  : LibraryLoader(loaderProc) { }
  LibraryFn::~LibraryFn() { }
  
  
  InstanceFn::InstanceFn(const Rc<LibraryLoader>& library, bool owned, VkInstance instance)
  : InstanceLoader(library, owned, instance) { }
  InstanceFn::~InstanceFn() {
    if (m_owned)
      this->vkDestroyInstance(m_instance, nullptr);
  }
  
  
  DeviceFn::DeviceFn(const Rc<InstanceLoader>& library, bool owned, VkDevice device)
  : DeviceLoader(library, owned, device) { }
  DeviceFn::~DeviceFn() {
    if (m_owned)
      this->vkDestroyDevice(m_device, nullptr);
  }
  
}
