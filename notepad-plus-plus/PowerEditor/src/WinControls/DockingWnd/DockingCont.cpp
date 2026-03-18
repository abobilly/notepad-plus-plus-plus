// This file is part of Notepad++ project
// Copyright (C)2006 Jens Lorenz <jens.plugin.npp@gmx.de>

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// at your option any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include "dockingResource.h"
#include "DockingCont.h"

#include "SplitterContainer.h"
#include "ToolTip.h"
#include "Parameters.h"
#include "localization.h"

using namespace std;

#ifndef WH_MOUSE_LL
#define WH_MOUSE_LL 14
#endif

static HWND hWndServer = NULL;
static HHOOK hookMouse = NULL;

namespace
{
	constexpr int kDockingAccentThickness = 3;
	constexpr int kDockingCaptionTextPadding = 8;
	constexpr int kDockingCloseButtonCornerRadius = 4;
	constexpr COLORREF kDockingLightCaptionActive = RGB(246, 249, 252);
	constexpr COLORREF kDockingLightCaptionInactive = RGB(232, 237, 244);
	constexpr COLORREF kDockingLightTabActive = RGB(248, 250, 253);
	constexpr COLORREF kDockingLightTabInactive = RGB(236, 240, 246);
	constexpr COLORREF kDockingLightText = RGB(34, 39, 46);
	constexpr COLORREF kDockingLightMutedText = RGB(93, 101, 112);
	constexpr COLORREF kDockingLightEdge = RGB(191, 198, 208);
	constexpr COLORREF kDockingLightHotBg = RGB(224, 230, 239);
	constexpr COLORREF kDockingLightPressedBg = RGB(214, 221, 232);

	LOGFONT createDockingFont(HWND hwnd, DPIManagerV2::FontType fontType, LONG weight)
	{
		LOGFONT lf{DPIManagerV2::getDefaultGUIFontForDpi(hwnd, fontType)};
		lf.lfQuality = CLEARTYPE_NATURAL_QUALITY;
		lf.lfWeight = weight;
		return lf;
	}

	RECT getDockingCloseButtonRect(const RECT &rcItem, bool isTopCaption, int closeButtonWidth, int closeButtonHeight, int closeButtonPosLeftDynamic, int closeButtonPosTopDynamic)
	{
		RECT buttonRect{};
		if (isTopCaption)
		{
			buttonRect.left = rcItem.right - closeButtonWidth - closeButtonPosLeftDynamic;
			buttonRect.top = rcItem.top + closeButtonPosTopDynamic;
		}
		else
		{
			buttonRect.left = rcItem.left + closeButtonPosLeftDynamic;
			buttonRect.top = rcItem.top + closeButtonPosTopDynamic;
		}

		buttonRect.right = buttonRect.left + closeButtonWidth;
		buttonRect.bottom = buttonRect.top + closeButtonHeight;
		return buttonRect;
	}
}

static LRESULT CALLBACK hookProcMouse(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode >= 0)
	{
		switch (wParam)
		{
		case WM_MOUSEMOVE:
		case WM_NCMOUSEMOVE:
			::PostMessage(hWndServer, UINT(wParam), 0, 0);
			break;

		case WM_LBUTTONUP:
		case WM_NCLBUTTONUP:
			::PostMessage(hWndServer, UINT(wParam), 0, 0);
			break;

		default:
			break;
		}
	}

	return ::CallNextHookEx(hookMouse, nCode, wParam, lParam);
}

DockingCont::DockingCont()
{
	setDpi();
	_captionHeightDynamic = _dpiManager.scale(HIGH_CAPTION);
	_captionGapDynamic = _dpiManager.scale(CAPTION_GAP);
	_closeButtonPosLeftDynamic = _dpiManager.scale(CLOSEBTN_POS_LEFT);
	_closeButtonPosTopDynamic = _dpiManager.scale(CLOSEBTN_POS_TOP);

	_closeButtonWidth = _dpiManager.scale(g_dockingContCloseBtnSize);
	_closeButtonHeight = _dpiManager.scale(g_dockingContCloseBtnSize);
}

DockingCont::~DockingCont()
{
	destroyFonts();
}

void DockingCont::doDialog(bool willBeShown, bool isFloating)
{
	if (!isCreated())
	{
		NativeLangSpeaker *pNativeSpeaker = NppParameters::getInstance().getNativeLangSpeaker();
		create(IDD_CONTAINER_DLG, pNativeSpeaker->isRTL(), true, 0);

		_isFloating = isFloating;

		if (_isFloating)
		{
			::SetWindowLongPtr(_hSelf, GWL_STYLE, POPUP_STYLES);
			::SetWindowLongPtr(_hSelf, GWL_EXSTYLE, pNativeSpeaker->isRTL() ? POPUP_EXSTYLES | WS_EX_LAYOUTRTL : POPUP_EXSTYLES);
			::ShowWindow(_hCaption, SW_HIDE);
		}
		else
		{
			::SetWindowLongPtr(_hSelf, GWL_STYLE, CHILD_STYLES);
			::SetWindowLongPtr(_hSelf, GWL_EXSTYLE, CHILD_EXSTYLES);
			::ShowWindow(_hCaption, SW_SHOW);
		}

		// If you want default GUI font
		LOGFONT lfTab{createDockingFont(_hParent, DPIManagerV2::FontType::regular, FW_MEDIUM)};
		_hFont = ::CreateFontIndirect(&lfTab);

		LOGFONT lfCaption{createDockingFont(_hParent, DPIManagerV2::FontType::smcaption, FW_SEMIBOLD)};
		_hFontCaption = ::CreateFontIndirect(&lfCaption);
	}

	display(willBeShown);
}

tTbData *DockingCont::createToolbar(const tTbData &data)
{
	tTbData *pTbData = new tTbData;

	*pTbData = data;

	// force window style of client window
	::SetWindowLongPtr(pTbData->hClient, GWL_STYLE, CHILD_STYLES);
	::SetWindowLongPtr(pTbData->hClient, GWL_EXSTYLE, CHILD_EXSTYLES);

	// restore position if plugin is in floating state
	if ((_isFloating) && (::SendMessage(_hContTab, TCM_GETITEMCOUNT, 0, 0) == 0))
	{
		reSizeToWH(pTbData->rcFloat);
	}

	// set attached child window
	::SetParent(pTbData->hClient, ::GetDlgItem(_hSelf, IDC_CLIENT_TAB));

	// set names for captions and view toolbar
	viewToolbar(pTbData);

	// attach to list
	_vTbData.push_back(pTbData);

	return pTbData;
}

void DockingCont::removeToolbar(const tTbData &data)
{
	// remove from list
	// items in _vTbData are removed in the loop so _vTbData.size() should be checked in every iteration
	for (size_t iTb = 0; iTb < _vTbData.size(); ++iTb)
	{
		if (_vTbData[iTb]->hClient == data.hClient)
		{
			// remove tab
			removeTab(_vTbData[iTb]);

			// free resources
			delete _vTbData[iTb];
			vector<tTbData *>::iterator itr = _vTbData.begin() + iTb;
			_vTbData.erase(itr);
		}
	}
}

tTbData *DockingCont::findToolbarByWnd(HWND hClient)
{
	auto matchesWnd = [hClient](const tTbData *pTb) -> bool
	{
		return pTb->hClient == hClient;
	};

	auto it = std::find_if(_vTbData.begin(), _vTbData.end(), matchesWnd);

	return (it != _vTbData.end()) ? *it : nullptr;
}

tTbData *DockingCont::findToolbarByName(wchar_t *pszName)
{
	tTbData *pTbData = NULL;

	// find entry by handle
	for (size_t iTb = 0, len = _vTbData.size(); iTb < len; ++iTb)
	{
		if (lstrcmp(pszName, _vTbData[iTb]->pszName) == 0)
		{
			pTbData = _vTbData[iTb];
		}
	}
	return pTbData;
}

