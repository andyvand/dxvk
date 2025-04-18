#if defined(DXVK_WSI_SDL2)

#include "wsi_platform_sdl2.h"
#include "../../util/util_error.h"
#include "../../util/util_string.h"
#include "../../util/util_win32_compat.h"

#include <SDL_vulkan.h>

namespace dxvk::wsi {

  Sdl2WsiDriver::Sdl2WsiDriver() {
#ifdef __APPLE__
      libsdl = dlopen("SDL2.framework/SDL2", RTLD_NOW | RTLD_LOCAL);

      if (libsdl == nullptr) {
#endif
#ifdef _WIN32
          libsdl = LoadLibraryA( // FIXME: Get soname as string from meson
#else
          libsdl = dlopen(
#endif
#if defined(_WIN32)
          "SDL2.dll"
#elif defined(__APPLE__)
          "libSDL2-2.0.0.dylib"
#else
          "libSDL2-2.0.so.0"
#endif
#ifndef _WIN32
          , RTLD_NOW | RTLD_LOCAL
#endif
          );
#ifdef __APPLE__
    }
#endif
    if (libsdl == nullptr)
      throw DxvkError("SDL2 WSI: Failed to load SDL2 DLL.");

#ifdef _WIN32
    #define SDL_PROC(ret, name, params) \
      name = reinterpret_cast<pfn_##name>(GetProcAddress(libsdl, #name)); \
      if (name == nullptr) { \
        FreeLibrary(libsdl); \
        libsdl = nullptr; \
        throw DxvkError("SDL2 WSI: Failed to load " #name "."); \
      }
#else
    #define SDL_PROC(ret, name, params) \
        name = reinterpret_cast<pfn_##name>(dlsym(libsdl, #name)); \
        if (name == nullptr) { \
            dlclose(libsdl); \
            libsdl = nullptr; \
            throw DxvkError("SDL2 WSI: Failed to load " #name "."); \
        }
#endif

    #include "wsi_platform_sdl2_funcs.h"
  }

  Sdl2WsiDriver::~Sdl2WsiDriver() {
#ifdef _WIN32
    FreeLibrary(libsdl);
#else
    dlclose(libsdl);
#endif
  }

  std::vector<const char *> Sdl2WsiDriver::getInstanceExtensions() {
    SDL_Vulkan_LoadLibrary(nullptr);

    uint32_t extensionCount = 0;
    if (!SDL_Vulkan_GetInstanceExtensions(nullptr, &extensionCount, nullptr))
      throw DxvkError(str::format("SDL2 WSI: Failed to get instance extension count. ", SDL_GetError()));

    auto extensionNames = std::vector<const char *>(extensionCount);
    if (!SDL_Vulkan_GetInstanceExtensions(nullptr, &extensionCount, extensionNames.data()))
      throw DxvkError(str::format("SDL2 WSI: Failed to get instance extensions. ", SDL_GetError()));

    return extensionNames;
  }

  static bool createSdl2WsiDriver(WsiDriver **driver) {
    try {
      *driver = new Sdl2WsiDriver();
    } catch (const DxvkError& e) {
      return false;
    }
    return true;
  }

  WsiBootstrap Sdl2WSI = {
    "SDL2",
    createSdl2WsiDriver
  };

}

#endif
