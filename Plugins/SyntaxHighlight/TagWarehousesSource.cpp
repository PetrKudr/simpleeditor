#include "TagWarehousesSource.hpp"

TagWarehousesSource TagWarehousesSource::ourInstance;

const std::wstring TagWarehousesSource::SECTION_WAREHOUSES(L"WAREHOUSES");


const TagWarehouse* TagWarehousesSource::GetTagWarehouse(const std::wstring& name) 
{
  std::map<std::wstring, TagWarehouse*>::const_iterator iter = myTagWarehouses.find(name);
  return iter != myTagWarehouses.end() ? iter->second : 0;
}

bool TagWarehousesSource::Init()
{
  for (std::map<std::wstring, TagWarehouse*>::iterator iter = myTagWarehouses.begin(); iter != myTagWarehouses.end(); ++iter)
  {
    if (!iter->second->Init())
      return false;
  }
  return true;
}

void TagWarehousesSource::Clear()
{
  for (std::map<std::wstring, TagWarehouse*>::iterator iter = myTagWarehouses.begin(); iter != myTagWarehouses.end(); ++iter)
    delete iter->second;
  myTagWarehouses.clear();
}

const std::wstring& TagWarehousesSource::GetRootPath() const
{
  return myRootPath;
}

void TagWarehousesSource::SetRootPath(const std::wstring &rootPath)
{
  myRootPath = rootPath;
}