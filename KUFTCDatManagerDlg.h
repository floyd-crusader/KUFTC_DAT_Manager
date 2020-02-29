#pragma once

#include "resource.h"
#include "atlbase.h"
#include "atlwin.h"
#include "atlapp.h"
#include "atlctrls.h"
#include <string>
#include <vector>
#include <memory>

struct DAT_FILE_HEADER
{
	char szHeaderString[32];
	int nVersion;
	int nDirCount;
	char cDummy[24];
};

struct FILE_ITEM_RAW
{
	char szFileName[32];
	int nOffset;
	int nSize;
};

struct DAT_DIR;

struct FILE_ITEM
{
	std::string strFolder;
	std::string strFileName;
	int nOffset;
	int nSize;
};

struct DAT_DIR
{
	std::string strDir;
	std::vector<FILE_ITEM> vecFileItems;
};

struct TREE_DATA
{
	enum TYPE { ROOT, DIR, FILE } type;

	union
	{
		wchar_t wszDATFileName[MAX_PATH];
		DAT_DIR* pDatDir;
		FILE_ITEM* pFileItem;
	};
};

class CKUFTCDatManagerDlg : public CDialogImpl<CKUFTCDatManagerDlg>
{
public:
	CKUFTCDatManagerDlg(const std::wstring& strFileName);
	~CKUFTCDatManagerDlg();

	enum { IDD = IDD_DIALOG_MAIN };

	BEGIN_MSG_MAP(CKUFTCDatManagerDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_WINDOWPOSCHANGED, OnWindowPosChanged)

		NOTIFY_CODE_HANDLER(NM_RCLICK, OnNMRClick)

		COMMAND_ID_HANDLER(IDCANCEL, OnClose)
	END_MSG_MAP()

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnWindowPosChanged(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnClose(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	LRESULT OnNMRClick(int idFrom, LPNMHDR pnmh, BOOL&);
private:
	void ExtractAllTo(LPCTSTR szFolder, HANDLE hFile);
	void ExtractDirTo(LPCTSTR szFolder, HANDLE hFile, DAT_DIR* pDir);
	void ExtractFileTo(LPCTSTR szFolder, HANDLE hFile, FILE_ITEM* pDir);

private:
	std::wstring m_strDATFileName;
	std::vector <std::unique_ptr<DAT_DIR>> m_vecDirectories;

	WTL::CTreeViewCtrlEx m_wndTreeView;
	TREE_DATA* m_pTreeData;
};

