// KUFTC_Dat_Manager.cpp : 애플리케이션에 대한 진입점을 정의합니다.
//

#include "atlbase.h"
#include "atlapp.h"
#include "atldlgs.h"
#include "framework.h"
#include "KUFTC_Dat_Manager.h"
#include "KUFTCDatManagerDlg.h"

CAppModule _Module;

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPTSTR /*lpstrCmdLine*/, int /*nCmdShow*/)
{
	AtlInitCommonControls(ICC_BAR_CLASSES);	// add flags to support other controls

	HRESULT hRes = _Module.Init(NULL, hInstance);
	ATLASSERT(SUCCEEDED(hRes));

	int nRet = 0;
	// BLOCK: Run application
	{
		CFileDialog dlg(TRUE, L"dat",NULL, 6UL, L"KUFTC Dat File\0*.dat\0All Files\0*.*\0\0");

		if (IDOK == dlg.DoModal())
		{
			CKUFTCDatManagerDlg dlgMain(dlg.m_szFileName);
			dlgMain.DoModal();
		}
		// CMainDlg dlgMain;
		// nRet = (int)dlgMain.DoModal();
	}

	_Module.Term();

	return nRet;
}