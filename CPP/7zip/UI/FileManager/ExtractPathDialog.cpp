// ExtractPathDialog.cpp

#include "StdAfx.h"

#include "../../../Windows/FileName.h"
#include "../../../Windows/Shell.h"

#include "../../../Windows/Control/Static.h"

#include "../../UI/FileManager/LangUtils.h"

#include "ExtractPathDialog.h"

#ifdef LANG
#include "LangUtils.h"
#endif

using namespace NWindows;

void CExtractPathDialog::MakeSpace(RECT r, LONG extraY)
{
	int ids[] = {
		IDT_EXTRACT_PATH_PROMPT,
		IDT_EXTRACT_PATH_EXTRA_TEXT,
		IDT_EXTRACT_PATH,
		IDC_EXTRACT_PATH,
		IDB_EXTRACT_PATH_SET_PATH
	};
	RECT c;
	if (GetWindowRect(&c))
		MoveWindow(_window, c.left, c.top, RECT_SIZE_X(c), RECT_SIZE_Y(c) + extraY, FALSE);
	for (int i = 0; i < sizeof(ids) / sizeof(*ids); i++) {
		GetClientRectOfItem(ids[i], c);
		if (c.top >= r.bottom)
			MoveItem(ids[i], c.left, c.top + extraY, RECT_SIZE_X(c), RECT_SIZE_Y(c), false);
	}
}

bool CExtractPathDialog::OnInit()
{
  #ifdef LANG
  LangSetDlgItems(*this, NULL, 0);
  #endif
  _path.Attach(GetItem(IDC_EXTRACT_PATH));
  SetText(Title);

  SetItemText(IDT_EXTRACT_PATH_PROMPT, Prompt);
  RECT r1, r2;
  GetClientRectOfItem(IDT_EXTRACT_PATH_PROMPT, r1);
  if (DrawTextW(GetDC(GetDlgItem(_window, IDT_EXTRACT_PATH_PROMPT)), Prompt, -1, &r2, DT_CALCRECT) &&
	  RECT_SIZE_Y(r2) > RECT_SIZE_Y(r1)) {
    LONG diff = RECT_SIZE_Y(r2) - RECT_SIZE_Y(r1);
	MoveItem(IDT_EXTRACT_PATH_PROMPT, r1.left, r1.top, RECT_SIZE_X(r1), RECT_SIZE_Y(r1) + diff, false);
	MakeSpace(r1, diff);
  }

  if (!Extra.IsEmpty())
    SetItemText(IDT_EXTRACT_PATH_EXTRA_TEXT, Extra);
  else {
    RECT r;
    GetClientRectOfItem(IDT_EXTRACT_PATH_EXTRA_TEXT, r);
    MakeSpace(r, -RECT_SIZE_Y(r));
    HideItem(IDT_EXTRACT_PATH_EXTRA_TEXT);
  }

  NControl::CStatic staticContol;
  staticContol.Attach(GetItem(IDT_EXTRACT_PATH));
  staticContol.SetText(Label);
  #ifdef UNDER_CE
  // we do it, since WinCE selects Value\something instead of Value !!!!
  _path.AddString(Value);
  #endif
  _path.SetText(Value);
  NormalizeSize(true);
  return CModalDialog::OnInit();
}

bool CExtractPathDialog::OnSize(WPARAM /* wParam */, int xSize, int ySize)
{
  int mx, my;
  GetMargins(8, mx, my);
  int bx1, bx2, by;
  GetItemSizes(IDCANCEL, bx1, by);
  GetItemSizes(IDOK, bx2, by);
  int y = ySize - my - by;
  int x = xSize - mx - bx1;

  InvalidateRect(NULL);

  {
    RECT r;
    GetClientRectOfItem(IDB_EXTRACT_PATH_SET_PATH, r);
    int bx = RECT_SIZE_X(r);
    MoveItem(IDB_EXTRACT_PATH_SET_PATH, xSize - mx - bx, r.top, bx, RECT_SIZE_Y(r));
    ChangeSubWindowSizeX(_path, xSize - mx - mx - bx - mx);
  }

  {
    RECT r;
    GetClientRectOfItem(IDT_EXTRACT_PATH_PROMPT, r);
    NControl::CStatic staticContol;
    staticContol.Attach(GetItem(IDT_EXTRACT_PATH_PROMPT));
    int yPos = r.top;
    staticContol.Move(mx, yPos, xSize - mx * 2, y - 2 - yPos);
  }

  MoveItem(IDCANCEL, x, y, bx1, by);
  MoveItem(IDOK, x - mx - bx2, y, bx2, by);

  return false;
}

bool CExtractPathDialog::OnButtonClicked(int buttonID, HWND buttonHWND)
{
  switch (buttonID)
  {
    case IDB_EXTRACT_PATH_SET_PATH:
      OnButtonSetPath();
      return true;
  }
  return CModalDialog::OnButtonClicked(buttonID, buttonHWND);
}

void CExtractPathDialog::OnButtonSetPath()
{
  UString currentPath;
  _path.GetText(currentPath);

  const UString title = LangString(IDS_SET_FOLDER);

  UString resultPath;
  if (!NShell::BrowseForFolder(*this, title, currentPath, resultPath))
    return;
  NFile::NName::NormalizeDirPathPrefix(resultPath);
  _path.SetText(resultPath);
}

void CExtractPathDialog::OnOK()
{
  _path.GetText(Value);
  CModalDialog::OnOK();
}
