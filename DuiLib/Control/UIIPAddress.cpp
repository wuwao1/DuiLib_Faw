#include "StdAfx.h"
#pragma comment( lib, "ws2_32.lib" )

DWORD GetLocalIpAddress () {
	WORD wVersionRequested = MAKEWORD (2, 2);
	WSADATA wsaData;
	if (WSAStartup (wVersionRequested, &wsaData) != 0)
		return 0;
	char local[255] = { 0 };
	gethostname (local, sizeof (local));
	hostent* ph = gethostbyname (local);
	if (ph == nullptr)
		return 0;
	in_addr addr;
	memcpy (&addr, ph->h_addr_list[0], sizeof (in_addr));
	DWORD dwIP = MAKEIPADDRESS (addr.S_un.S_un_b.s_b1, addr.S_un.S_un_b.s_b2, addr.S_un.S_un_b.s_b3, addr.S_un.S_un_b.s_b4);
	return dwIP;
}

namespace DuiLib {
	//CDateTimeUI::m_nDTUpdateFlag
#define IP_NONE   0
#define IP_UPDATE 1
#define IP_DELETE 2
#define IP_KEEP   3

	class CIPAddressWnd: public CWindowWnd {
	public:
		CIPAddressWnd ();

		void Init (CIPAddressUI* pOwner);
		RECT CalPos ();

		string_view_t GetWindowClassName () const;
		string_view_t GetSuperClassName () const;
		void OnFinalMessage (HWND hWnd);

		LRESULT HandleMessage (UINT uMsg, WPARAM wParam, LPARAM lParam);
		LRESULT OnKillFocus (UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

	protected:
		CIPAddressUI	*m_pOwner	= nullptr;
		HBRUSH			m_hBkBrush	= NULL;
		bool			m_bInit		= false;
	};

	CIPAddressWnd::CIPAddressWnd () {}

	void CIPAddressWnd::Init (CIPAddressUI* pOwner) {
		m_pOwner = pOwner;
		m_pOwner->m_nIPUpdateFlag = IP_NONE;

		if (m_hWnd == nullptr) {
			INITCOMMONCONTROLSEX   CommCtrl;
			CommCtrl.dwSize = sizeof (CommCtrl);
			CommCtrl.dwICC = ICC_INTERNET_CLASSES;//ָ��Class
			if (InitCommonControlsEx (&CommCtrl)) {
				RECT rcPos = CalPos ();
				UINT uStyle = WS_CHILD | WS_TABSTOP | WS_GROUP;
				Create (m_pOwner->GetManager ()->GetPaintWindow (), nullptr, uStyle, 0, rcPos);
			}
			SetWindowFont (m_hWnd, m_pOwner->GetManager ()->GetFontInfo (m_pOwner->GetFont ())->hFont, TRUE);
		}

		if (m_pOwner->GetText ().empty ())
			m_pOwner->m_dwIP = GetLocalIpAddress ();
		::SendMessage (m_hWnd, IPM_SETADDRESS, 0, m_pOwner->m_dwIP);
		::ShowWindow (m_hWnd, SW_SHOW);
		::SetFocus (m_hWnd);

		m_bInit = true;
	}

	RECT CIPAddressWnd::CalPos () {
		return m_pOwner->GetPos ();
	}

	string_view_t CIPAddressWnd::GetWindowClassName () const {
		return _T ("IPAddressWnd");
	}

	string_view_t CIPAddressWnd::GetSuperClassName () const {
		return WC_IPADDRESS;
	}

	void CIPAddressWnd::OnFinalMessage (HWND /*hWnd*/) {
		// Clear reference and die
		if (m_hBkBrush != nullptr) ::DeleteObject (m_hBkBrush);
		m_pOwner->m_pWindow = nullptr;
		delete this;
	}

	LRESULT CIPAddressWnd::HandleMessage (UINT uMsg, WPARAM wParam, LPARAM lParam) {
		LRESULT lRes = 0;
		BOOL bHandled = TRUE;
		if (uMsg == WM_KILLFOCUS) {
			bHandled = TRUE;
			return 0;
			lRes = OnKillFocus (uMsg, wParam, lParam, bHandled);
		} else if (uMsg == WM_KEYUP && (wParam == VK_DELETE || wParam == VK_BACK)) {
			lRes = ::DefWindowProc (m_hWnd, uMsg, wParam, lParam);
			m_pOwner->m_nIPUpdateFlag = IP_DELETE;
			m_pOwner->UpdateText ();
			PostMessage (WM_CLOSE);
			return lRes;
		} else if (uMsg == WM_KEYUP && wParam == VK_ESCAPE) {
			lRes = ::DefWindowProc (m_hWnd, uMsg, wParam, lParam);
			m_pOwner->m_nIPUpdateFlag = IP_KEEP;
			PostMessage (WM_CLOSE);
			return lRes;
		} else if (uMsg == OCM_COMMAND) {
			if (GET_WM_COMMAND_CMD (wParam, lParam) == EN_KILLFOCUS) {
				lRes = OnKillFocus (uMsg, wParam, lParam, bHandled);
			}
		} else bHandled = FALSE;
		if (!bHandled) return CWindowWnd::HandleMessage (uMsg, wParam, lParam);
		return lRes;
	}

