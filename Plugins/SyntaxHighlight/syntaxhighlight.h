#ifndef __SYNTAX_HIGHLIGHT_
#define __SYNTAX_HIGHLIGHT_

#define SE_PLUGIN

#include "seditcontrol.h"
#include "Plugin.hpp"

#ifdef COMPILE_DLL_HIGHLIGHT
  #define DECLSPEC_ATTR dllexport
#else
  #define DECLSPEC_ATTR dllimport
#endif

#ifdef __cplusplus
extern "C" {
#endif

__declspec(DECLSPEC_ATTR)
extern Plugin* __stdcall SECreatePlugin();

__declspec(DECLSPEC_ATTR)
extern void __stdcall SEDeletePlugin(Plugin *plugin);

#ifdef __cplusplus
}
#endif

#endif /* __SYNTAX_HIGHLIGHT_ */