void DockingCont::setActiveTb(tTbData *pTbData)
{
	int iItem = searchPosInTab(pTbData);
	setActiveTb(iItem);
}

void DockingCont::setActiveTb(int iItem)
{
	if (iItem < ::SendMessage(_hContTab, TCM_GETITEMCOUNT, 0, 0))
	{
		selectTab(iItem);
	}
}

int DockingCont::getActiveTb()
{
	return static_cast<int32_t>(::SendMessage(_hContTab, TCM_GETCURSEL, 0, 0));
}

tTbData *DockingCont::getDataOfActiveTb()
{
	tTbData *pTbData = NULL;
	int iItem = getActiveTb();

	if (iItem != -1)
	{
		TCITEM tcItem{};

		tcItem.mask = TCIF_PARAM;
		::SendMessage(_hContTab, TCM_GETITEM, iItem, reinterpret_cast<LPARAM>(&tcItem));
		pTbData = (tTbData *)tcItem.lParam;
	}

	return pTbData;
}

vector<tTbData *> DockingCont::getDataOfVisTb()
{
	vector<tTbData *> vTbData;
	TCITEM tcItem{};
	int iItemCnt = static_cast<int32_t>(::SendMessage(_hContTab, TCM_GETITEMCOUNT, 0, 0));

	tcItem.mask = TCIF_PARAM;

	for (int iItem = 0; iItem < iItemCnt; ++iItem)
	{
		::SendMessage(_hContTab, TCM_GETITEM, iItem, reinterpret_cast<LPARAM>(&tcItem));
		vTbData.push_back((tTbData *)tcItem.lParam);
	}
	return vTbData;
}

bool DockingCont::isTbVis(tTbData *data)
{
	TCITEM tcItem{};
	int iItemCnt = static_cast<int32_t>(::SendMessage(_hContTab, TCM_GETITEMCOUNT, 0, 0));

	tcItem.mask = TCIF_PARAM;

	for (int iItem = 0; iItem < iItemCnt; ++iItem)
	{
		::SendMessage(_hContTab, TCM_GETITEM, iItem, reinterpret_cast<LPARAM>(&tcItem));
		if (!tcItem.lParam)
			return false;
		if (((tTbData *)tcItem.lParam) == data)
			return true;
	}
	return false;
}

void DockingCont::destroyFonts()
{
	if (_hFont != nullptr)
	{
		::DeleteObject(_hFont);
		_hFont = nullptr;
	}

	if (_hFontCaption != nullptr)
	{
		::DeleteObject(_hFontCaption);
		_hFontCaption = nullptr;
	}
}

//
//    Process function of caption bar
//
LRESULT DockingCont::runProcCaption(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	static ToolTip toolTip;

	switch (Message)
	{
	case WM_ERASEBKGND:
	{
		if (!NppDarkMode::isEnabled())
		{
			break;
		}

		RECT rc{};
		::GetClientRect(hwnd, &rc);
		::FillRect(reinterpret_cast<HDC>(wParam), &rc, NppDarkMode::getDlgBackgroundBrush());
		return TRUE;
	}

	case WM_LBUTTONDOWN:
	{
		_isMouseDown = TRUE;

		if (isInRect(hwnd, LOWORD(lParam), HIWORD(lParam)) == posClose)
		{
			_isMouseClose = TRUE;
			_isMouseOver = TRUE;

			// start hooking
			hWndServer = _hCaption;
			hookMouse = ::SetWindowsHookEx(WH_MOUSE_LL, hookProcMouse, _hInst, 0);

			if (!hookMouse)
			{
				DWORD dwError = ::GetLastError();
				wchar_t str[128]{};
				::wsprintf(str, L"GetLastError() returned %lu", dwError);
				::MessageBox(NULL, str, L"SetWindowsHookEx(MOUSE) failed on runProcCaption", MB_OK | MB_ICONERROR);
			}
			::RedrawWindow(hwnd, nullptr, nullptr, RDW_INVALIDATE);
		}

		focusClient();
		return 0;
	}
	case WM_LBUTTONUP:
	{
		_isMouseDown = FALSE;
		if (_isMouseClose == TRUE)
		{
			// end hooking
			::UnhookWindowsHookEx(hookMouse);

			if (_isMouseOver == TRUE)
			{
				doClose(GetKeyState(VK_SHIFT) < 0);
			}
			_isMouseClose = FALSE;
			_isMouseOver = FALSE;
		}

		focusClient();
		return 0;
	}
	case WM_LBUTTONDBLCLK:
	{
		if (isInRect(hwnd, LOWORD(lParam), HIWORD(lParam)) == posCaption)
			::SendMessage(_hParent, DMM_FLOATALL, 0, reinterpret_cast<LPARAM>(this));

		focusClient();
		return 0;
	}
	case WM_MOUSEMOVE:
	{
		POINT pt{};

		// get correct cursor position
		::GetCursorPos(&pt);
		::ScreenToClient(_hCaption, &pt);

		if (_isMouseDown == TRUE)
		{
			if (_isMouseClose == FALSE)
			{
				// keep sure that button is still down and within caption
				if ((wParam == MK_LBUTTON) && (isInRect(hwnd, pt.x, pt.y) == posCaption))
				{
					_dragFromTab = FALSE;
					NotifyParent(DMM_MOVE);
					_isMouseDown = FALSE;
				}
				else
				{
					_isMouseDown = FALSE;
				}
			}
			else
			{
				BOOL isMouseOver = _isMouseOver;
				_isMouseOver = (isInRect(hwnd, pt.x, pt.y) == posClose ? TRUE : FALSE);

				// if state is changed draw new
				if (_isMouseOver != isMouseOver)
				{
					::SetFocus(NULL);
					::RedrawWindow(hwnd, nullptr, nullptr, RDW_INVALIDATE);
				}
			}
		}
		else if (_bCapTTHover == FALSE)
		{
			_hoverMPos = isInRect(hwnd, LOWORD(lParam), HIWORD(lParam));

			if ((_bCaptionTT == TRUE) || (_hoverMPos == posClose))
			{
				TRACKMOUSEEVENT tme{};
				tme.cbSize = sizeof(tme);
				tme.hwndTrack = hwnd;
				tme.dwFlags = TME_LEAVE | TME_HOVER;
				tme.dwHoverTime = 1000;
				_bCapTTHover = _TrackMouseEvent(&tme);
			}
		}
		else if ((_bCapTTHover == TRUE) &&
				 (_hoverMPos != isInRect(hwnd, LOWORD(lParam), HIWORD(lParam))))
		{
			toolTip.destroy();
			_bCapTTHover = FALSE;
		}
		return 0;
	}
	case WM_MOUSEHOVER:
	{
		RECT rc{};
		POINT pt{};

		// get mouse position
		::GetCursorPos(&pt);

		toolTip.init(_hInst, hwnd);
		if (_hoverMPos == posCaption)
		{
			toolTip.Show(rc, _pszCaption.c_str(), pt.x, pt.y + 20);
		}
		else
		{
			NativeLangSpeaker *pNativeSpeaker = (NppParameters::getInstance()).getNativeLangSpeaker();
			wstring tip = pNativeSpeaker->getLocalizedStrFromID("close-panel-tip", L"Close");
			toolTip.Show(rc, tip.c_str(), pt.x, pt.y + 20);
		}
		return 0;
	}
	case WM_MOUSELEAVE:
	{
		toolTip.destroy();
		_bCapTTHover = FALSE;
		return 0;
	}
	case WM_SIZE:
	{
		::GetWindowRect(hwnd, &_rcCaption);
		ScreenRectToClientRect(hwnd, &_rcCaption);
		break;
	}
	case WM_SETTEXT:
	{
		::RedrawWindow(hwnd, nullptr, nullptr, RDW_INVALIDATE);
		return TRUE;
	}
	default:
		break;
	}

	return ::DefSubclassProc(hwnd, Message, wParam, lParam);
}

