Sources of notepad for Windows Mobile 5.0 and later, known as CrossPad.

This is an university project which later was ported to Windows Mobile and further developed as notepad.

Discussion was here: http://4pda.ru/forum/index.php?showtopic=161905

Repo contains three projects:
SimpleEditControl - base project for the notepad and plugins. Implementation of edit control with support of big files (more than 64K).
SimpleEditor - notepad itself.
Plugins - folder with various plugins for the notepad.


How to build:
the project was created in VS2008, so is supposed to be built using VS2008.
1) Build SimpleEditControl.
2) Put built .lib file into folder SimpleEditor/Source/SEditControl. Rename library according to README file.
3) Build SimpleEditor.
