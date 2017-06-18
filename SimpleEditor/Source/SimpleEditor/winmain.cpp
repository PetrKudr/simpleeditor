#include "notepad.hpp"
#include "resources\res.hpp"



#if !defined(_WIN32_WCE)
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
#else
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
#endif
{
  CNotepad &Notepad = CNotepad::Instance();
  bool success;

  // Initialization
#if !defined(_WIN32_WCE)
	success = Notepad.InitNotepad(hInstance, GetCommandLine());
#else
  success = Notepad.InitNotepad(hInstance, lpCmdLine);
#endif
  if (!success)
  {
    if (CError::GetError().code != NOTEPAD_NO_ERROR)
      Notepad.Interface.Dialogs.ShowError(0, CError::GetError().message);
    return 0;
  }

  HWND hwnd = Notepad.Interface.GetHWND();
  HACCEL haccel = LoadAccelerators(hInstance, NOTEPAD_ACCELLERATORS); 

  MSG message;
  int ret;

	while ((ret = GetMessage(&message, NULL, 0, 0)) != 0 && ret != -1)
	{
    //if (!hfrWnd || !IsDialogMessage(hfrWnd, &message))
   // {
      if (!TranslateAccelerator(hwnd, haccel, &message))
      {
  		  TranslateMessage(&message);
	  	  DispatchMessage(&message);
      }
    //}
	}
  return message.wParam;
}