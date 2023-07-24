#ifndef RESOURCE_H_
#define RESOURCE_H_

#define APP_NAME L"Brainfuck for SHARP Brain"
#define APP_DESCRIPTION L"Brainfuck interpreter and debugger for SHARP Brain."
#define APP_COPYRIGHT L"(C) 2022-2023 watamario15 (https://github.com/watamario15)"
#define APP_VERSION L"1.0 RC4"

#define VER_STR_COMPANYNAME "watamario15\0"
#define VER_STR_DESCRIPTION "Brainfuck for SHARP Brain\0"
#define VER_STR_FILEVERSION 0, 0, 0, 4
#define VER_STR_APPNAME "Brainfuck for SHARP Brain\0"
#define VER_STR_COPYRIGHT "(C) 2022-2023 watamario15 (https://github.com/watamario15)\0"
#define VER_STR_ORIGINALFILENAME "brainfuck-<platform>-<architecture>.exe\0"
#define VER_STR_VERSION "1.0 RC4\0"

#define IDC_EDITOR 0
#define IDC_INPUT 1
#define IDC_OUTPUT 2
#define IDC_MEMVIEW 3
#define IDC_CMDBTN_FIRST 4
#define CMDBTN_LEN 4
#define IDC_SCRKBD_FIRST 8
#define SCRKBD_LEN 9

#define IDM_FILE_NEW 100
#define IDM_FILE_OPEN 110
#define IDM_FILE_SAVE 120
#define IDM_FILE_SAVE_AS 130
#define IDM_FILE_EXIT 140
#define IDM_EDIT_UNDO 200
#define IDM_EDIT_REDO 210
#define IDM_EDIT_CUT 220
#define IDM_EDIT_COPY 230
#define IDM_EDIT_PASTE 240
#define IDM_EDIT_SELALL 250
#define IDM_BF_MEMTYPE_SIGNED 300
#define IDM_BF_MEMTYPE_UNSIGNED 301
#define IDM_BF_OUTPUT_UTF8 310
#define IDM_BF_OUTPUT_SJIS 311
#define IDM_BF_OUTPUT_HEX 312
#define IDM_BF_INPUT_UTF8 320
#define IDM_BF_INPUT_SJIS 321
#define IDM_BF_INPUT_HEX 322
#define IDM_BF_NOINPUT_ERROR 330
#define IDM_BF_NOINPUT_ZERO 331
#define IDM_BF_NOINPUT_FF 332
#define IDM_BF_INTOVF_ERROR 340
#define IDM_BF_INTOVF_WRAPAROUND 341
#define IDM_BF_BREAKPOINT 350
#define IDM_OPT_SPEED_FASTEST 400
#define IDM_OPT_SPEED_1MS 401
#define IDM_OPT_SPEED_10MS 402
#define IDM_OPT_SPEED_100MS 403
#define IDM_OPT_MEMVIEW 410
#define IDM_OPT_TRACK 420
#define IDM_OPT_HLTPROG 430
#define IDM_OPT_HLTMEM 440
#define IDM_OPT_DARK 450
#define IDM_OPT_LAYOUT 460
#define IDM_OPT_FONT 470
#define IDM_OPT_WORDWRAP 480
#define IDM_ABOUT 500

#endif