LRESULT DockingCont::DockingCaptionSubclass(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	switch (uMsg)
	{
	case WM_NCDESTROY:
	{
		::RemoveWindowSubclass(hWnd, DockingCaptionSubclass, uIdSubclass);
		break;
	}

	default:
		break;
	}
	return reinterpret_cast<DockingCont *>(dwRefData)->runProcCaption(hWnd, uMsg, wParam, lParam);
}

void DockingCont::drawCaptionItem(DRAWITEMSTRUCT *pDrawItemStruct)
{
	RECT rc = pDrawItemStruct->rcItem;
	HDC hDc = pDrawItemStruct->hDC;
	const int length = static_cast<int32_t>(_pszCaption.length());
	const bool isDarkMode = NppDarkMode::isEnabled();
	const int accentThickness = _dpiManager.scale(kDockingAccentThickness);
	const int textPadding = _dpiManager.scale(kDockingCaptionTextPadding);
	const COLORREF fillColor = isDarkMode ? (_isActive ? NppDarkMode::getCtrlBackgroundColor() : NppDarkMode::getBackgroundColor()) : (_isActive ? kDockingLightCaptionActive : kDockingLightCaptionInactive);
	const COLORREF textColor = isDarkMode ? (_isActive ? NppDarkMode::getTextColor() : NppDarkMode::getDarkerTextColor()) : (_isActive ? kDockingLightText : kDockingLightMutedText);
	const COLORREF edgeColor = isDarkMode ? NppDarkMode::getEdgeColor() : kDockingLightEdge;
	const COLORREF accentColor = isDarkMode ? NppDarkMode::getAccentColor() : RGB(84, 122, 181);
	RECT closeButtonRect = getDockingCloseButtonRect(rc, _isTopCaption == TRUE, _closeButtonWidth, _closeButtonHeight, _closeButtonPosLeftDynamic, _closeButtonPosTopDynamic);

	const int nSavedDC = ::SaveDC(hDc);
	::SetBkMode(hDc, TRANSPARENT);

	HBRUSH bgbrush = ::CreateSolidBrush(fillColor);
	HPEN edgePen = ::CreatePen(PS_SOLID, 1, edgeColor);
	auto holdPen = static_cast<HPEN>(::SelectObject(hDc, edgePen));
	::FillRect(hDc, &rc, bgbrush);
	::SetTextColor(hDc, textColor);

	if (_isActive == TRUE)
	{
		RECT accentRect = rc;
		if (_isTopCaption == TRUE)
		{
			accentRect.bottom = accentRect.top + accentThickness;
		}
		else
		{
			accentRect.right = accentRect.left + accentThickness;
		}

		HBRUSH accentBrush = ::CreateSolidBrush(accentColor);
		::FillRect(hDc, &accentRect, accentBrush);
		::DeleteObject(accentBrush);
	}

	if (_isTopCaption == TRUE)
	{
		POINT bottomBorder[] = {
			{rc.left, rc.bottom - 1},
			{rc.right, rc.bottom - 1}};
		::Polyline(hDc, bottomBorder, _countof(bottomBorder));

		RECT textRect = rc;
		textRect.left += textPadding;
		textRect.top += _dpiManager.scale(1);
		textRect.right = closeButtonRect.left - textPadding;

		auto hOldFont = static_cast<HFONT>(::SelectObject(hDc, _hFontCaption));
		::DrawText(hDc, _pszCaption.c_str(), length, &textRect, DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS | DT_NOPREFIX);

		SIZE size{};
		GetTextExtentPoint32(hDc, _pszCaption.c_str(), length, &size);
		_bCaptionTT = (((textRect.right - textRect.left) < size.cx) ? TRUE : FALSE);

		::SelectObject(hDc, hOldFont);
	}
	else
	{
		POINT rightBorder[] = {
			{rc.right - 1, rc.top},
			{rc.right - 1, rc.bottom}};
		::Polyline(hDc, rightBorder, _countof(rightBorder));

		RECT textRect = rc;
		textRect.left += _dpiManager.scale(2);
		textRect.top += _captionHeightDynamic;
		textRect.right = textRect.bottom - textRect.top;
		textRect.bottom += _dpiManager.scale(14);

		LOGFONT lf{createDockingFont(_hParent, DPIManagerV2::FontType::smcaption, FW_SEMIBOLD)};
		lf.lfEscapement = 900;
		lf.lfOrientation = 900;
		HFONT hFont = ::CreateFontIndirect(&lf);
		if (hFont == nullptr)
		{
			hFont = ::CreateFont(12, 0, 90 * 10, 0,
								 FW_SEMIBOLD, FALSE, FALSE, FALSE,
								 ANSI_CHARSET, OUT_DEFAULT_PRECIS,
								 CLIP_DEFAULT_PRECIS, CLEARTYPE_NATURAL_QUALITY,
								 DEFAULT_PITCH | FF_ROMAN,
								 L"Segoe UI");
		}

		auto hOldFont = static_cast<HFONT>(::SelectObject(hDc, hFont));
		::DrawText(hDc, _pszCaption.c_str(), length, &textRect, DT_BOTTOM | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);

		SIZE size{};
		GetTextExtentPoint32(hDc, _pszCaption.c_str(), length, &size);
		_bCaptionTT = (((textRect.bottom - textRect.top) < size.cy) ? TRUE : FALSE);

		::SelectObject(hDc, hOldFont);
		::DeleteObject(hFont);
	}

	if (_hFont == nullptr)
	{
		LOGFONT lf{createDockingFont(_hParent, DPIManagerV2::FontType::regular, FW_MEDIUM)};
		_hFont = ::CreateFontIndirect(&lf);
	}

	const bool isHot = (_isMouseOver == TRUE);
	const bool isPressed = isHot && (_isMouseDown == TRUE);
	if (isHot)
	{
		if (isDarkMode)
		{
			NppDarkMode::paintRoundRect(hDc, closeButtonRect, isPressed ? NppDarkMode::getEdgePen() : NppDarkMode::getHotEdgePen(), isPressed ? NppDarkMode::getCtrlBackgroundBrush() : NppDarkMode::getHotBackgroundBrush(), kDockingCloseButtonCornerRadius, kDockingCloseButtonCornerRadius);
		}
		else
		{
			HBRUSH buttonBrush = ::CreateSolidBrush(isPressed ? kDockingLightPressedBg : kDockingLightHotBg);
			HPEN buttonPen = ::CreatePen(PS_SOLID, 1, edgeColor);
			auto holdButtonBrush = static_cast<HBRUSH>(::SelectObject(hDc, buttonBrush));
			auto holdButtonPen = static_cast<HPEN>(::SelectObject(hDc, buttonPen));
			::RoundRect(hDc, closeButtonRect.left, closeButtonRect.top, closeButtonRect.right, closeButtonRect.bottom, kDockingCloseButtonCornerRadius, kDockingCloseButtonCornerRadius);
			::SelectObject(hDc, holdButtonBrush);
			::SelectObject(hDc, holdButtonPen);
			::DeleteObject(buttonBrush);
			::DeleteObject(buttonPen);
		}
	}

	auto hOld = static_cast<HFONT>(::SelectObject(hDc, _hFont));
	::SetTextColor(hDc, isPressed ? (isDarkMode ? NppDarkMode::getTextColor() : kDockingLightText) : textColor);
	closeButtonRect.top -= _dpiManager.scale(1);
	::DrawText(hDc, L"×", 1, &closeButtonRect, DT_VCENTER | DT_CENTER | DT_SINGLELINE);
	::SelectObject(hDc, hOld);

	::SelectObject(hDc, holdPen);
	::DeleteObject(edgePen);
	::DeleteObject(bgbrush);
	::RestoreDC(hDc, nSavedDC);
}

