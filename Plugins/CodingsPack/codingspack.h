#ifndef codingspack_h__
#define codingspack_h__

#ifdef __cplusplus
extern "C" {
#endif

__declspec(dllexport)
extern Plugin* __stdcall SECreatePlugin();

__declspec(dllexport)
extern void __stdcall SEDeletePlugin(Plugin *plugin);

#ifdef __cplusplus
}
#endif

#endif // codingspack_h__