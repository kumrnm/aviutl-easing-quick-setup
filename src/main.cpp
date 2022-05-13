#include <Windows.h>
#include "tchar.h"
#include "aviutl_plugin_sdk/filter.h"
#include "auls/aulslib/exedit.h"
#include "auls/yulib/extra.h"

#ifdef _DEBUG
#include "dbgstream/dbgstream.h"
#endif


#define PLUGIN_NAME TEXT("イージング設定時短プラグイン")
#ifdef _DEBUG
#define PLUGIN_NAME_SUFFIX TEXT("（Debug）")
#else
#define PLUGIN_NAME_SUFFIX TEXT("")
#endif
#define PLUGIN_VERSION TEXT("1.0.1")


void show_error(LPCTSTR text) {
	MessageBox(NULL, text, PLUGIN_NAME, MB_OK | MB_ICONERROR);
}

UINT64 GetTime() {
	FILETIME ft;
	GetSystemTimeAsFileTime(&ft);
	return ((UINT64)(ft.dwHighDateTime) << 32) | (UINT64)(ft.dwLowDateTime);
}

struct {
	BOOL auto_popup;
	BOOL move_cursor;
} config;


//================================
//  拡張編集の内部処理への干渉
//================================

POINT menu_position;


// イベントハンドラをフック

BOOL(*exedit_WndProc_original)(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam, void* editp, void* fp) = nullptr;
using exedit_WndProc_t = decltype(exedit_WndProc_original);
BOOL exedit_WndProc_hooked(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam, void* editp, void* fp) {
	const auto res = exedit_WndProc_original(hwnd, message, wparam, lparam, editp, fp);

	// イージング設定の変更を捕捉
	if (message == WM_COMMAND && wparam == 1003 /* OBJECT_DIALOG_MENU */) {
		const int trackbar_id = lparam - 0x80000;
		if (0 <= trackbar_id && trackbar_id < auls::EXEDIT_OBJECT::MAX_TRACK) {

			// 無限ループ対策
			static bool in_this_procedure = false;
			if (!in_this_procedure) {

				// カーソルをボタン上に移動
				if (config.move_cursor) {
					SetCursorPos(menu_position.x, menu_position.y);
				}

				// イージングパラメータ設定ダイアログを表示
				if (config.auto_popup) {
					in_this_procedure = true;
					SendMessage(auls::ObjDlg_GetWindow(hwnd), WM_COMMAND, 0x462, trackbar_id);
					in_this_procedure = false;
				}

			}

		}
	}

	return res;
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

		char exedit_path[1024] = {};
		GetModuleFileName(exedit->dll_hinst, exedit_path, sizeof(exedit_path));

		if (config.move_cursor) {
			TrackPopupMenu_original = (TrackPopupMenu_t)yulib::RewriteFunction(exedit_path, "TrackPopupMenu", TrackPopupMenu_hooked);
		}

		exedit_WndProc_original = exedit->func_WndProc;
		exedit->func_WndProc = exedit_WndProc_hooked;
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
	.information = (TCHAR*)(PLUGIN_NAME PLUGIN_NAME_SUFFIX TEXT(" v") PLUGIN_VERSION)
};

EXTERN_C __declspec(dllexport) FILTER_DLL* __stdcall GetFilterTable(void) {
	return &filter;
}