eMousePos DockingCont::isInRect(HWND hwnd, int x, int y)
{
	RECT rc{};
	eMousePos ret = posOutside;

	::GetWindowRect(hwnd, &rc);
	::MapWindowPoints(NULL, hwnd, (LPPOINT)&rc, 2);

	if (_isTopCaption == TRUE)
	{
		if ((x > rc.left) && (x < rc.right - _captionHeightDynamic) && (y > rc.top) && (y < rc.bottom))
		{
			ret = posCaption;
		}
		else if ((x > rc.right - (_closeButtonWidth + _closeButtonPosLeftDynamic)) && (x < (rc.right - _closeButtonPosLeftDynamic)) &&
				 (y > (rc.top + _closeButtonPosTopDynamic)) && (y < (rc.bottom - _closeButtonPosTopDynamic)))
		{
			ret = posClose;
		}
	}
	else
	{
		if ((x > rc.left) && (x < rc.right) && (y > rc.top + _captionHeightDynamic) && (y < rc.bottom))
		{
			ret = posCaption;
		}
		else if ((x > rc.left + _closeButtonPosLeftDynamic) && (x < rc.right - _closeButtonPosLeftDynamic) &&
				 (y > (rc.top + _closeButtonPosTopDynamic)) && (y < (rc.top + (_closeButtonHeight + _closeButtonPosLeftDynamic))))
		{
			ret = posClose;
		}
	}

	return ret;
}

//----------------------------------------------------------
//    Process function of tab
//
LRESULT DockingCont::runProcTab(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	static ToolTip toolTip;

	switch (Message)
	{
	case WM_ERASEBKGND:
	{
		if (!NppDarkMode::isEnabled())
		{
			break;
		}

		RECT rc{};
		::GetClientRect(hwnd, &rc);
		::FillRect(reinterpret_cast<HDC>(wParam), &rc, NppDarkMode::getCtrlBackgroundBrush());
		return TRUE;
	}

	case WM_PAINT:
	{
		if (!NppDarkMode::isEnabled())
		{
			break;
		}

		LONG_PTR dwStyle = GetWindowLongPtr(hwnd, GWL_STYLE);
		if (!(dwStyle & TCS_OWNERDRAWFIXED))
		{
			break;
		}

		PAINTSTRUCT ps{};
		HDC hdc = ::BeginPaint(hwnd, &ps);
		::FillRect(hdc, &ps.rcPaint, NppDarkMode::getCtrlBackgroundBrush());

		UINT id = ::GetDlgCtrlID(hwnd);

		auto holdPen = static_cast<HPEN>(::SelectObject(hdc, NppDarkMode::getEdgePen()));

		HRGN holdClip = CreateRectRgn(0, 0, 0, 0);
		if (1 != GetClipRgn(hdc, holdClip))
		{
			DeleteObject(holdClip);
			holdClip = nullptr;
		}

		int nTabs = TabCtrl_GetItemCount(hwnd);
		int nFocusTab = TabCtrl_GetCurFocus(hwnd);
		int nSelTab = TabCtrl_GetCurSel(hwnd);
		for (int i = 0; i < nTabs; ++i)
		{
			DRAWITEMSTRUCT dis{};
			dis.CtlType = ODT_TAB;
			dis.CtlID = id;
			dis.itemID = static_cast<UINT>(i);
			dis.itemAction = ODA_DRAWENTIRE;
			dis.itemState = ODS_DEFAULT;
			dis.hwndItem = hwnd;
			dis.hDC = hdc;

			TabCtrl_GetItemRect(hwnd, i, &dis.rcItem);

			if (i == nFocusTab)
			{
				dis.itemState |= ODS_FOCUS;
			}

			if (i == nSelTab)
			{
				dis.itemState |= ODS_SELECTED;
			}

			dis.itemState |= ODS_NOFOCUSRECT; // maybe, does it handle it already?

			RECT rcIntersect{};
			if (IntersectRect(&rcIntersect, &ps.rcPaint, &dis.rcItem))
			{
				dis.rcItem.right -= 1;
				::OffsetRect(&dis.rcItem, 0, _dpiManager.scale(CAPTION_GAP * 2));
				if (i == 0)
				{
					POINT edges[] = {
						{dis.rcItem.left - 1, dis.rcItem.top},
						{dis.rcItem.left - 1, dis.rcItem.bottom}};
					Polyline(hdc, edges, _countof(edges));
				}

				{
					POINT edges[] = {
						{dis.rcItem.right, dis.rcItem.top},
						{dis.rcItem.right, dis.rcItem.bottom}};
					Polyline(hdc, edges, _countof(edges));
				}

				HRGN hClip = CreateRectRgnIndirect(&dis.rcItem);

				SelectClipRgn(hdc, hClip);

				drawTabItem(&dis);

				DeleteObject(hClip);

				SelectClipRgn(hdc, holdClip);
			}
		}

		SelectClipRgn(hdc, holdClip);
		if (holdClip)
		{
			DeleteObject(holdClip);
			holdClip = nullptr;
		}

		SelectObject(hdc, holdPen);

		EndPaint(hwnd, &ps);
		return 0;
	}

	case WM_LBUTTONDOWN:
	{
		_beginDrag = TRUE;
		return 0;
	}
	case WM_LBUTTONUP:
	{
		int iItem = 0;
		TCHITTESTINFO info = {};

		// get selected sub item
		info.pt.x = LOWORD(lParam);
		info.pt.y = HIWORD(lParam);
		iItem = static_cast<int32_t>(::SendMessage(hwnd, TCM_HITTEST, 0, reinterpret_cast<LPARAM>(&info)));

		selectTab(iItem);
		_beginDrag = FALSE;
		return 0;
	}
	case WM_LBUTTONDBLCLK:
	{
		NotifyParent((_isFloating == true) ? DMM_DOCK : DMM_FLOAT);
		return 0;
	}
	case WM_MBUTTONUP:
	{
		int iItem = 0;
		TCITEM tcItem{};
		TCHITTESTINFO info{};

		// get selected sub item
		info.pt.x = LOWORD(lParam);
		info.pt.y = HIWORD(lParam);
		iItem = static_cast<int32_t>(::SendMessage(hwnd, TCM_HITTEST, 0, reinterpret_cast<LPARAM>(&info)));

		selectTab(iItem);

		// get data and hide toolbar
		tcItem.mask = TCIF_PARAM;
		::SendMessage(hwnd, TCM_GETITEM, iItem, reinterpret_cast<LPARAM>(&tcItem));

		if (!tcItem.lParam)
			return FALSE;

		// notify child windows
		if (NotifyParent(DMM_CLOSE) == 0)
		{
			hideToolbar((tTbData *)tcItem.lParam);
		}
		return 0;
	}

	case WM_MOUSEMOVE:
	{
		int iItem = 0;
		TCHITTESTINFO info = {};

		// get selected sub item
		info.pt.x = LOWORD(lParam);
		info.pt.y = HIWORD(lParam);
		iItem = static_cast<int32_t>(::SendMessage(hwnd, TCM_HITTEST, 0, reinterpret_cast<LPARAM>(&info)));

		if ((_beginDrag == TRUE) && (wParam == MK_LBUTTON))
		{
			selectTab(iItem);

			// send moving message to parent window
			_dragFromTab = TRUE;
			NotifyParent(DMM_MOVE);
			_beginDrag = FALSE;
		}
		else
		{
			int iItemSel = static_cast<int32_t>(::SendMessage(hwnd, TCM_GETCURSEL, 0, 0));

			if ((_bTabTTHover == FALSE) && (iItem != iItemSel))
			{
				TRACKMOUSEEVENT tme{};
				tme.cbSize = sizeof(tme);
				tme.hwndTrack = hwnd;
				tme.dwFlags = TME_LEAVE | TME_HOVER;
				tme.dwHoverTime = 1000;
				_bTabTTHover = _TrackMouseEvent(&tme);
			}
			else
			{
				if (iItem == iItemSel)
				{
					toolTip.destroy();
					_bTabTTHover = FALSE;
				}
				else if (iItem != _iLastHovered)
				{
					TCITEM tcItem{};
					RECT rc{};

					// recalc mouse position
					::ClientToScreen(hwnd, &info.pt);

					// get text of toolbar
					tcItem.mask = TCIF_PARAM;
					::SendMessage(hwnd, TCM_GETITEM, iItem, reinterpret_cast<LPARAM>(&tcItem));
					if (!tcItem.lParam)
						break;

					// destroy old tooltip
					toolTip.destroy();

					toolTip.init(_hInst, hwnd);
					toolTip.Show(rc, (reinterpret_cast<tTbData *>(tcItem.lParam))->pszName, info.pt.x, info.pt.y + 20);
				}
			}

			// save last hovered item
			_iLastHovered = iItem;

			_beginDrag = FALSE;
		}
		return 0;
	}

	case WM_MOUSEHOVER:
	{
		int iItem = 0;
		TCITEM tcItem{};
		RECT rc{};
		TCHITTESTINFO info{};

		// get selected sub item
		info.pt.x = LOWORD(lParam);
		info.pt.y = HIWORD(lParam);
		iItem = static_cast<int32_t>(::SendMessage(hwnd, TCM_HITTEST, 0, reinterpret_cast<LPARAM>(&info)));

		// recalc mouse position
		::ClientToScreen(hwnd, &info.pt);

		// get text of toolbar
		tcItem.mask = TCIF_PARAM;
		::SendMessage(hwnd, TCM_GETITEM, iItem, reinterpret_cast<LPARAM>(&tcItem));
		if (!tcItem.lParam)
			break;

		toolTip.init(_hInst, hwnd);
		toolTip.Show(rc, reinterpret_cast<tTbData *>(tcItem.lParam)->pszName, info.pt.x, info.pt.y + 20);
		return 0;
	}

	case WM_MOUSELEAVE:
	{
		toolTip.destroy();
		_bTabTTHover = FALSE;
		return 0;
	}

	case WM_NOTIFY:
	{
		LPNMHDR lpnmhdr = reinterpret_cast<LPNMHDR>(lParam);

		if ((lpnmhdr->hwndFrom == _hContTab) && (lpnmhdr->code == TCN_GETOBJECT))
		{
			int iItem = 0;
			TCHITTESTINFO info = {};

			// get selected sub item
			info.pt.x = LOWORD(lParam);
			info.pt.y = HIWORD(lParam);
			iItem = static_cast<int32_t>(::SendMessage(hwnd, TCM_HITTEST, 0, reinterpret_cast<LPARAM>(&info)));

			selectTab(iItem);
			return 0;
		}
		break;
	}

	case WM_PARENTNOTIFY:
	{
		switch (LOWORD(wParam))
		{
		case WM_CREATE:
		{
			auto hwndUpdown = reinterpret_cast<HWND>(lParam);
			if (NppDarkMode::subclassTabUpDownControl(hwndUpdown))
			{
				_hTabUpdown = hwndUpdown;
				return 0;
			}
			break;
		}
		}
		break;
	}

	default:
		break;
	}

	return ::DefSubclassProc(hwnd, Message, wParam, lParam);
}

