#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <commctrl.h>
#include "resource.h"

const TCHAR ClassName[] = _T("ServiceManager");

HINSTANCE hInstance;
HWND hListView;
HWND hStatus;


void
GetServices ( void )
{
    LV_ITEM item;
    WORD wCodePage;
    WORD wLangID;
    SC_HANDLE ScHandle;
    SC_HANDLE hService;
    DWORD BytesNeeded = 0;
    DWORD ResumeHandle = 0;
    DWORD NumServices = 0;
    DWORD dwHandle, dwLen;
    size_t Index;
    UINT BufLen;
    TCHAR szStatus[128];
    TCHAR* lpData;
    TCHAR* lpBuffer;
    TCHAR szStrFileInfo[80];
    TCHAR FileName[MAX_PATH];
    TCHAR szNumServices[32];
    LPVOID pvData;

    LPSERVICE_FAILURE_ACTIONS pServiceFailureActions = NULL;
    LPQUERY_SERVICE_CONFIG pServiceConfig = NULL;
    ENUM_SERVICE_STATUS_PROCESS *pServiceStatus = NULL;

    ScHandle = OpenSCManager(NULL, NULL, SC_MANAGER_ENUMERATE_SERVICE);
    if (ScHandle != INVALID_HANDLE_VALUE)
    {
        if (EnumServicesStatusEx(ScHandle,
                                 SC_ENUM_PROCESS_INFO,
                                 SERVICE_WIN32,
                                 SERVICE_STATE_ALL,
                                 (LPBYTE)pServiceStatus,
                                 0, &BytesNeeded,
                                 &NumServices,
                                 &ResumeHandle,
                                 0) == 0)
        {
            /* Call function again if required size was returned */
            if (GetLastError() == ERROR_MORE_DATA)
            {
                /* reserve memory for service info array */
                pServiceStatus = (ENUM_SERVICE_STATUS_PROCESS *) HeapAlloc(GetProcessHeap(), 0, BytesNeeded);
                if (pServiceStatus == NULL)
			        return;

                /* fill array with service info */
                if (EnumServicesStatusEx(ScHandle,
                                         SC_ENUM_PROCESS_INFO,
                                         SERVICE_WIN32,
                                         SERVICE_STATE_ALL,
                                         (LPBYTE)pServiceStatus,
                                         BytesNeeded,
                                         &BytesNeeded,
                                         &NumServices,
                                         &ResumeHandle,
                                         0) == 0)
                {
                    HeapFree(GetProcessHeap(), 0, pServiceStatus);
                    return;
                }
            }
            else /* exit on failure */
            {
                return;
            }
        }

        if (NumServices)
        {
            HICON hiconItem;     // icon for list-view items
            HIMAGELIST hSmall;   // image list for other views
            TCHAR buf[40];

            /* Create the icon image lists */
            hSmall = ImageList_Create(GetSystemMetrics(SM_CXSMICON),
                GetSystemMetrics(SM_CYSMICON), ILC_MASK, 1, 1);

            /* Add an icon to each image list */
            hiconItem = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SM_ICON));
            ImageList_AddIcon(hSmall, hiconItem);
            DestroyIcon(hiconItem);

            ListView_SetImageList(hListView, hSmall, LVSIL_SMALL);

            /* set the number of services in the status bar */
            LoadString(hInstance, IDS_SERVICES_NUM_SERVICES, szNumServices, 32);
            _stprintf(buf, szNumServices, NumServices);
            SendMessage(hStatus, SB_SETTEXT, 0, (LPARAM)buf);

            for (Index = 0; Index < NumServices; Index++)
            {
                memset(&item, 0, sizeof(LV_ITEM));
                item.mask = LVIF_TEXT;
                //item.iImage = 0;
                item.pszText = pServiceStatus[Index].lpDisplayName;
                item.iItem = ListView_GetItemCount(hListView);
                item.lParam = 0;
                item.iItem = ListView_InsertItem(hListView, &item);

                BytesNeeded = 0;
                hService = OpenService(ScHandle,
                                       pServiceStatus[Index].lpServiceName,
                                       SC_MANAGER_CONNECT);
                if (hService != INVALID_HANDLE_VALUE)
                {
                    /* check if service is required by the system*/
                    if (!QueryServiceConfig2(hService,
                                             SERVICE_CONFIG_FAILURE_ACTIONS,
                                             (LPBYTE)pServiceFailureActions,
                                             0,
                                             &BytesNeeded))
                    {
                        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
                        {
                            pServiceFailureActions = (LPSERVICE_FAILURE_ACTIONS)
                                HeapAlloc(GetProcessHeap(), 0, BytesNeeded);
                            if (pServiceFailureActions == NULL)
			                    return;

                            if (!QueryServiceConfig2(hService,
                                                     SERVICE_CONFIG_FAILURE_ACTIONS,
                                                     (LPBYTE)pServiceFailureActions,
                                                     BytesNeeded,
                                                     &BytesNeeded))
                            {
                                HeapFree(GetProcessHeap(), 0, pServiceFailureActions);
                                return;
                            }
                        }
                        else /* exit on failure */
                        {
                            return;
                        }
                    }
                    if (pServiceFailureActions->cActions)
                    {
                        if (pServiceFailureActions->lpsaActions[0].Type == SC_ACTION_REBOOT)
                        {
                            LoadString(hInstance, IDS_SERVICES_YES, szStatus, 128);
                            item.pszText = szStatus;
                            item.iSubItem = 1;
                            SendMessage(hListView, LVM_SETITEMTEXT, item.iItem, (LPARAM) &item);
                        }
                    }

					if (pServiceFailureActions != NULL)
					{
						HeapFree(GetProcessHeap(), 0, pServiceFailureActions);
						pServiceFailureActions = NULL;
					}

                    /* get vendor of service binary */
                    BytesNeeded = 0;
                    if (!QueryServiceConfig(hService, pServiceConfig, 0, &BytesNeeded))
                    {
                        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
                        {
                            pServiceConfig = (LPQUERY_SERVICE_CONFIG)
                                HeapAlloc(GetProcessHeap(), 0, BytesNeeded);
                            if (pServiceConfig == NULL)
			                    return;

                            if (!QueryServiceConfig(hService,
                                                    pServiceConfig,
                                                    BytesNeeded,
                                                    &BytesNeeded))
                            {
                                HeapFree(GetProcessHeap(), 0, pServiceConfig);
                                return;
                            }
                        }
                        else /* exit on failure */
                        {
                            return;
                        }
                    }

                    memset(&FileName, 0, MAX_PATH);
                    if (_tcscspn(pServiceConfig->lpBinaryPathName, _T("\"")))
                    {
                        _tcsncpy(FileName, pServiceConfig->lpBinaryPathName,
                            _tcscspn(pServiceConfig->lpBinaryPathName, _T(" ")) );
                    }
                    else
                    {
                        _tcscpy(FileName, pServiceConfig->lpBinaryPathName);
                    }

					HeapFree(GetProcessHeap(), 0, pServiceConfig);
					pServiceConfig = NULL;

					dwLen = GetFileVersionInfoSize(FileName, &dwHandle);
                    if (dwLen)
                    {
                        lpData = (TCHAR*) HeapAlloc(GetProcessHeap(), 0, dwLen);
                        if (lpData == NULL)
			                return;

                        if (!GetFileVersionInfo (FileName, dwHandle, dwLen, lpData)) {
		                    HeapFree(GetProcessHeap(), 0, lpData);
		                    return;
	                    }

                        if (VerQueryValue(lpData, _T("\\VarFileInfo\\Translation"), &pvData, (PUINT) &BufLen))
                        {
                            wCodePage = LOWORD(*(DWORD*) pvData);
                            wLangID = HIWORD(*(DWORD*) pvData);
                            wsprintf(szStrFileInfo, _T("StringFileInfo\\%04X%04X\\CompanyName"), wCodePage, wLangID);
                        }

                        if (VerQueryValue (lpData, szStrFileInfo, (LPVOID) &lpBuffer, (PUINT) &BufLen)) {
                            item.pszText = lpBuffer;
                            item.iSubItem = 2;
                            SendMessage(hListView, LVM_SETITEMTEXT, item.iItem, (LPARAM) &item);
                        }
						HeapFree(GetProcessHeap(), 0, lpData);
                    }
                    else
                    {
                        LoadString(hInstance, IDS_SERVICES_UNKNOWN, szStatus, 128);
                        item.pszText = szStatus;
                        item.iSubItem = 2;
                        SendMessage(hListView, LVM_SETITEMTEXT, item.iItem, (LPARAM) &item);
                    }
                    CloseServiceHandle(hService);
                }

                if (pServiceStatus[Index].ServiceStatusProcess.dwCurrentState == SERVICE_RUNNING)
                {
                    LoadString(hInstance, IDS_SERVICES_STATUS_RUNNING, szStatus, 128);
                    item.pszText = szStatus;
                    item.iSubItem = 2;
                    SendMessage(hListView, LVM_SETITEMTEXT, item.iItem, (LPARAM) &item);
                }
                else
                {
                    item.pszText = '\0';
                    item.iSubItem = 2;
                    SendMessage(hListView, LVM_SETITEMTEXT, item.iItem, (LPARAM) &item);
                }

            }
        }

        HeapFree(GetProcessHeap(), 0, pServiceStatus);
        CloseServiceHandle(ScHandle);
    }


}



LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch(msg)
	{
		case WM_CREATE:
		{
			//HFONT hfDefault;

			HWND hTool;
			TBBUTTON tbb[7];
			TBADDBITMAP tbab;

			int statwidths[] = {110, -1};

			TCHAR szTemp[256];

			/* Create List View */
            LVCOLUMN lvc = { 0 };
            //LVITEM   lv  = { 0 };

            //GetClientRect(hwnd, &rc);
            hListView = CreateWindow(WC_LISTVIEW,
                                     NULL,
                                     WS_CHILD | WS_VISIBLE | LVS_REPORT | WS_BORDER | LVS_EDITLABELS,
                                     0, 0, 0, 0, /* sized via WM_SIZE */
                                     hwnd,
                                     (HMENU) IDC_SERVLIST,
                                     hInstance,
                                     NULL);

            ListView_SetExtendedListViewStyle(hListView, LVS_EX_FULLROWSELECT |
                    /*LVS_EX_GRIDLINES |*/ LVS_EX_HEADERDRAGDROP);

            lvc.mask = LVCF_TEXT | LVCF_SUBITEM | LVCF_WIDTH  | LVCF_FMT;
            lvc.fmt  = LVCFMT_LEFT;

            /* Add columns to the list-view (first column contains check box). */
            lvc.iSubItem = 0;
            lvc.cx       = 160;
            LoadString(hInstance, IDS_FIRSTCOLUMN, szTemp, 256);
            lvc.pszText  = szTemp;
            ListView_InsertColumn(hListView, 0, &lvc);

            lvc.iSubItem = 1;
            lvc.cx       = 150;
            LoadString(hInstance, IDS_SECONDCOLUMN, szTemp, 256);
            lvc.pszText  = szTemp;
            ListView_InsertColumn(hListView, 1, &lvc);

            lvc.iSubItem = 2;
            lvc.cx       = 80;
            LoadString(hInstance, IDS_THIRDCOLUMN, szTemp, 256);
            lvc.pszText  = szTemp;
            ListView_InsertColumn(hListView, 2, &lvc);

            lvc.iSubItem = 3;
            lvc.cx       = 80;
            LoadString(hInstance, IDS_FOURTHCOLUMN, szTemp, 256);
            lvc.pszText  = szTemp;
            ListView_InsertColumn(hListView, 3, &lvc);

            lvc.iSubItem = 4;
            lvc.cx       = 100;
            LoadString(hInstance, IDS_FITHCOLUMN, szTemp, 256);
            lvc.pszText  = szTemp;
            ListView_InsertColumn(hListView, 4, &lvc);

			// Create Toolbar
			hTool = CreateWindowEx(0,
                                   TOOLBARCLASSNAME,
                                   NULL,
                                   WS_CHILD | WS_VISIBLE | TBSTYLE_FLAT | TBSTYLE_TOOLTIPS,
                                   0, 0, 0, 0,
                                   hwnd,
                                   (HMENU)IDC_TOOLBAR,
                                   hInstance,
                                   NULL);
			if(hTool == NULL)
				MessageBox(hwnd, _T("Could not create tool bar."), _T("Error"), MB_OK | MB_ICONERROR);

			// Send the TB_BUTTONSTRUCTSIZE message, which is required for
			// backward compatibility.
			SendMessage(hTool, TB_BUTTONSTRUCTSIZE, (WPARAM)sizeof(TBBUTTON), 0);

			tbab.hInst = HINST_COMMCTRL;
			tbab.nID = IDB_STD_SMALL_COLOR;
			SendMessage(hTool, TB_ADDBITMAP, 0, (LPARAM)&tbab);

            ZeroMemory(tbb, sizeof(tbb));
            tbb[0].iBitmap = STD_PROPERTIES;
            tbb[0].fsState = TBSTATE_ENABLED;
            tbb[0].fsStyle = TBSTYLE_BUTTON;
            tbb[0].idCommand = ID_PROP;
            tbb[1].iBitmap = STD_FILENEW;
            tbb[1].fsState = TBSTATE_ENABLED;
            tbb[1].fsStyle = TBSTYLE_BUTTON;
            tbb[1].idCommand = ID_REFRESH;
            tbb[2].iBitmap = STD_FILENEW;
            tbb[2].fsState = TBSTATE_ENABLED;
            tbb[2].fsStyle = TBSTYLE_BUTTON;
            tbb[2].idCommand = ID_EXPORT;
            /* seperator */
            tbb[3].iBitmap = STD_FILENEW;
            tbb[3].fsState = TBSTATE_ENABLED;
            tbb[3].fsStyle = TBSTYLE_BUTTON;
            tbb[3].idCommand = ID_START;
            tbb[4].iBitmap = STD_FILENEW;
            tbb[4].fsState = TBSTATE_ENABLED;
            tbb[4].fsStyle = TBSTYLE_BUTTON;
            tbb[4].idCommand = ID_STOP;
            tbb[5].iBitmap = STD_FILENEW;
            tbb[5].fsState = TBSTATE_ENABLED;
            tbb[5].fsStyle = TBSTYLE_BUTTON;
            tbb[5].idCommand = ID_PAUSE;
            tbb[6].iBitmap = STD_FILENEW;
            tbb[6].fsState = TBSTATE_ENABLED;
            tbb[6].fsStyle = TBSTYLE_BUTTON;
            tbb[6].idCommand = ID_RESTART;
            SendMessage(hTool, TB_ADDBUTTONS, sizeof(tbb)/sizeof(TBBUTTON), (LPARAM)&tbb);

			/* Create Status bar */
			hStatus = CreateWindowEx(0,
                                     STATUSCLASSNAME,
                                     NULL,
                                     WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
                                     0, 0, 0, 0,
                                     hwnd,
                                     (HMENU)IDC_STATUSBAR,
                                     hInstance,
                                     NULL);
            if(hStatus == NULL)
				MessageBox(hwnd, _T("Could not create status bar."), _T("Error"), MB_OK | MB_ICONERROR);

			SendMessage(hStatus, SB_SETPARTS, sizeof(statwidths)/sizeof(int), (LPARAM)statwidths);

			/* populate the list view with all services */
			GetServices();
		}
		break;
		case WM_SIZE:
		{
			HWND hTool;
			RECT rcTool;
			int iToolHeight;

			HWND hStatus;
			RECT rcStatus;
			int iStatusHeight;

			HWND hListView;
			int lvHeight;
			RECT rcClient;

			/* Size toolbar and get height */
			hTool = GetDlgItem(hwnd, IDC_TOOLBAR);
			SendMessage(hTool, TB_AUTOSIZE, 0, 0);

			GetWindowRect(hTool, &rcTool);
			iToolHeight = rcTool.bottom - rcTool.top;

			/* Size status bar and get height */
			hStatus = GetDlgItem(hwnd, IDC_STATUSBAR);
			SendMessage(hStatus, WM_SIZE, 0, 0);

			GetWindowRect(hStatus, &rcStatus);
			iStatusHeight = rcStatus.bottom - rcStatus.top;

			/* Calculate remaining height and size list view */
			GetClientRect(hwnd, &rcClient);

			lvHeight = rcClient.bottom - iToolHeight - iStatusHeight;

			hListView = GetDlgItem(hwnd, IDC_SERVLIST);
			SetWindowPos(hListView, NULL, 0, iToolHeight, rcClient.right, lvHeight, SWP_NOZORDER);
		}
		break;

		case WM_NOTIFY:
            switch (((LPNMHDR) lParam)->code)
            {
                case TTN_GETDISPINFO:
                {
                    LPTOOLTIPTEXT lpttt;
                    UINT idButton;

                    lpttt = (LPTOOLTIPTEXT) lParam;
                    lpttt->hinst = hInstance;

                    // Specify the resource identifier of the descriptive
                    // text for the given button.
                    idButton = lpttt->hdr.idFrom;
                    switch (idButton)
                    {
                        case ID_PROP:
                            lpttt->lpszText = MAKEINTRESOURCE(IDS_TOOLTIP_PROP);
                        break;

                        case ID_REFRESH:
                            lpttt->lpszText = MAKEINTRESOURCE(IDS_TOOLTIP_REFRESH);
                        break;

                        case ID_EXPORT:
                            lpttt->lpszText = MAKEINTRESOURCE(IDS_TOOLTIP_EXPORT);
                        break;

                        case ID_START:
                            lpttt->lpszText = MAKEINTRESOURCE(IDS_TOOLTIP_START);
                        break;

                        case ID_STOP:
                            lpttt->lpszText = MAKEINTRESOURCE(IDS_TOOLTIP_STOP);
                        break;

                        case ID_PAUSE:
                            lpttt->lpszText = MAKEINTRESOURCE(IDS_TOOLTIP_PAUSE);
                        break;

                        case ID_RESTART:
                            lpttt->lpszText = MAKEINTRESOURCE(IDS_TOOLTIP_RESTART);
                        break;

                    }
                }
                break;

                // Process other notifications here.

                default:
                break;
            }
        break;

		case WM_CLOSE:
			DestroyWindow(hwnd);
		break;
		case WM_DESTROY:
			PostQuitMessage(0);
		break;
		case WM_COMMAND:
			switch(LOWORD(wParam))
			{
				case ID_FILE_EXIT:
					PostMessage(hwnd, WM_CLOSE, 0, 0);
				break;

				case ID_START:
				break;

                case ID_STOP:
                break;

                case ID_PAUSE:
                break;

                case ID_RESUME:
                break;

                case ID_RESTART:
                break;

                case ID_REFRESH:
                break;

                case ID_PROP:
                break;

                case ID_VIEW_CUSTOMIZE:
                break;

                case ID_HELP_ABOUT:
                break;

                case ID_EXPORT:
                break;

			}
		break;
		default:
			return DefWindowProc(hwnd, msg, wParam, lParam);
	}
	return 0;
}



