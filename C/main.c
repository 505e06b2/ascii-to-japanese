#include "table.h"

#include <windows.h>
#include <stdio.h>

INPUT input = {0};

void sendKey(unsigned char c, char down) {
	input.ki.wVk = c;
	input.ki.dwFlags = (down) ? 0 : KEYEVENTF_KEYUP;
	SendInput(1, &input, sizeof(input));
}

void sendShortcut(char c) {
	sendKey(VK_CONTROL, 1);
	sendKey(c, 1);
	sendKey(c, 0);
	sendKey(VK_CONTROL, 0);
	Sleep(32);
}

unsigned char *toUTF8(const wchar_t *str, size_t *size) { //you must free memory after using
	size_t needed = WideCharToMultiByte(CP_UTF8, 0, str, *size, NULL, 0, NULL, NULL);
	*size = needed;
	unsigned char *out = malloc(needed * sizeof(unsigned char));
	WideCharToMultiByte(CP_UTF8, 0, str, *size, out, needed, NULL, NULL);
	return out;
}

void log_file(const wchar_t *old, const wchar_t *new, const size_t size) {
	size_t utflen = size;
	unsigned char *text = toUTF8(old, &utflen);
	FILE *f = fopen("log.txt", "ab");
	fwrite(">>> ", sizeof(char), 4, f);
	fwrite(text, sizeof(unsigned char), utflen, f);
	free(text);
	fwrite("\n<<< ", sizeof(char), 5, f);
	utflen = size;
	text = toUTF8(new, &utflen);
	fwrite(text, sizeof(unsigned char), utflen, f);
	free(text);
	fwrite("\n", sizeof(char), 1, f);
	fclose(f);
}

//https://stackoverflow.com/questions/9149600/global-keyboard-hooks-in-c
LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
	if(nCode < 0) return CallNextHookEx(NULL, nCode, wParam, lParam); //this isn't for us
	
	KBDLLHOOKSTRUCT *pKeyBoard = (KBDLLHOOKSTRUCT *)lParam;
	if(pKeyBoard->flags & LLKHF_EXTENDED && pKeyBoard->vkCode == VK_RETURN) { //if numpad enter pressed
		if(wParam == WM_KEYDOWN) return 1;
		
		sendShortcut('A');
		sendShortcut('X');
		
		if (!OpenClipboard(NULL)) {
			puts("Couldn't open clipboard");
			return 1;
		}
		const wchar_t *clipboard = GetClipboardData(CF_UNICODETEXT);
		CloseClipboard();
		
		size_t clipboard_len = wcslen(clipboard)+1;
		HGLOBAL newClip_hwdl =  GlobalAlloc(GMEM_MOVEABLE, clipboard_len * sizeof(wchar_t));
		{
			wchar_t *newClip_mem = GlobalLock(newClip_hwdl);
				for(unsigned int i = 0; i < clipboard_len; i++) {
					newClip_mem[i] = (clipboard[i] <= 127 && clipboard[i] >= 32) ? table[clipboard[i]] : clipboard[i];
				}
			//log_file(clipboard, newClip_mem, clipboard_len-1); // <=== LOG TO FILE
			GlobalUnlock(newClip_hwdl);
		}
		OpenClipboard(NULL);
		EmptyClipboard();
		SetClipboardData(CF_UNICODETEXT, newClip_hwdl);
		CloseClipboard();
		
		
		sendShortcut('V');
		sendKey(VK_RETURN, 1);
		sendKey(VK_RETURN, 0);
		Sleep(32);
		
		sendKey(VK_CONTROL, 1); //or for some reason it was perma held down
		sendKey(VK_CONTROL, 0);
		
		return 1;

	}
	
	return 0;
}

int main() {
	//Setup inputs
	input.type = INPUT_KEYBOARD;
	input.ki.dwFlags = 0;

	HHOOK hookhandle = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, GetModuleHandle(NULL), 0); //hook system
	if(!hookhandle) {
		puts("Couldn't hook keyboard...\nExiting...");
		return 1;
	} else puts("Hooked into keyboard\nRunning...\nPress the Enter key on your numpad to replace the currently typed text\nCheck log.txt for info");
	
	MSG message;
	while(GetMessage(&message, NULL, 0, 0) != 0) {
		TranslateMessage(&message);
		DispatchMessage(&message);
	}
	
	UnhookWindowsHookEx(hookhandle);
	
	return 0;
}