LRESULT DockingCont::DockingTabSubclass(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	switch (uMsg)
	{
	case WM_NCDESTROY:
	{
		::RemoveWindowSubclass(hWnd, DockingTabSubclass, uIdSubclass);
		break;
	}

	default:
		break;
	}
	return reinterpret_cast<DockingCont *>(dwRefData)->runProcTab(hWnd, uMsg, wParam, lParam);
}

void DockingCont::drawTabItem(DRAWITEMSTRUCT *pDrawItemStruct)
{
	TCITEM tcItem{};
	RECT rc = pDrawItemStruct->rcItem;

	int nTab = pDrawItemStruct->itemID;
	bool isSelected = (nTab == getActiveTb());

	// get current selected item
	tcItem.mask = TCIF_PARAM;
	::SendMessage(_hContTab, TCM_GETITEM, nTab, reinterpret_cast<LPARAM>(&tcItem));
	if (!tcItem.lParam)
		return;

	auto tbData = reinterpret_cast<tTbData *>(tcItem.lParam);

	const wchar_t *text = tbData->pszName;
	int length = lstrlen(tbData->pszName);

	// get drawing context
	HDC hDc = pDrawItemStruct->hDC;

	int nSavedDC = ::SaveDC(hDc);

	::SetBkMode(hDc, TRANSPARENT);

	const int onePadding = _dpiManager.scale(1);
	const int accentThickness = _dpiManager.scale(kDockingAccentThickness);
	const COLORREF selectedBg = NppDarkMode::isEnabled() ? NppDarkMode::getDlgBackgroundColor() : kDockingLightTabActive;
	const COLORREF inactiveBg = NppDarkMode::isEnabled() ? NppDarkMode::getCtrlBackgroundColor() : kDockingLightTabInactive;
	const COLORREF selectedText = NppDarkMode::isEnabled() ? NppDarkMode::getTextColor() : kDockingLightText;
	const COLORREF accentColor = NppDarkMode::isEnabled() ? NppDarkMode::getAccentColor() : RGB(84, 122, 181);

	HBRUSH tabBrush = ::CreateSolidBrush(isSelected ? selectedBg : inactiveBg);
	::FillRect(hDc, &rc, tabBrush);
	::DeleteObject(tabBrush);
	::OffsetRect(&rc, 0, -onePadding);

	if (isSelected)
	{
		RECT accentRect = rc;
		accentRect.bottom = accentRect.top + accentThickness;
		HBRUSH accentBrush = ::CreateSolidBrush(accentColor);
		::FillRect(hDc, &accentRect, accentBrush);
		::DeleteObject(accentBrush);
	}

	// draw icon if enabled
	if ((tbData->uMask & DWS_ICONTAB) == DWS_ICONTAB)
	{
		const int wPadding = _dpiManager.scale(g_dockingContTabIconPadding);
		const int iconSize = _dpiManager.scale(g_dockingContTabIconSize);

		const int iconDpiDynamicalX = rc.left + (isSelected ? wPadding : (rc.right - rc.left - iconSize + 1) / 2);
		const int iconDpiDynamicalY = rc.top + (rc.bottom - rc.top - iconSize - onePadding) / 2;

		::DrawIconEx(hDc, iconDpiDynamicalX, iconDpiDynamicalY, tbData->hIconTab, iconSize, iconSize, 0, nullptr, DI_NORMAL);

		if (isSelected)
		{
			rc.left += iconSize + wPadding * 2;
		}
	}

	if (isSelected)
	{
		const int textOffset = 3 * onePadding / 2 - 1;
		::OffsetRect(&rc, 0, -textOffset);
		::SetTextColor(hDc, selectedText);

		// draw text
		auto hOldFont = static_cast<HFONT>(::SelectObject(hDc, _hFont));
		rc.left += _dpiManager.scale(2);
		::DrawText(hDc, text, length, &rc, DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS);
		::SelectObject(hDc, hOldFont);
	}

	::RestoreDC(hDc, nSavedDC);
}

