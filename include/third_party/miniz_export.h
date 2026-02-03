#pragma once

#if defined(_WIN32) && defined(MINIZ_DLL)
  #if defined(MINIZ_DLL_EXPORT)
    #define MINIZ_EXPORT __declspec(dllexport)
  #else
    #define MINIZ_EXPORT __declspec(dllimport)
  #endif
#else
  #define MINIZ_EXPORT
#endif

