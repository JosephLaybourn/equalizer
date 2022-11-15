// Control.h
// -- interface for cross-platform audio controls?
// jsh 9/18

#ifndef JSH_CONTROL_H
#define JSH_CONTROL_H

#include <vector>
#include <windows.h>


class Control {
  public:
    Control(unsigned n, const char *name="Control Class 0.3");
    virtual ~Control(void);
    void setRange(unsigned index, int min, int max);
    void setLabel(unsigned index, const char *label);
    void setValue(unsigned index, int value);
    void show(bool visible=true);
    void setTitle(const char *title);
    virtual void valueChanged(unsigned index, int value) {}
  private:
    HWND window;
    HANDLE thread;
    HANDLE event;
	const char *name;
    struct Data {
      HWND scroll_handle,
           label_handle;
    };
    std::vector<Data> controls;
    static LRESULT CALLBACK WinProc(HWND,UINT,WPARAM,LPARAM);
    static DWORD WINAPI ThrdProc(LPVOID);
};


#endif

