#ifndef hlalloc_h__
#define hlalloc_h__

//#define HL_SELF_MEMORY_SYSTEM

#ifdef HL_SELF_MEMORY_SYSTEM

#include <new>


extern void InitMemorySystem();

extern void CloseMemorySystem();

extern int GetNativeAllocateCount();

extern int GetNativeFreeCount();


extern void* __cdecl operator new(size_t size);

extern void __cdecl operator delete(void *p);

extern void* __cdecl operator new[](size_t size);

extern void __cdecl operator delete[](void *p);

#endif


#ifdef HL_SELF_MEMORY_SYSTEM
  #define HL_INIT_MEMORY_SYSTEM() InitMemorySystem()
  #define HL_CLOSE_MEMORY_SYSTEM() CloseMemorySystem()
#else
  #define HL_INIT_MEMORY_SYSTEM()
  #define HL_CLOSE_MEMORY_SYSTEM()
#endif



#endif // hlalloc_h__