	LRESULT CIPAddressWnd::OnKillFocus (UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
		HWND hWndFocus = GetFocus ();
		while (hWndFocus) {
			if (GetFocus () == m_hWnd) {
				bHandled = TRUE;
				return 0;
			}
			hWndFocus = GetParent (hWndFocus);
		}

		LRESULT lRes = ::DefWindowProc (m_hWnd, uMsg, wParam, lParam);
		if (m_pOwner->m_nIPUpdateFlag == IP_NONE) {
			::SendMessage (m_hWnd, IPM_GETADDRESS, 0, (LPARAM) &m_pOwner->m_dwIP);
			m_pOwner->m_nIPUpdateFlag = IP_UPDATE;
			m_pOwner->UpdateText ();
		}
		::ShowWindow (m_hWnd, SW_HIDE);
		return lRes;
	}

	//////////////////////////////////////////////////////////////////////////
	//
	IMPLEMENT_DUICONTROL (CIPAddressUI)

	CIPAddressUI::CIPAddressUI () {
		m_dwIP = GetLocalIpAddress ();
		m_nIPUpdateFlag = IP_UPDATE;
		UpdateText ();
		m_nIPUpdateFlag = IP_NONE;
	}

	string_view_t CIPAddressUI::GetClass () const {
		return _T ("DateTimeUI");
	}

	LPVOID CIPAddressUI::GetInterface (string_view_t pstrName) {
		if (pstrName == DUI_CTRL_IPADDRESS) return static_cast<CIPAddressUI*>(this);
		return CLabelUI::GetInterface (pstrName);
	}

	DWORD CIPAddressUI::GetIP () {
		return m_dwIP;
	}

	void CIPAddressUI::SetIP (DWORD dwIP) {
		m_dwIP = dwIP;
		UpdateText ();
	}

	void CIPAddressUI::SetReadOnly (bool bReadOnly) {
		m_bReadOnly = bReadOnly;
		Invalidate ();
	}

	bool CIPAddressUI::IsReadOnly () const {
		return m_bReadOnly;
	}

	void CIPAddressUI::UpdateText () {
		if (m_nIPUpdateFlag == IP_DELETE)
			SetText (_T (""));
		else if (m_nIPUpdateFlag == IP_UPDATE) {
			TCHAR szIP[MAX_PATH] = { 0 };
			in_addr addr;
			addr.S_un.S_addr = m_dwIP;
			_stprintf (szIP, _T ("%d.%d.%d.%d"), addr.S_un.S_un_b.s_b4, addr.S_un.S_un_b.s_b3, addr.S_un.S_un_b.s_b2, addr.S_un.S_un_b.s_b1);
			SetText (szIP);
		}
	}

	void CIPAddressUI::DoEvent (TEventUI& event) {
		if (!IsMouseEnabled () && event.Type > UIEVENT__MOUSEBEGIN && event.Type < UIEVENT__MOUSEEND) {
			if (m_pParent != nullptr) m_pParent->DoEvent (event);
			else CLabelUI::DoEvent (event);
			return;
		} else if (event.Type == UIEVENT_SETCURSOR && IsEnabled ()) {
			::SetCursor (::LoadCursor (nullptr, IDC_IBEAM));
			return;
		} else if (event.Type == UIEVENT_WINDOWSIZE) {
			if (m_pWindow != nullptr) m_pManager->SetFocusNeeded (this);
		} else if (event.Type == UIEVENT_SCROLLWHEEL) {
			if (m_pWindow != nullptr) return;
		} else if (event.Type == UIEVENT_SETFOCUS && IsEnabled ()) {
			if (m_pWindow) {
				return;
			}
			m_pWindow = new CIPAddressWnd ();
			ASSERT (m_pWindow);
			m_pWindow->Init (this);
			m_pWindow->ShowWindow ();
		} else if (event.Type == UIEVENT_KILLFOCUS && IsEnabled ()) {
			Invalidate ();
		} else if (event.Type == UIEVENT_BUTTONDOWN || event.Type == UIEVENT_DBLCLICK || event.Type == UIEVENT_RBUTTONDOWN) {
			if (IsEnabled ()) {
				GetManager ()->ReleaseCapture ();
				if (IsFocused () && m_pWindow == nullptr) {
					m_pWindow = new CIPAddressWnd ();
					ASSERT (m_pWindow);
				}
				if (m_pWindow != nullptr) {
					m_pWindow->Init (this);
					m_pWindow->ShowWindow ();
				}
			}
			return;
		} else if (event.Type == UIEVENT_MOUSEMOVE || event.Type == UIEVENT_BUTTONUP || event.Type == UIEVENT_CONTEXTMENU || event.Type == UIEVENT_MOUSEENTER || event.Type == UIEVENT_MOUSELEAVE) {
			return;
		}

		CLabelUI::DoEvent (event);
	}

	void CIPAddressUI::SetAttribute (string_view_t pstrName, string_view_t pstrValue) {
		CLabelUI::SetAttribute (pstrName, pstrValue);
	}
}