//----------------------------------------------
//    Process function of dialog
//
intptr_t CALLBACK DockingCont::run_dlgProc(UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch (Message)
	{
	case WM_NCACTIVATE:
	{
		// Note: lParam to identify the trigger window
		if (static_cast<int>(lParam) != -1)
		{
			::SendMessage(_hParent, WM_NCACTIVATE, wParam, 0);
		}
		break;
	}
	case WM_INITDIALOG:
	{
		_hContTab = ::GetDlgItem(_hSelf, IDC_TAB_CONT);
		_hCaption = ::GetDlgItem(_hSelf, IDC_BTN_CAPTION);

		// intial subclassing of caption
		::SetWindowSubclass(_hCaption, DockingCaptionSubclass, static_cast<UINT_PTR>(SubclassID::first), reinterpret_cast<DWORD_PTR>(this));

		// intial subclassing of tab
		::SetWindowSubclass(_hContTab, DockingTabSubclass, static_cast<UINT_PTR>(SubclassID::first), reinterpret_cast<DWORD_PTR>(this));

		// set min tab width
		const int tabDpiPadding = _dpiManager.scale(g_dockingContTabIconSize + g_dockingContTabIconPadding * 2);
		::SendMessage(_hContTab, TCM_SETMINTABWIDTH, 0, tabDpiPadding);
		TabCtrl_SetPadding(_hContTab, (tabDpiPadding / 2) + _dpiManager.scale(2), 0);
		TabCtrl_SetItemSize(_hContTab, 2 * tabDpiPadding + _dpiManager.scale(8), tabDpiPadding + _dpiManager.scale(2));

		return TRUE;
	}
	case WM_NCCALCSIZE:
	case WM_SIZE:
	{
		onSize();
		break;
	}
	case WM_GETMINMAXINFO:
	{
		if (_isFloating)
		{
			// ensure a reasonable minimal size of the floating panels
			LPMINMAXINFO pMinMax = reinterpret_cast<LPMINMAXINFO>(lParam);
			pMinMax->ptMinTrackSize.x = NppParameters::getInstance().getNppGUI()._dockingData._minFloatingPanelSize.cx;
			pMinMax->ptMinTrackSize.y = NppParameters::getInstance().getNppGUI()._dockingData._minFloatingPanelSize.cy;
			return 0;
		}
		break;
	}

	case WM_ERASEBKGND:
	{
		HDC hDC = reinterpret_cast<HDC>(wParam);

		RECT rc{};
		getClientRect(rc);

		RECT rcTab{};
		getMappedChildRect(_hContTab, rcTab);

		RECT rcClientTab{};
		getMappedChildRect(IDC_CLIENT_TAB, rcClientTab);

		RECT rcCap{};
		getMappedChildRect(_hCaption, rcCap);

		::ExcludeClipRect(hDC, rcTab.left, rcTab.top, rcTab.right, rcTab.bottom);
		::ExcludeClipRect(hDC, rcClientTab.left, rcClientTab.top, rcClientTab.right, rcClientTab.bottom);
		::ExcludeClipRect(hDC, rcCap.left, rcCap.top, rcCap.right, rcCap.bottom);

		::FillRect(hDC, &rc, NppDarkMode::isEnabled() ? NppDarkMode::getDlgBackgroundBrush() : ::GetSysColorBrush(COLOR_3DFACE));
		return TRUE;
	}

	case WM_CTLCOLORDLG:
	case WM_CTLCOLORSTATIC:
	{
		return NppDarkMode::onCtlColorDlg(reinterpret_cast<HDC>(wParam));
	}

	case WM_DRAWITEM:
	{
		// draw tab or caption
		if (reinterpret_cast<DRAWITEMSTRUCT *>(lParam)->CtlID == IDC_TAB_CONT)
		{
			if (!NppDarkMode::isEnabled())
			{
				drawTabItem(reinterpret_cast<DRAWITEMSTRUCT *>(lParam));
				return TRUE;
			}
			break;
		}
		else
		{
			drawCaptionItem((DRAWITEMSTRUCT *)lParam);
			return TRUE;
		}
		break;
	}
	case WM_NCLBUTTONDBLCLK:
	{
		RECT rcWnd{};
		RECT rcClient{};
		POINT pt = {HIWORD(lParam), LOWORD(lParam)};

		getWindowRect(rcWnd);
		getClientRect(rcClient);
		ClientRectToScreenRect(_hSelf, &rcClient);
		rcWnd.bottom = rcClient.top;

		// if in caption
		if ((rcWnd.top < pt.x) && (rcWnd.bottom > pt.x) &&
			(rcWnd.left < pt.y) && (rcWnd.right > pt.y))
		{
			NotifyParent(DMM_DOCKALL);
			return TRUE;
		}
		break;
	}
	case WM_SYSCOMMAND:
	{
		switch (wParam & 0xfff0)
		{
		case SC_MOVE:
			NotifyParent(DMM_MOVE);
			return TRUE;
		default:
			break;
		}
		return FALSE;
	}

	case WM_DPICHANGED:
	{
		_dpiManager.setDpiWP(wParam);
		[[fallthrough]];
	}
	case WM_DPICHANGED_AFTERPARENT:
	{
		if (Message != WM_DPICHANGED)
		{
			_dpiManager.setDpi(_hParent);
		}
		_captionHeightDynamic = _dpiManager.scale(HIGH_CAPTION);
		_captionGapDynamic = _dpiManager.scale(CAPTION_GAP);
		_closeButtonPosLeftDynamic = _dpiManager.scale(CLOSEBTN_POS_LEFT);
		_closeButtonPosTopDynamic = _dpiManager.scale(CLOSEBTN_POS_TOP);

		_closeButtonWidth = _dpiManager.scale(g_dockingContCloseBtnSize);
		_closeButtonHeight = _dpiManager.scale(g_dockingContCloseBtnSize);

		const int tabDpiPadding = _dpiManager.scale(g_dockingContTabIconSize + g_dockingContTabIconPadding * 2);
		::SendMessage(_hContTab, TCM_SETMINTABWIDTH, 0, tabDpiPadding);
		TabCtrl_SetPadding(_hContTab, (tabDpiPadding / 2) + _dpiManager.scale(2), 0);
		TabCtrl_SetItemSize(_hContTab, 2 * tabDpiPadding + _dpiManager.scale(8), tabDpiPadding + _dpiManager.scale(2));

		destroyFonts();

		LOGFONT lfTab{createDockingFont(_hParent, DPIManagerV2::FontType::regular, FW_MEDIUM)};
		_hFont = ::CreateFontIndirect(&lfTab);

		LOGFONT lfCaption{createDockingFont(_hParent, DPIManagerV2::FontType::smcaption, FW_SEMIBOLD)};
		_hFontCaption = ::CreateFontIndirect(&lfCaption);

		if (Message == WM_DPICHANGED)
		{
			_dpiManager.setPositionDpi(lParam, _hSelf);
		}
		else
		{
			onSize();
		}

		return TRUE;
	}

	case WM_COMMAND:
	{
		switch (LOWORD(wParam))
		{
		case IDCANCEL:
			doClose(GetKeyState(VK_SHIFT) < 0);
			return TRUE;
		default:
			break;
		}
		break;
	}
	default:
		break;
	}

	return FALSE;
}

