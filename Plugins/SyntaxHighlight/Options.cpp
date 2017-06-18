#include "Options.hpp"

#include <cwchar>
#include <cstdlib>
#include <cstdio>



void Options::ReadOptions()
{
  ReadFile((myRootPath + DIRECTORY_SEPARATOR + FILE_NAME).c_str());
}

void Options::SaveOptions()
{
  std::wstring pathToFile = (myRootPath + DIRECTORY_SEPARATOR + FILE_NAME);

  if (myOptionsChanged)
  {
    FILE *fOut;

    fOut = _wfopen(pathToFile.c_str(), L"wb");
    if (fOut)
    {
      // write bom
      unsigned short bom = 0xFEFF;
      fwrite(&bom, 2, 1, fOut);

      // save common options
      fwprintf(fOut, L"[COMMON]\n");
      fwprintf(fOut, L"DetectLanguageByExtension=%s\n", myDetectLanguageByExtension ? L"true" : L"false");

      if (myCaretLineTextColor)
        fwprintf(fOut, L"CaretLineTextColor=%.8x\n", *myCaretLineTextColor);

      if (myCaretLineBackgroundColor)
        fwprintf(fOut, L"CaretLineBackgroundColor=%.8x\n", *myCaretLineBackgroundColor);

      fclose(fOut);
    }
  }
}


// static initilization

const std::wstring Options::FILE_NAME(L"options.ini");

const std::wstring Options::SECTION_COMMON(L"COMMON");