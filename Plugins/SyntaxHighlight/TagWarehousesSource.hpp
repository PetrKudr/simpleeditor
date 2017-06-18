#ifndef TagWarehouseSource_h__
#define TagWarehouseSource_h__

#include <map>

#include "PropertyFileParser.hpp"
#include "TagWarehouse.hpp"
#include "HighlighterTools.hpp"

class TagWarehousesSource : public PropertyFileParser
{
private:

  enum Section 
  {
    undefined,
    warehouses
  };


private:

  static const std::wstring SECTION_WAREHOUSES;


private:
  
  static TagWarehousesSource ourInstance;

  std::map<std::wstring, TagWarehouse*> myTagWarehouses;

  std::wstring myRootPath;


  // for loading:

  Section section;


public:

  static TagWarehousesSource& GetInstance() 
  {
    return ourInstance;
  }


public:

  const TagWarehouse* GetTagWarehouse(const std::wstring& name);

  bool Init();

  void Clear();

  const std::wstring& GetRootPath() const;

  void SetRootPath(const std::wstring &rootPath);


protected:

  void HandleSection(const std::wstring &name)
  {
    if (name.compare(SECTION_WAREHOUSES) == 0)
      section = warehouses;
    else
      section = undefined;
  }

  void HandleSectionProperty(const std::wstring &name, const std::wstring &value)
  {
    switch (section)
    {
    case warehouses:
      sectionWarehouses(name, value);
      break;
    }
  }


private:

 void sectionWarehouses(const std::wstring &propertyName, const std::wstring &propertyValue) {
   if (propertyName.compare(L"Warehouse") == 0) 
   {
     TagWarehouse *pwarehouse = new TagWarehouse();
     if (pwarehouse->ReadFile((myRootPath + DIRECTORY_SEPARATOR + propertyValue).c_str()))
     {
       if (!pwarehouse->GetName().empty())
       {
         myTagWarehouses.insert(std::pair<std::wstring, TagWarehouse*>(pwarehouse->GetName(), pwarehouse));
         return;
       }
     }
     delete pwarehouse;
   }
 }

  TagWarehousesSource() : section(undefined) {}

  ~TagWarehousesSource()
  {
    Clear();
  }

};

#endif // TagWarehouseSource_h__