void DockingCont::onSize()
{
	TCITEM tcItem{};
	RECT rc{};
	RECT rcTemp{};
	UINT iItemCnt = static_cast<int32_t>(::SendMessage(_hContTab, TCM_GETITEMCOUNT, 0, 0));
	UINT iTabOff = 0;

	getClientRect(rc);

	if (iItemCnt >= 1)
	{
		// resize to docked window
		const int tabDpiDynamicalHeight = _dpiManager.scale(g_dockingContTabIconSize + (g_dockingContTabIconPadding) * 2 + CAPTION_GAP);
		if (_isFloating == false)
		{
			// draw caption
			if (_isTopCaption == TRUE)
			{
				::SetWindowPos(_hCaption, NULL, rc.left, rc.top, rc.right, _captionHeightDynamic, SWP_NOZORDER | SWP_NOACTIVATE);
				rc.top += _captionHeightDynamic;
				rc.bottom -= _captionHeightDynamic;
			}
			else
			{
				::SetWindowPos(_hCaption, NULL, rc.left, rc.top, _captionHeightDynamic, rc.bottom, SWP_NOZORDER | SWP_NOACTIVATE);
				rc.left += _captionHeightDynamic;
				rc.right -= _captionHeightDynamic;
			}

			if (iItemCnt >= 2)
			{
				// resize tab and plugin control if tabs exceeds one
				// resize tab
				rcTemp = rc;
				rcTemp.top = (rcTemp.bottom + rcTemp.top) - (tabDpiDynamicalHeight + _captionGapDynamic);
				rcTemp.bottom = tabDpiDynamicalHeight;
				iTabOff = tabDpiDynamicalHeight;

				::SetWindowPos(_hContTab, NULL,
							   rcTemp.left, rcTemp.top, rcTemp.right, rcTemp.bottom,
							   SWP_NOZORDER | SWP_SHOWWINDOW | SWP_NOACTIVATE);

				if (_hTabUpdown != nullptr)
				{
					::InvalidateRect(_hTabUpdown, nullptr, TRUE);
					::UpdateWindow(_hTabUpdown);
				}
			}

			// resize client area for plugin
			rcTemp = rc;
			if (_isTopCaption == TRUE)
			{
				rcTemp.top += _captionGapDynamic;
				rcTemp.bottom -= (iTabOff + _captionGapDynamic);
			}
			else
			{
				rcTemp.left += _captionGapDynamic;
				rcTemp.right -= _captionGapDynamic;
				rcTemp.bottom -= iTabOff;
			}

			// set position of client area
			::SetWindowPos(::GetDlgItem(_hSelf, IDC_CLIENT_TAB), NULL,
						   rcTemp.left, rcTemp.top, rcTemp.right, rcTemp.bottom,
						   SWP_NOZORDER | SWP_NOACTIVATE);
		}
		// resize to float window
		else
		{
			// update floating size
			for (size_t iTb = 0, len = _vTbData.size(); iTb < len; ++iTb)
			{
				getWindowRect(_vTbData[iTb]->rcFloat);
			}

			// draw caption
			if (iItemCnt >= 2)
			{
				// resize tab if size of elements exceeds one
				rcTemp = rc;
				rcTemp.top = rcTemp.bottom - (tabDpiDynamicalHeight + _captionGapDynamic);
				rcTemp.bottom = tabDpiDynamicalHeight;

				::SetWindowPos(_hContTab, NULL,
							   rcTemp.left, rcTemp.top, rcTemp.right, rcTemp.bottom,
							   SWP_NOZORDER | SWP_SHOWWINDOW);

				if (_hTabUpdown != nullptr)
				{
					::InvalidateRect(_hTabUpdown, nullptr, TRUE);
					::UpdateWindow(_hTabUpdown);
				}
			}

			// resize client area for plugin
			rcTemp = rc;
			rcTemp.bottom -= ((iItemCnt == 1) ? 0 : tabDpiDynamicalHeight);

			::SetWindowPos(::GetDlgItem(_hSelf, IDC_CLIENT_TAB), NULL,
						   rcTemp.left, rcTemp.top, rcTemp.right, rcTemp.bottom,
						   SWP_NOZORDER | SWP_NOACTIVATE);
		}

		// get active item data
		size_t iItemCnt2 = static_cast<size_t>(::SendMessage(_hContTab, TCM_GETITEMCOUNT, 0, 0));

		// resize visible plugin windows
		for (size_t iItem = 0; iItem < iItemCnt2; ++iItem)
		{
			tcItem.mask = TCIF_PARAM;
			::SendMessage(_hContTab, TCM_GETITEM, iItem, reinterpret_cast<LPARAM>(&tcItem));
			if (!tcItem.lParam)
				continue;

			::SetWindowPos(((tTbData *)tcItem.lParam)->hClient, NULL,
						   0, 0, rcTemp.right, rcTemp.bottom,
						   SWP_NOZORDER);

			// Notify switch in
			NMHDR nmhdr{};
			nmhdr.code = DMN_FLOATDROPPED;
			nmhdr.hwndFrom = _hSelf;
			nmhdr.idFrom = 0;
			::SendMessage(((tTbData *)tcItem.lParam)->hClient, WM_NOTIFY, nmhdr.idFrom, reinterpret_cast<LPARAM>(&nmhdr));
		}
	}
}

void DockingCont::doClose(BOOL closeAll)
{
	// Always close active tab first
	int iItemCur = getActiveTb();
	TCITEM item{};
	item.mask = TCIF_PARAM;
	::SendMessage(_hContTab, TCM_GETITEM, iItemCur, reinterpret_cast<LPARAM>(&item));
	if (item.lParam)
	{
		// notify child windows
		if (NotifyParent(DMM_CLOSE) == 0)
		{
			// delete tab
			hideToolbar((tTbData *)item.lParam);
		}
	}

	// Close all other tabs if requested
	if (closeAll)
	{
		int nbItem = static_cast<int32_t>(::SendMessage(_hContTab, TCM_GETITEMCOUNT, 0, 0));
		int iItemOff = 0;
		for (int iItem = 0; iItem < nbItem; ++iItem)
		{
			TCITEM tcItem{};
			// get item data
			selectTab(iItemOff);
			tcItem.mask = TCIF_PARAM;
			::SendMessage(_hContTab, TCM_GETITEM, iItemOff, reinterpret_cast<LPARAM>(&tcItem));
			if (!tcItem.lParam)
				continue;

			// notify child windows
			if (NotifyParent(DMM_CLOSE) == 0)
			{
				// delete tab
				hideToolbar((tTbData *)tcItem.lParam);
			}
			else
			{
				++iItemOff;
			}
		}
	}

	// Hide dialog window if all tabs closed
	int iItemCnt = static_cast<int32_t>(::SendMessage(_hContTab, TCM_GETITEMCOUNT, 0, 0));
	if (iItemCnt == 0)
	{
		// hide dialog first
		doDialog(false);
		::SendMessage(_hParent, WM_SIZE, 0, 0);
	}
}

void DockingCont::showToolbar(tTbData *pTbData, BOOL state)
{
	if (state == SW_SHOW)
	{
		viewToolbar(pTbData);
	}
	else
	{
		hideToolbar(pTbData);
	}
}

int DockingCont::hideToolbar(tTbData *pTbData, BOOL hideClient)
{
	int iItem = searchPosInTab(pTbData);
	BOOL hadFocus = ::IsChild(pTbData->hClient, ::GetFocus());

	// delete item
	if (TRUE == ::SendMessage(_hContTab, TCM_DELETEITEM, iItem, 0))
	{
		auto iItemCnt = ::SendMessage(_hContTab, TCM_GETITEMCOUNT, 0, 0);

		if (iItemCnt != 0)
		{
			if (iItem == iItemCnt)
			{
				iItem--;
			}

			// activate new selected item and view plugin dialog
			_prevItem = iItem;
			selectTab(iItem);

			// hide tabs if only one element
			if (iItemCnt == 1)
			{
				::ShowWindow(_hContTab, SW_HIDE);
			}
		}
		else
		{
			// hide dialog
			doDialog(false);

			// send message to docking manager for resize
			if (!_isFloating)
			{
				::SendMessage(_hParent, WM_SIZE, 0, 0);
			}
			// set focus to current edit window if the docking window had focus
			if (hadFocus)
				::PostMessage(::GetParent(_hParent), WM_ACTIVATE, WA_ACTIVE, 0);
		}

		// keep sure, that client is hide!!!
		if (hideClient == TRUE)
		{
			::ShowWindow(pTbData->hClient, SW_HIDE);
		}
	}
	onSize();

	return iItem;
}

