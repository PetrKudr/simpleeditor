#ifndef LanguagesSource_h__
#define LanguagesSource_h__

#include <map>

#include "PropertyFileParser.hpp"
#include "Language.hpp"
#include "TagWarehousesSource.hpp"
#include "HighlightParsersSource.hpp"


class LanguagesSource : public PropertyFileParser
{
private:

  enum Section 
  {
    language,
    undefined
  };


private:

  static const std::wstring FILE_NAME;

  static const std::wstring SECTION_LANGUAGE;


private:
  
  static LanguagesSource ourInstance;

  std::map<std::wstring, Language*> myLanguages;


  // for loading:

  Language *myCurrentLanguage;

  Section section;


public:

  static LanguagesSource& GetInstance() 
  {
    return ourInstance;
  }


public:

  const Language* GetLanguage(const std::wstring& name) const
  {
    std::map<std::wstring, Language*>::const_iterator iter = myLanguages.find(name);
    return iter != myLanguages.end() ? iter->second : 0;
  }

  const Language* GetLanguageByExtension(const std::wstring &extension) const
  {
    for (std::map<std::wstring, Language*>::const_iterator iterLanguage = myLanguages.begin(); iterLanguage != myLanguages.end(); ++iterLanguage)
    {
      const std::vector<std::wstring> &extensions = iterLanguage->second->GetExtensions();

      for (std::vector<std::wstring>::const_iterator iterExtension = extensions.begin(); iterExtension != extensions.end(); ++iterExtension)
      {
        if (extension.compare(*iterExtension) == 0)
          return iterLanguage->second; 
      }
    }
    return 0;
  }

  const std::map<std::wstring, Language*>& GetLanguagesMap() const
  {
    return myLanguages;
  }

  bool Init()
  {
    for (std::map<std::wstring, Language*>::iterator iter = myLanguages.begin(); iter != myLanguages.end(); ++iter)
    {
      if (!iter->second->Init())
        return false;
    }
    return true;
  }

  void Clear()
  {
    for (std::map<std::wstring, Language*>::iterator iter = myLanguages.begin(); iter != myLanguages.end(); ++iter)
      delete iter->second;
    myLanguages.clear();
  }


protected:

  void HandleSection(const std::wstring &name)
  {
    handleEndOfSection();

    if (name.compare(SECTION_LANGUAGE) == 0)
    {
      myCurrentLanguage = new Language();
      section = language;
    }
    else
      section = undefined;
  }

  void HandleSectionProperty(const std::wstring &name, const std::wstring &value)
  {
    switch (section)
    {
    case language:
      sectionLanguage(name, value);
      break;
    }
  }

  void HandleEndOfFile() 
  {
    handleEndOfSection();
  }


private:

  void sectionLanguage(const std::wstring &propertyName, const std::wstring &propertyValue) 
  {
    if (propertyName.compare(L"Name") == 0) 
      myCurrentLanguage->SetName(propertyValue);
    else if (propertyName.compare(L"TagWarehouse") == 0) 
      myCurrentLanguage->SetTagWarehouseName(propertyValue);
    else if (propertyName.compare(L"HighlightParser") == 0) 
      myCurrentLanguage->SetHighlighParserName(propertyValue);
    else if (propertyName.compare(L"Exported") == 0)
      myCurrentLanguage->SetExported(parseInt(propertyValue, 10) != 0);
    else if (propertyName.compare(L"ExportName") == 0)
      myCurrentLanguage->SetExportName(propertyValue);
    else if (propertyName.compare(L"Extensions") == 0)
    {
      std::vector<std::wstring> extensions;

      unsigned int extStart = 0;
      for (unsigned int i = 0; i < propertyValue.size(); ++i)
      {
        if (propertyValue.at(i) == L';')
        {
          extensions.push_back(propertyValue.substr(extStart, i - extStart));
          extStart = i + 1;
        }
      }
      myCurrentLanguage->SetExtensions(extensions);
    }
  }

  void putLanguage() 
  {
    if (0 != myCurrentLanguage && !myCurrentLanguage->GetName().empty())
    {
      myLanguages.insert(std::pair<std::wstring, Language*>(myCurrentLanguage->GetName(), myCurrentLanguage));
      myCurrentLanguage = 0;
    }
    else if (0 != myCurrentLanguage)
    {
      delete myCurrentLanguage;
      myCurrentLanguage = 0;
    }
  }

  void handleEndOfSection()
  {
    switch (section)
    {
    case language:
      putLanguage();
    }
  }

  LanguagesSource() : myCurrentLanguage(0), section(undefined) {}

  ~LanguagesSource() 
  {
    Clear(); 
  }

};
#endif // LanguagesSource_h__
