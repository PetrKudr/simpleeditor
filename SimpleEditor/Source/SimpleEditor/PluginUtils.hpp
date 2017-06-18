#ifndef PluginUtils_h__
#define PluginUtils_h__

#include <Windows.h>

#include <string>


namespace PluginUtils
{

  template<typename F>
  bool SearchFiles(const std::wstring &searchPath, F &handler)
  {    
    WIN32_FIND_DATA fd = {0};

    HANDLE hfind = FindFirstFile(searchPath.c_str(), &fd);

    if (INVALID_HANDLE_VALUE != hfind)
    {
      do 
      {
        handler(fd);
      } while (FindNextFile(hfind, &fd) > 0);

      FindClose(hfind);

      return true;
    }

    return false;
  }

};

#endif // PluginUtils_h__