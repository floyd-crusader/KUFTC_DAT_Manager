#include "KUFTCDatManagerDlg.h"
#include <vector>
#include "resource.h"
#include "atldlgs.h"

std::wstring str2wstr(const std::string& str)
{
	wchar_t wszTemp[MAX_PATH] = { 0, };

	MultiByteToWideChar(CP_ACP, 0, str.c_str(), str.length(), wszTemp, MAX_PATH);

	return wszTemp;
}

CKUFTCDatManagerDlg::CKUFTCDatManagerDlg(const std::wstring& strFileName)
	: m_strDATFileName(strFileName)
{
}

CKUFTCDatManagerDlg::~CKUFTCDatManagerDlg()
{
	delete[] m_pTreeData;
}

LRESULT CKUFTCDatManagerDlg::OnWindowPosChanged(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
{
	RECT rcClient;

	GetClientRect(&rcClient);
	GetDlgItem(IDC_TREE_DAT).MoveWindow( rcClient.left + 5, rcClient.top + 5, rcClient.right - 10, rcClient.bottom - 10);
	return TRUE;
}

LRESULT CKUFTCDatManagerDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	HICON hIcon = LoadIcon(NULL, MAKEINTRESOURCE(IDI_KUFTCDATMANAGER));
	SetIcon(hIcon, 0);
	SetIcon(hIcon, 1);
	CenterWindow();

	HANDLE hFile = CreateFile(m_strDATFileName.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		MessageBox(L"Can't open file", L"KUFTC Data File Manager", MB_OK | MB_ICONERROR);
		return FALSE;
	}

	DWORD dwReadBytes = 0;
	int nHeaderOffset = 0;

	DWORD result = SetFilePointer(hFile, -4, 0, FILE_END);

	BOOL bRet = ReadFile(hFile, &nHeaderOffset, 4, &dwReadBytes, NULL);
	if (!bRet)
	{
		MessageBox(L"Can't read header offset", L"KUFTC Data File Manager", MB_OK | MB_ICONERROR);
		return FALSE;
	}

	SetFilePointer(hFile, nHeaderOffset, 0, FILE_BEGIN);

	DAT_FILE_HEADER header;

	if (!ReadFile(hFile, &header, sizeof(DAT_FILE_HEADER), &dwReadBytes, NULL))
	{
		MessageBox(L"Can't read header", L"KUFTC Data File Manager", MB_OK | MB_ICONERROR);
		return FALSE;
	}
	std::vector <DAT_DIR> vecDatDirectory;

	int nTotalTreeData = 1 + header.nDirCount;

	for (int i = 0; i < header.nDirCount; ++i)
	{
		std::unique_ptr<DAT_DIR> datDir = std::make_unique<DAT_DIR>();
		int nFileCount;

		if (!ReadFile(hFile, &nFileCount, sizeof(int), &dwReadBytes, NULL))
		{
			MessageBox(L"Can't read File Count", L"KUFTC Data File Manager", MB_OK | MB_ICONERROR);
			CloseHandle(hFile);
			return FALSE;
		}
		nTotalTreeData += nFileCount;

		char szDir[MAX_PATH];
		if (!ReadFile(hFile, &szDir[0], _MAX_PATH, &dwReadBytes, NULL))
		{
			MessageBox(L"Can't read file/directory name", L"KUFTC Data File Manager", MB_OK | MB_ICONERROR);
			CloseHandle(hFile);
			return FALSE;
		}
		datDir->strDir = szDir;
		
		auto pFileItemsRaw = new FILE_ITEM_RAW[nFileCount];
		
		if (!ReadFile(hFile, pFileItemsRaw, sizeof(FILE_ITEM_RAW) * nFileCount, &dwReadBytes, NULL))
		{
			MessageBox(L"Can't read files information", L"KUFTC Data File Manager", MB_OK | MB_ICONERROR);
			CloseHandle(hFile);
			return FALSE;
		}
		datDir->vecFileItems.reserve(nFileCount);
		for (int i = 0; i < nFileCount; ++i)
			datDir->vecFileItems.emplace_back(FILE_ITEM{ datDir->strDir, pFileItemsRaw[i].szFileName, pFileItemsRaw[i].nOffset, pFileItemsRaw[i].nSize });

		delete [] pFileItemsRaw;

		m_vecDirectories.emplace_back(std::move(datDir));
	}
	CloseHandle(hFile);

	m_pTreeData = new TREE_DATA[nTotalTreeData];
	m_pTreeData[0].type = TREE_DATA::ROOT;
	wcscpy_s(m_pTreeData[0].wszDATFileName, m_strDATFileName.c_str());

	m_wndTreeView.Attach(GetDlgItem(IDC_TREE_DAT));

	int nTreeDataIndex = 1;
	auto root = m_wndTreeView.InsertItem(m_strDATFileName.c_str(), TVI_ROOT, TVI_LAST);
	m_wndTreeView.SetItemData(root, (DWORD_PTR)&m_pTreeData[0]);

	for ( auto& dir : m_vecDirectories )
	{
		auto wstrDir = str2wstr(dir->strDir);
		m_pTreeData[nTreeDataIndex].type = TREE_DATA::DIR;
		m_pTreeData[nTreeDataIndex].pDatDir = dir.get();

		
		auto dir_item = m_wndTreeView.InsertItem(wstrDir.c_str(), root, TVI_LAST);
		m_wndTreeView.SetItemData(dir_item, (DWORD_PTR)&m_pTreeData[nTreeDataIndex]);

		++nTreeDataIndex;

		for (auto& file_item : dir->vecFileItems )
		{
			auto wstrFile = str2wstr(file_item.strFileName);
			m_pTreeData[nTreeDataIndex].type = TREE_DATA::FILE;
			m_pTreeData[nTreeDataIndex].pFileItem = &file_item;

			auto file_item = m_wndTreeView.InsertItem(wstrFile.c_str(), dir_item, TVI_LAST);
			m_wndTreeView.SetItemData(file_item, (DWORD_PTR)&m_pTreeData[nTreeDataIndex]);

			++nTreeDataIndex;
		}
	}
	return TRUE;
}

