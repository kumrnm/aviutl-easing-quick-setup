#include <Windows.h>
#include "tchar.h"
#include "aviutl_plugin_sdk/filter.h"
#include "auls/aulslib/exedit.h"
#include "auls/yulib/extra.h"

#define PLUGIN_NAME "イージング設定時短プラグイン"
#define PLUGIN_VERSION "v1.1.0"
#define PLUGIN_DEVELOPER "蝙蝠の目"

#ifdef _DEBUG
#define PLUGIN_VERSION_SUFFIX " (Debug)"
#include "dbgstream/dbgstream.h"
#else
#define PLUGIN_VERSION_SUFFIX ""
#endif


void show_error(LPCTSTR text) {
	MessageBox(NULL, text, PLUGIN_NAME, MB_OK | MB_ICONERROR);
}

struct {
	BOOL auto_popup;
	BOOL move_cursor;
} config;


//================================
//  拡張編集の内部処理への干渉
//================================

HHOOK hhooks[2];
HWND hwnd_exedit, hwnd_objDlg;
POINT menu_position;
bool dialogShortcutLocked = true;


// WndProcのフック

LRESULT CALLBACK HookProc_WndProc(int nCode, WPARAM wParam, LPARAM lParam) {
	auto info = reinterpret_cast<PCWPSTRUCT>(lParam);

	if (info->hwnd == hwnd_exedit) {

		if (info->message == WM_FILTER_EXIT) {
			// フック解除
			if (hhooks[0]) UnhookWindowsHookEx(hhooks[0]);
			if (hhooks[1]) UnhookWindowsHookEx(hhooks[1]);
		}

	}
	else if (info->hwnd == hwnd_objDlg) {

		if (info->message == WM_COMMAND && 4000 <= info->wParam && info->wParam < 4000 + auls::EXEDIT_OBJECT::MAX_TRACK) {
			// イージングボタンクリック時
			if (GetKeyState(VK_MENU) < 0) {
				dialogShortcutLocked = true;
			}
			else {
				dialogShortcutLocked = false;
			}
		} else if (info->message == WM_COMMAND && info->wParam == 0x462) {
			// イージングパラメータ設定ダイアログ出現時（Altクリック時は何故か反応しない）
			dialogShortcutLocked = true;
		}

	}

	return CallNextHookEx(NULL, nCode, wParam, lParam);
}

LRESULT CALLBACK HookProc_WndProcRet(int nCode, WPARAM wParam, LPARAM lParam) {
	auto info = reinterpret_cast<PCWPRETSTRUCT>(lParam);

	if (info->hwnd == hwnd_exedit) {

		if (info->message == WM_FILTER_INIT) {
			
			// オブジェクト設定ウィンドウを取得
			hwnd_objDlg = auls::ObjDlg_GetWindow(hwnd_exedit);

		}
		else if (info->message == WM_COMMAND && info->wParam == 1003) {

			// イージング変更時
			const int trackbar_id = info->lParam - 0x80000;
			if (0 <= trackbar_id && trackbar_id < auls::EXEDIT_OBJECT::MAX_TRACK && !dialogShortcutLocked) {

				// カーソルをボタン上に移動
				if (config.move_cursor) {
					SetCursorPos(menu_position.x, menu_position.y);
				}

				// イージングパラメータ設定ダイアログを表示
				if (config.auto_popup) {
					SendMessage(auls::ObjDlg_GetWindow(hwnd_exedit), WM_COMMAND, 0x462, trackbar_id);
				}

			}

		}

	}

	return CallNextHookEx(NULL, nCode, wParam, lParam);
}


// ポップアップメニュー表示処理をフック

BOOL (WINAPI* TrackPopupMenu_original)(_In_ HMENU hMenu, _In_ UINT uFlags, _In_ int x, _In_ int y, _Reserved_ int nReserved, _In_ HWND hWnd, _Reserved_ CONST RECT* prcRect) = nullptr;
using TrackPopupMenu_t = decltype(TrackPopupMenu_original);
BOOL WINAPI TrackPopupMenu_hooked(_In_ HMENU hMenu, _In_ UINT uFlags, _In_ int x, _In_ int y, _Reserved_ int nReserved, _In_ HWND hWnd, _Reserved_ CONST RECT* prcRect) {
	// 表示位置（クリック位置）を記録する
	menu_position.x = x;
	menu_position.y = y;

	return TrackPopupMenu_original(hMenu, uFlags, x, y, nReserved, hWnd, prcRect);
}



//================================
//  初期化処理
//================================

BOOL func_init(FILTER* fp) {
	auto exedit = auls::Exedit_GetFilter(fp);
	if (!exedit) {
		show_error(TEXT("拡張編集が見つかりませんでした。"));
	}
	else {
		hwnd_exedit = exedit->hwnd;

		// コンフィグ
		const int CONFIG_UNDEFINED = -0x3777;
#define LoadConfig(name, default_value) \
	config.name = fp->exfunc->ini_load_int(fp, (LPSTR)#name, CONFIG_UNDEFINED); \
	if (config.name == CONFIG_UNDEFINED) { \
		config.name = default_value; \
		fp->exfunc->ini_save_int(fp, (LPSTR)#name, config.name); \
	}

		LoadConfig(auto_popup, 1)
		LoadConfig(move_cursor, 0)
#undef LoadConfig

		// 各種フック

		hhooks[0] = SetWindowsHookEx(WH_CALLWNDPROC, HookProc_WndProc, NULL, GetCurrentThreadId());
		hhooks[1] = SetWindowsHookEx(WH_CALLWNDPROCRET, HookProc_WndProcRet, NULL, GetCurrentThreadId());

		char exedit_path[1024] = {};
		GetModuleFileName(exedit->dll_hinst, exedit_path, sizeof(exedit_path));

		if (config.move_cursor) {
			TrackPopupMenu_original = (TrackPopupMenu_t)yulib::RewriteFunction(exedit_path, "TrackPopupMenu", TrackPopupMenu_hooked);
		}
	}

	return TRUE;
}


//================================
//  エクスポート情報
//================================

FILTER_DLL filter = {
	.flag = FILTER_FLAG_ALWAYS_ACTIVE | FILTER_FLAG_NO_CONFIG | FILTER_FLAG_EX_INFORMATION,
	.name = (TCHAR*)PLUGIN_NAME,
	.func_init = func_init,
	.information = (TCHAR*)(PLUGIN_NAME " " PLUGIN_VERSION PLUGIN_VERSION_SUFFIX " by " PLUGIN_DEVELOPER)
};

EXTERN_C __declspec(dllexport) FILTER_DLL* __stdcall GetFilterTable(void) {
	return &filter;
}
