#include <Windows.h>
#include "tchar.h"
#include "aviutl_plugin_sdk/filter.h"
#include "auls/aulslib/exedit.h"
#include "auls/yulib/extra.h"

#ifdef _DEBUG
#include "dbgstream/dbgstream.h"
#endif


#define PLUGIN_NAME TEXT("�C�[�W���O�ݒ莞�Z�v���O�C��")
#ifdef _DEBUG
#define PLUGIN_NAME_SUFFIX TEXT("�iDebug�j")
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
//  �g���ҏW�̓��������ւ̊���
//================================

POINT menu_position;


// �C�x���g�n���h�����t�b�N

BOOL(*exedit_WndProc_original)(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam, void* editp, void* fp) = nullptr;
using exedit_WndProc_t = decltype(exedit_WndProc_original);
BOOL exedit_WndProc_hooked(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam, void* editp, void* fp) {
	const auto res = exedit_WndProc_original(hwnd, message, wparam, lparam, editp, fp);

	// �C�[�W���O�ݒ�̕ύX��ߑ�
	if (message == WM_COMMAND && wparam == 1003 /* OBJECT_DIALOG_MENU */) {
		const int trackbar_id = lparam - 0x80000;
		if (0 <= trackbar_id && trackbar_id < auls::EXEDIT_OBJECT::MAX_TRACK) {

			// �������[�v�΍�
			static bool in_this_procedure = false;
			if (!in_this_procedure) {

				// �J�[�\�����{�^����Ɉړ�
				if (config.move_cursor) {
					SetCursorPos(menu_position.x, menu_position.y);
				}

				// �C�[�W���O�p�����[�^�ݒ�_�C�A���O��\��
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


// �|�b�v�A�b�v���j���[�\���������t�b�N

BOOL (WINAPI* TrackPopupMenu_original)(_In_ HMENU hMenu, _In_ UINT uFlags, _In_ int x, _In_ int y, _Reserved_ int nReserved, _In_ HWND hWnd, _Reserved_ CONST RECT* prcRect) = nullptr;
using TrackPopupMenu_t = decltype(TrackPopupMenu_original);
BOOL WINAPI TrackPopupMenu_hooked(_In_ HMENU hMenu, _In_ UINT uFlags, _In_ int x, _In_ int y, _Reserved_ int nReserved, _In_ HWND hWnd, _Reserved_ CONST RECT* prcRect) {
	// �\���ʒu�i�N���b�N�ʒu�j���L�^����
	menu_position.x = x;
	menu_position.y = y;

	return TrackPopupMenu_original(hMenu, uFlags, x, y, nReserved, hWnd, prcRect);
}



//================================
//  ����������
//================================

BOOL func_init(FILTER* fp) {
	auto exedit = auls::Exedit_GetFilter(fp);
	if (!exedit) {
		show_error(TEXT("�g���ҏW��������܂���ł����B"));
	}
	else {
		// �R���t�B�O
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


		// �e��t�b�N

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
//  �G�N�X�|�[�g���
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