void DockingCont::viewToolbar(tTbData *pTbData)
{
	TCITEM tcItem{};
	int iItemCnt = static_cast<int32_t>(::SendMessage(_hContTab, TCM_GETITEMCOUNT, 0, 0));

	if (iItemCnt > 0)
	{
		UINT iItem = getActiveTb();

		tcItem.mask = TCIF_PARAM;
		::SendMessage(_hContTab, TCM_GETITEM, iItem, reinterpret_cast<LPARAM>(&tcItem));
		if (!tcItem.lParam)
			return;

		// hide active dialog
		::ShowWindow(((tTbData *)tcItem.lParam)->hClient, SW_HIDE);
	}

	// create new tab if it not exists
	int iTabPos = searchPosInTab(pTbData);
	tcItem.mask = TCIF_PARAM;
	tcItem.lParam = reinterpret_cast<LPARAM>(pTbData);

	if (iTabPos == -1)
	{
		// set only params and text even if icon available
		::SendMessage(_hContTab, TCM_INSERTITEM, iItemCnt, reinterpret_cast<LPARAM>(&tcItem));
		selectTab(iItemCnt);
	}
	// if exists select it and update data
	else
	{
		::SendMessage(_hContTab, TCM_SETITEM, iTabPos, reinterpret_cast<LPARAM>(&tcItem));
		selectTab(iTabPos);
	}

	// show dialog and notify parent to update dialog view
	if (isVisible() == false)
	{
		doDialog();
		::SendMessage(_hParent, WM_SIZE, 0, 0);
	}

	// set position of client
	onSize();
}

int DockingCont::searchPosInTab(tTbData *pTbData)
{
	TCITEM tcItem{};
	int iItemCnt = static_cast<int32_t>(::SendMessage(_hContTab, TCM_GETITEMCOUNT, 0, 0));

	tcItem.mask = TCIF_PARAM;

	for (int iItem = 0; iItem < iItemCnt; ++iItem)
	{
		::SendMessage(_hContTab, TCM_GETITEM, iItem, reinterpret_cast<LPARAM>(&tcItem));
		if (!tcItem.lParam)
			continue;

		if (((tTbData *)tcItem.lParam)->hClient == pTbData->hClient)
			return iItem;
	}
	return -1;
}

void DockingCont::selectTab(int iTab)
{
	if (iTab != -1)
	{
		const wchar_t *pszMaxTxt = NULL;
		TCITEM tcItem{};
		SIZE size = {};
		int maxWidth = 0;
		int iItemCnt = static_cast<int32_t>(::SendMessage(_hContTab, TCM_GETITEMCOUNT, 0, 0));

		// get data of new active dialog
		tcItem.mask = TCIF_PARAM;
		::SendMessage(_hContTab, TCM_GETITEM, iTab, reinterpret_cast<LPARAM>(&tcItem));
		// show active dialog
		if (!tcItem.lParam)
			return;

		::ShowWindow(((tTbData *)tcItem.lParam)->hClient, SW_SHOW);
		::SetFocus(((tTbData *)tcItem.lParam)->hClient);

		// Notify switch in
		NMHDR nmhdrIn{};
		nmhdrIn.code = DMN_SWITCHIN;
		nmhdrIn.hwndFrom = _hSelf;
		nmhdrIn.idFrom = 0;
		::SendMessage(reinterpret_cast<tTbData *>(tcItem.lParam)->hClient, WM_NOTIFY, nmhdrIn.idFrom, reinterpret_cast<LPARAM>(&nmhdrIn));

		if (static_cast<unsigned int>(iTab) != _prevItem)
		{
			// hide previous dialog
			::SendMessage(_hContTab, TCM_GETITEM, _prevItem, reinterpret_cast<LPARAM>(&tcItem));

			if (!tcItem.lParam)
				return;
			::ShowWindow(((tTbData *)tcItem.lParam)->hClient, SW_HIDE);

			// Notify switch off
			NMHDR nmhdrOff{};
			nmhdrOff.code = DMN_SWITCHOFF;
			nmhdrOff.hwndFrom = _hSelf;
			nmhdrOff.idFrom = 0;
			::SendMessage(((tTbData *)tcItem.lParam)->hClient, WM_NOTIFY, nmhdrOff.idFrom, reinterpret_cast<LPARAM>(&nmhdrOff));
		}

		// resize tab item

		// get at first largest item ...
		HDC hDc = ::GetDC(_hContTab);
		SelectObject(hDc, _hFont);

		for (int iItem = 0; iItem < iItemCnt; ++iItem)
		{
			const wchar_t *pszTabTxt = NULL;

			::SendMessage(_hContTab, TCM_GETITEM, iItem, reinterpret_cast<LPARAM>(&tcItem));
			if (!tcItem.lParam)
				continue;
			pszTabTxt = reinterpret_cast<tTbData *>(tcItem.lParam)->pszName;

			// get current font width
			GetTextExtentPoint32(hDc, pszTabTxt, lstrlen(pszTabTxt), &size);

			if (maxWidth < size.cx)
			{
				maxWidth = size.cx;
				pszMaxTxt = pszTabTxt;
			}
		}
		::ReleaseDC(_hSelf, hDc);

		tcItem.mask = TCIF_TEXT;

		for (int iItem = 0; iItem < iItemCnt; ++iItem)
		{
			wstring szText;
			if (iItem == iTab && pszMaxTxt)
			{
				// fake here an icon before text ...
				szText = L"        ";
				szText += pszMaxTxt;
			}
			tcItem.pszText = (wchar_t *)szText.c_str();
			::SendMessage(_hContTab, TCM_SETITEM, iItem, reinterpret_cast<LPARAM>(&tcItem));
		}

		// selects the pressed tab and store previous tab
		::SendMessage(_hContTab, TCM_SETCURSEL, iTab, 0);
		_prevItem = iTab;

		// update caption text
		updateCaption();

		onSize();
	}
}

bool DockingCont::updateCaption()
{
	if (!_hContTab)
		return false;

	TCITEM tcItem{};
	int iItem = getActiveTb();

	if (iItem < 0)
		return false;

	// get data of new active dialog
	tcItem.mask = TCIF_PARAM;
	::SendMessage(_hContTab, TCM_GETITEM, iItem, reinterpret_cast<LPARAM>(&tcItem));

	if (!tcItem.lParam)
		return false;

	// update caption text
	_pszCaption = ((tTbData *)tcItem.lParam)->pszName;

	// test if additional information are available
	if ((((tTbData *)tcItem.lParam)->uMask & DWS_ADDINFO) &&
		(lstrlen(((tTbData *)tcItem.lParam)->pszAddInfo) != 0))
	{
		_pszCaption += L" - ";
		_pszCaption += ((tTbData *)tcItem.lParam)->pszAddInfo;
	}

	if (_isFloating == true)
	{
		::SetWindowText(_hSelf, _pszCaption.c_str());
	}
	else
	{
		::SetWindowText(_hCaption, _pszCaption.c_str());
	}
	return true;
}

void DockingCont::focusClient()
{
	TCITEM tcItem{};
	int iItem = getActiveTb();

	if (iItem != -1)
	{
		// get data of new active dialog
		tcItem.mask = TCIF_PARAM;
		::SendMessage(_hContTab, TCM_GETITEM, iItem, reinterpret_cast<LPARAM>(&tcItem));

		// set focus
		if (!tcItem.lParam)
			return;

		tTbData *tbData = (tTbData *)tcItem.lParam;
		if (tbData->pszAddInfo && lstrcmp(tbData->pszAddInfo, DM_NOFOCUSWHILECLICKINGCAPTION) == 0)
			return;

		::SetFocus(tbData->hClient);
	}
}

LPARAM DockingCont::NotifyParent(UINT message)
{
	return ::SendMessage(_hParent, message, 0, reinterpret_cast<LPARAM>(this));
}