void CKUFTCDatManagerDlg::ExtractAllTo(LPCTSTR szFolder, HANDLE hFile)
{
	for (auto& dir : m_vecDirectories)
		ExtractDirTo(szFolder, hFile, dir.get());
}

void CKUFTCDatManagerDlg::ExtractDirTo(LPCTSTR szFolder, HANDLE hFile, DAT_DIR* pDir)
{
	std::wstring strFolderName = szFolder;
	if (!strFolderName.ends_with(L"\\"))
		strFolderName += L"\\";
	strFolderName += str2wstr(pDir->strDir);

	SHCreateDirectoryEx(NULL, strFolderName.c_str(), NULL);

	for (auto& file_item : pDir->vecFileItems)
		ExtractFileTo(szFolder, hFile, &file_item);
}

void CKUFTCDatManagerDlg::ExtractFileTo(LPCTSTR szFolder, HANDLE hFile, FILE_ITEM* pFile)
{
	wchar_t wszTemp[256];

	std::wstring strFileName = szFolder;
	if (!strFileName.ends_with(L"\\"))
		strFileName += L"\\";
	strFileName += str2wstr(pFile->strFolder) + str2wstr(pFile->strFileName);

	HANDLE hFileDest = CreateFile(strFileName.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, NULL, NULL);
	if (hFileDest == INVALID_HANDLE_VALUE)
	{
		wsprintf(wszTemp, L"Can't create file %s for extracting", strFileName.c_str());
		MessageBox(wszTemp, L"KUFTC Data File Manager", MB_OK | MB_ICONERROR);
		return;
	}

	void* pBuf = new unsigned char[pFile->nSize];

	DWORD dwRead;
	SetFilePointer(hFile, pFile->nOffset, 0, FILE_BEGIN);
	if (!ReadFile(hFile, pBuf, pFile->nSize, &dwRead, NULL))
	{
		wsprintf(wszTemp, L"Can't read from dat file for extracting %s", strFileName.c_str());
		MessageBox(wszTemp, L"KUFTC Data File Manager", MB_OK | MB_ICONERROR);
		CloseHandle(hFileDest);
		return;
	}
	if (!WriteFile(hFileDest, pBuf, pFile->nSize, &dwRead, NULL))
	{
		wsprintf(wszTemp, L"Can't write to %s", strFileName.c_str());
		MessageBox(wszTemp, L"KUFTC Data File Manager", MB_OK | MB_ICONERROR);
	}
	CloseHandle(hFileDest);
	delete[] pBuf;
}

LRESULT CKUFTCDatManagerDlg::OnNMRClick(int idFrom, LPNMHDR pnmh, BOOL&)
{
	POINT pt = { 0, 0 };
	::GetCursorPos(&pt);
	POINT ptClient = pt;
	if (pnmh->hwndFrom != NULL)
		::ScreenToClient(pnmh->hwndFrom, &ptClient);

	if (pnmh->hwndFrom == m_wndTreeView.m_hWnd)
	{
		TVHITTESTINFO tvhti = { 0 };
		tvhti.pt = ptClient;
		m_wndTreeView.HitTest(&tvhti);
		if ((tvhti.flags & TVHT_ONITEMLABEL) != 0)
		{
			HMENU hMenu = LoadMenu(NULL, MAKEINTRESOURCE(IDR_MENU_EXTRACT));
			if (hMenu == NULL)
				return FALSE;

			HMENU hSubMenu = GetSubMenu(hMenu, 0);
			int idCmd = ::TrackPopupMenu(hSubMenu, TPM_LEFTALIGN | TPM_RETURNCMD | TPM_RIGHTBUTTON, pt.x, pt.y, 0, m_wndTreeView.m_hWnd, NULL);

			if (idCmd == IDC_EXTRACT)
			{
				CFolderDialog dlg;

				if (dlg.DoModal() == IDOK)
				{
					HANDLE hFile = CreateFile(m_strDATFileName.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);
					if (hFile == INVALID_HANDLE_VALUE)
					{
						MessageBox(L"Can't open file for extract", L"KUFTC Data File Manager", MB_OK | MB_ICONERROR);
						return FALSE;
					}

					auto pItem = (TREE_DATA*)m_wndTreeView.GetItemData(tvhti.hItem);
					switch (pItem->type)
					{
					case TREE_DATA::ROOT:
						ExtractAllTo(dlg.m_szFolderPath, hFile);
						break;
					case TREE_DATA::DIR:
						ExtractDirTo(dlg.m_szFolderPath, hFile, pItem->pDatDir);
						break;
					case TREE_DATA::FILE:
						{
							std::wstring strFolderName = dlg.m_szFolderPath;
							if (!strFolderName.ends_with(L"\\"))
								strFolderName += L"\\";
							strFolderName += str2wstr(pItem->pFileItem->strFolder);

							SHCreateDirectoryEx(NULL, strFolderName.c_str(), NULL);

							ExtractFileTo(dlg.m_szFolderPath, hFile, pItem->pFileItem);
						}
						break;
					}

					CloseHandle(hFile);
				}
			}
			
		}
	}

	return 0;
}

LRESULT CKUFTCDatManagerDlg::OnClose(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	EndDialog(0);
	return 0;
}