#pragma once
#ifdef _WIN32
 #ifdef CRAW2LIB_EXPORT
  #define EXPORT __declspec(dllexport)
 #else
  #define EXPORT __declspec(dllimport)
 #endif
#else
 #define EXPORT 
#endif