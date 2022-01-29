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
//  �g���ҏW�̓��������ւ̊���
//================================

UINT64 easing_dialog_last_closed_time = 0;
UINT64 EASING_DIALOG_COOL_TIME = 2000000; // 0.2s


// �C�x���g�����Ɋ��荞��

BOOL(*exedit_WndProc_original)(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam, void* editp, void* fp);
using exedit_WndProc_t = decltype(exedit_WndProc_original);
BOOL exedit_WndProc_hooked(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam, void* editp, void* fp) {
	const auto res = exedit_WndProc_original(hwnd, message, wparam, lparam, editp, fp);

	// �C�[�W���O�ݒ�̕ύX��ߑ�
	if (message == WM_COMMAND && wparam == 1003 /* OBJECT_DIALOG_MENU */) {
		const int trackbar_id = lparam - 0x80000;
		if (0 <= trackbar_id && trackbar_id < auls::EXEDIT_OBJECT::MAX_TRACK) {
			if (GetTime() - easing_dialog_last_closed_time > EASING_DIALOG_COOL_TIME) { // �N�[���^�C����݂��Ȃ��Ɩ������[�v�ɂȂ�
				// �C�[�W���O�p�����[�^�ݒ�_�C�A���O��\��
				SendMessage(auls::ObjDlg_GetWindow(hwnd), WM_COMMAND, 0x462, trackbar_id);
			}
		}
	}

	return res;
}


// �e��_�C�A���O�\�������Ɋ��荞��

INT_PTR(WINAPI* DialogBoxParamA_original)(_In_opt_ HINSTANCE hInstance, _In_ LPCSTR lpTemplateName, _In_opt_ HWND hWndParent, _In_opt_ DLGPROC lpDialogFunc, _In_ LPARAM dwInitParam);
using DialogBoxParamA_t = decltype(DialogBoxParamA_original);
INT_PTR WINAPI DialogBoxParamA_hooked(_In_opt_ HINSTANCE hInstance, _In_ LPCSTR lpTemplateName, _In_opt_ HWND hWndParent, _In_opt_ DLGPROC lpDialogFunc, _In_ LPARAM dwInitParam) {
	const auto res = DialogBoxParamA_original(hInstance, lpTemplateName, hWndParent, lpDialogFunc, dwInitParam);

	if (_tcscmp(lpTemplateName, TEXT("GET_PARAM_SMALL")) == 0 && res == 1 /* OK */) {
		// �C�[�W���O�ݒ�_�C�A���O���Ō�ɕ����������L�^����
		easing_dialog_last_closed_time = GetTime();
	}

	return res;
};



//================================
//  ����������
//================================

BOOL func_init(FILTER* fp) {
	auto exedit = auls::Exedit_GetFilter(fp);
	if (!exedit) {
		show_error(TEXT("�g���ҏW��������܂���ł����B"));
	}
	else {
		char exedit_path[1024] = {};
		GetModuleFileName(exedit->dll_hinst, exedit_path, sizeof(exedit_path));
		DialogBoxParamA_original = (DialogBoxParamA_t)yulib::RewriteFunction(exedit_path, "DialogBoxParamA", DialogBoxParamA_hooked);
		if (DialogBoxParamA_original == nullptr) {
			show_error("�������Ɏ��s���܂����B");
		}
		else {
			exedit_WndProc_original = exedit->func_WndProc;
			exedit->func_WndProc = exedit_WndProc_hooked;
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
	.information = (TCHAR*)(PLUGIN_NAME PLUGIN_NAME_SUFFIX TEXT(" v") PLUGIN_VERSION)
};

EXTERN_C __declspec(dllexport) FILTER_DLL* __stdcall GetFilterTable(void) {
	return &filter;
}
