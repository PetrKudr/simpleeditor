#ifndef PluginTypes_h__
#define PluginTypes_h__


#include <Windows.h>

#include <vector>


/*
***********************************************************************************
*  Plugin buffer (helper class for passing parameters between notepad and plugin)
***********************************************************************************
*/

template <class T>
struct PluginBuffer
{
  // Array of T
  virtual T* GetBuffer() = 0;

  // Buffer size in T
  virtual size_t GetBufferSize() = 0;

  // Parameter 'amount' - amount of T on which buffer will be increased
  virtual void IncreaseBufferSize(size_t amount) = 0;
};


// Just possible implementation of the structure PluginBuffer
template <class T>
class VectorPluginBuffer : public PluginBuffer<T>
{
private:

  std::vector<T> myVector;


public:

  VectorPluginBuffer()
  {
    myVector.resize(32);  // default length
  }

  T* GetBuffer()
  {
    return &myVector[0];
  }

  size_t GetBufferSize()
  {
    return myVector.size();
  }

  void IncreaseBufferSize(size_t amount)
  {
    if (amount > 0)
      myVector.resize(myVector.size() + amount);
  }
};


/*
***********************************************************************************
*  Notifications
***********************************************************************************
*/


// Callback notification (can be emitted only by plugin itself)
#define PN_CALLBACK 0            // data = void *param (defined by plugin)

// Notification from editor.
#define PN_FROM_EDITOR 1         // data = EditorNotificationData*

// Other plug-in has been unloaded.
#define PN_UNLOAD 2              // data = PluginEventData*

// Other plug-in has been loaded.
#define PN_LOAD 3                // data = PluginEventData*

// New editor has been created
#define PN_EDITOR_CREATED 4      // data = int *id

// Editor has been destroyed
#define PN_EDITOR_DESTROYED 5    // data = int *id

// File has been opened
#define PN_EDITOR_FILE_OPENED 6  // data = int *id

// File has been saved
#define PN_EDITOR_FILE_SAVED 7   // data = int *id



struct PluginNotification
{
  int notification;   // notification id (see above PN_).
  const void *data;   // data which depends on notification
};

// Data for notification PN_FROM_EDITOR
struct EditorNotificationData
{
  int notification;   // original notification
  const void *data;   // original data
};

// Data for notifications PN_UNLOAD and PN_LOAD
struct PluginEventData
{
  LPCWSTR guid;  // guid of the plugin
  int version;   // version of the plugin
};


/*
***************************************************************************************************
*  Plugin parameters which could affect on other plugins 
*  (So if they changed during lifetime of the plugin, it must emit appropriate notification)
***************************************************************************************************
*/


enum PluginParameter
{
  PP_MENU  // should be emitted if plugin changes number of items in its menu.
};

#endif // PluginTypes_h__