int WINAPI WinMain(HINSTANCE hThisInstance, HINSTANCE hPrevInstance,
	LPSTR lpCmdLine, int nCmdShow)
{
	WNDCLASSEX wc;
	HWND hwnd;
	MSG Msg;
	BOOL bRet;

	hInstance = hThisInstance;

	InitCommonControls();

	wc.cbSize		 = sizeof(WNDCLASSEX);
	wc.style		 = 0;
	wc.lpfnWndProc	 = WndProc;
	wc.cbClsExtra	 = 0;
	wc.cbWndExtra	 = 0;
	wc.hInstance	 = hInstance;
	wc.hIcon		 = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_SM_ICON));
	wc.hCursor		 = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW);
	wc.lpszMenuName  = MAKEINTRESOURCE(IDR_MAINMENU);
	wc.lpszClassName = ClassName;
	wc.hIconSm		 = (HICON)LoadImage(GetModuleHandle(NULL),
                        MAKEINTRESOURCE(IDI_SM_ICON), IMAGE_ICON, 16, 16, 0);

	if(!RegisterClassEx(&wc))
	{
		MessageBox(NULL, _T("Window Registration Failed!"), _T("Error!"),
			MB_ICONEXCLAMATION | MB_OK);
		return 0;
	}

	hwnd = CreateWindowEx(
		0,
		ClassName,
		_T("ReactOS Service Manager"),
		WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
		CW_USEDEFAULT, CW_USEDEFAULT, 700, 500,
		NULL, NULL, hInstance, NULL);

	if(hwnd == NULL)
	{
		MessageBox(NULL, _T("Window Creation Failed!"), _T("Error!"),
			MB_ICONEXCLAMATION | MB_OK);
		return 0;
	}

	ShowWindow(hwnd, nCmdShow);
	UpdateWindow(hwnd);

    while( (bRet = GetMessage( &Msg, NULL, 0, 0 )) != 0)
    {
        if (bRet == -1)
        {
            /* handle the error and possibly exit */
        }
        else
        {
            TranslateMessage(&Msg);
            DispatchMessage(&Msg);
        }
    }
	return Msg.wParam;
}


