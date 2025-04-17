#if defined(DXVK_WSI_SDL3)

#include "wsi_platform_sdl3.h"
#include "../../util/util_error.h"
#include "../../util/util_string.h"
#include "../../util/util_win32_compat.h"

#include <SDL3/SDL_vulkan.h>

namespace dxvk::wsi {

  Sdl3WsiDriver::Sdl3WsiDriver() {
#ifdef __APPLE__
    libsdl = dlopen("SDL3.framework/SDL3", RTLD_NOW | RTLD_LOCAL);

      if (libsdl == nullptr) {
#endif
#ifdef _WIN32
          libsdl = LoadLibraryA( // FIXME: Get soname as string from meson
#else
          libsdl = dlopen(
#endif
#if defined(_WIN32)
          "SDL3.dll"
#elif defined(__APPLE__)
          "libSDL3.0.dylib"
#else
          "libSDL3.so.0"
#endif
#ifndef _WIN32
          , RTLD_NOW | RTLD_LOCAL
#endif
        );
#ifdef __APPLE__
    }
#endif
    if (libsdl == nullptr)
      throw DxvkError("SDL3 WSI: Failed to load SDL3 DLL.");

#ifdef _WIN32
    #define SDL_PROC(ret, name, params) \
      name = reinterpret_cast<pfn_##name>(GetProcAddress(libsdl, #name)); \
      if (name == nullptr) { \
        FreeLibrary(libsdl); \
        libsdl = nullptr; \
        throw DxvkError("SDL3 WSI: Failed to load " #name "."); \
      }
#else
    #define SDL_PROC(ret, name, params) \
      name = reinterpret_cast<pfn_##name>(dlsym(libsdl, #name)); \
      if (name == nullptr) { \
        dlclose(libsdl); \
        libsdl = nullptr; \
        throw DxvkError("SDL3 WSI: Failed to load " #name "."); \
      }
#endif

    #include "wsi_platform_sdl3_funcs.h"

    if (!SDL_InitSubSystem(SDL_INIT_VIDEO))
      throw DxvkError("SDL3 WSI: Failed to initialize video subsystem."); \
  }

  Sdl3WsiDriver::~Sdl3WsiDriver() {
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
#ifdef _WIN32
    FreeLibrary(libsdl);
#else
    dlclose(libsdl);
#endif
  }

  std::vector<const char *> Sdl3WsiDriver::getInstanceExtensions() {
    if (!SDL_Vulkan_LoadLibrary(nullptr))
      throw DxvkError(str::format("SDL3 WSI: Failed to load Vulkan library: ", SDL_GetError()));

    uint32_t extensionCount = 0;
    auto extensions = SDL_Vulkan_GetInstanceExtensions(&extensionCount);

    if (!extensions)
      throw DxvkError(str::format("SDL3 WSI: Failed to get instance extensions: ", SDL_GetError()));

    std::vector<const char*> result(extensionCount);

    for (uint32_t i = 0; i < extensionCount; i++)
      result[i] = extensions[i];

    return result;
  }

  static bool createSdl3WsiDriver(WsiDriver **driver) {
    try {
      *driver = new Sdl3WsiDriver();
    } catch (const DxvkError& e) {
#ifdef _WIN32
      Logger::err(str::format(e.message()));
#else
      fprintf(stderr, "ERROR: %s\n", e.message().c_str());
#endif
      return false;
    }
    return true;
  }

  WsiBootstrap Sdl3WSI = {
    "SDL3",
    createSdl3WsiDriver
  };

}

#endif
