#include <Windows.h>
#include "tchar.h"
#include "aviutl_plugin_sdk/filter.h"
#include "auls/aulslib/exedit.h"
#include "auls/yulib/extra.h"

#define PLUGIN_NAME "�C�[�W���O�ݒ莞�Z�v���O�C��"
#define PLUGIN_VERSION "v1.1.0"
#define PLUGIN_DEVELOPER "�啂̖�"

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
//  �g���ҏW�̓��������ւ̊���
//================================

HHOOK hhooks[2];
HWND hwnd_exedit, hwnd_objDlg;
POINT menu_position;
bool dialogShortcutLocked = true;


// WndProc�̃t�b�N

LRESULT CALLBACK HookProc_WndProc(int nCode, WPARAM wParam, LPARAM lParam) {
	auto info = reinterpret_cast<PCWPSTRUCT>(lParam);

	if (info->hwnd == hwnd_exedit) {

		if (info->message == WM_FILTER_EXIT) {
			// �t�b�N����
			if (hhooks[0]) UnhookWindowsHookEx(hhooks[0]);
			if (hhooks[1]) UnhookWindowsHookEx(hhooks[1]);
		}

	}
	else if (info->hwnd == hwnd_objDlg) {

		if (info->message == WM_COMMAND && 4000 <= info->wParam && info->wParam < 4000 + auls::EXEDIT_OBJECT::MAX_TRACK) {
			// �C�[�W���O�{�^���N���b�N��
			if (GetKeyState(VK_MENU) < 0) {
				dialogShortcutLocked = true;
			}
			else {
				dialogShortcutLocked = false;
			}
		} else if (info->message == WM_COMMAND && info->wParam == 0x462) {
			// �C�[�W���O�p�����[�^�ݒ�_�C�A���O�o�����iAlt�N���b�N���͉��̂��������Ȃ��j
			dialogShortcutLocked = true;
		}

	}

	return CallNextHookEx(NULL, nCode, wParam, lParam);
}

LRESULT CALLBACK HookProc_WndProcRet(int nCode, WPARAM wParam, LPARAM lParam) {
	auto info = reinterpret_cast<PCWPRETSTRUCT>(lParam);

	if (info->hwnd == hwnd_exedit) {

		if (info->message == WM_FILTER_INIT) {
			
			// �I�u�W�F�N�g�ݒ�E�B���h�E���擾
			hwnd_objDlg = auls::ObjDlg_GetWindow(hwnd_exedit);

		}
		else if (info->message == WM_COMMAND && info->wParam == 1003) {

			// �C�[�W���O�ύX��
			const int trackbar_id = info->lParam - 0x80000;
			if (0 <= trackbar_id && trackbar_id < auls::EXEDIT_OBJECT::MAX_TRACK && !dialogShortcutLocked) {

				// �J�[�\�����{�^����Ɉړ�
				if (config.move_cursor) {
					SetCursorPos(menu_position.x, menu_position.y);
				}

				// �C�[�W���O�p�����[�^�ݒ�_�C�A���O��\��
				if (config.auto_popup) {
					SendMessage(auls::ObjDlg_GetWindow(hwnd_exedit), WM_COMMAND, 0x462, trackbar_id);
				}

			}

		}

	}

	return CallNextHookEx(NULL, nCode, wParam, lParam);
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
		hwnd_exedit = exedit->hwnd;

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
//  �G�N�X�|�[�g���
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
