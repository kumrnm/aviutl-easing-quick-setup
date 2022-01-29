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
#define PLUGIN_VERSION TEXT("1.0.0")


void show_error(LPCTSTR text) {
	MessageBox(NULL, text, PLUGIN_NAME, MB_OK | MB_ICONERROR);
}

UINT64 GetTime() {
	FILETIME ft;
	GetSystemTimeAsFileTime(&ft);
	return ((UINT64)(ft.dwHighDateTime) << 32) | (UINT64)(ft.dwLowDateTime);
}


//================================
//  拡張編集の内部処理への干渉
//================================

UINT64 easing_dialog_last_closed_time = 0;
UINT64 EASING_DIALOG_COOL_TIME = 2000000; // 0.2s


// イベント処理に割り込み

BOOL(*exedit_WndProc_original)(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam, void* editp, void* fp);
using exedit_WndProc_t = decltype(exedit_WndProc_original);
BOOL exedit_WndProc_hooked(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam, void* editp, void* fp) {
	const auto res = exedit_WndProc_original(hwnd, message, wparam, lparam, editp, fp);

	// イージング設定の変更を捕捉
	if (message == WM_COMMAND && wparam == 1003 /* OBJECT_DIALOG_MENU */) {
		const int trackbar_id = lparam - 0x80000;
		if (0 <= trackbar_id && trackbar_id < auls::EXEDIT_OBJECT::MAX_TRACK) {
			if (GetTime() - easing_dialog_last_closed_time > EASING_DIALOG_COOL_TIME) { // クールタイムを設けないと無限ループになる
				// イージングパラメータ設定ダイアログを表示
				SendMessage(auls::ObjDlg_GetWindow(hwnd), WM_COMMAND, 0x462, trackbar_id);
			}
		}
	}

	return res;
}


// 各種ダイアログ表示処理に割り込み

INT_PTR(WINAPI* DialogBoxParamA_original)(_In_opt_ HINSTANCE hInstance, _In_ LPCSTR lpTemplateName, _In_opt_ HWND hWndParent, _In_opt_ DLGPROC lpDialogFunc, _In_ LPARAM dwInitParam);
using DialogBoxParamA_t = decltype(DialogBoxParamA_original);
INT_PTR WINAPI DialogBoxParamA_hooked(_In_opt_ HINSTANCE hInstance, _In_ LPCSTR lpTemplateName, _In_opt_ HWND hWndParent, _In_opt_ DLGPROC lpDialogFunc, _In_ LPARAM dwInitParam) {
	const auto res = DialogBoxParamA_original(hInstance, lpTemplateName, hWndParent, lpDialogFunc, dwInitParam);

	if (_tcscmp(lpTemplateName, TEXT("GET_PARAM_SMALL")) == 0 && res == 1 /* OK */) {
		// イージング設定ダイアログが最後に閉じた時刻を記録する
		easing_dialog_last_closed_time = GetTime();
	}

	return res;
};



//================================
//  初期化処理
//================================

BOOL func_init(FILTER* fp) {
	auto exedit = auls::Exedit_GetFilter(fp);
	if (!exedit) {
		show_error(TEXT("拡張編集が見つかりませんでした。"));
	}
	else {
		char exedit_path[1024] = {};
		GetModuleFileName(exedit->dll_hinst, exedit_path, sizeof(exedit_path));
		DialogBoxParamA_original = (DialogBoxParamA_t)yulib::RewriteFunction(exedit_path, "DialogBoxParamA", DialogBoxParamA_hooked);
		if (DialogBoxParamA_original == nullptr) {
			show_error("初期化に失敗しました。");
		}
		else {
			exedit_WndProc_original = exedit->func_WndProc;
			exedit->func_WndProc = exedit_WndProc_hooked;
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
	.information = (TCHAR*)(PLUGIN_NAME PLUGIN_NAME_SUFFIX TEXT(" v") PLUGIN_VERSION)
};

EXTERN_C __declspec(dllexport) FILTER_DLL* __stdcall GetFilterTable(void) {
	return &filter;
}
