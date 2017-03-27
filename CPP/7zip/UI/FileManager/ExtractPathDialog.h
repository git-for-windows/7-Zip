// ExtractPathDialog.h

#ifndef __EXTRACT_PATH_DIALOG_H
#define __EXTRACT_PATH_DIALOG_H

#include "../../../Windows/Control/Edit.h"
#include "../../../Windows/Control/Dialog.h"

#include "ExtractPathDialogRes.h"

const int kExtractPathDialog_NumInfoLines = 11;

class CExtractPathDialog: public NWindows::NControl::CModalDialog
{
  NWindows::NControl::CEdit _path;
  virtual void OnOK();
  virtual bool OnInit();
  virtual bool OnSize(WPARAM wParam, int xSize, int ySize);
  void OnButtonSetPath();
  bool OnButtonClicked(int buttonID, HWND buttonHWND);
  void MakeSpace(RECT r, LONG extraY);
public:
  UString Title;
  UString Prompt;
  UString Extra;
  UString Label;
  UString Value;

  INT_PTR Create(HWND parentWindow = 0) { return CModalDialog::Create(IDD_EXTRACT_PATH, parentWindow); }
};

#endif
