#ifdef _WIN32_WINNT
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0500
#endif

#define WM_MOUSEWHEEL 0x020A

#include <tchar.h>
#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include "resource.h"


// application variables
HINSTANCE      hInstance = 0;
HACCEL         haccel = 0;
WNDPROC        oldSubClassProc = NULL;
HWND           hwnd_main = 0;
HWND           hwnd_edit = 0;
HWND           hwnd_status = 0;
HWND           hwnd_find = 0;
LONG           num_lines = 0;
LONG           line_num = 0;
LONG           lines_per_page = 0;
int            col_num = 0;
_TCHAR         full_path[MAX_PATH];
BOOL           file_modified = FALSE;
int            line_height = 0;
const _TCHAR   *app_name = _T("TEdit");
const _TCHAR   *class_name = _T("TEdit_class");
const int      window_x_size = 600;
const int      window_y_size = 400;
long           default_word_break_func;

DWORD          EDIT_STYLE_SCROLL =  WS_CHILD|WS_VISIBLE|ES_MULTILINE|ES_WANTRETURN|
                                    WS_VSCROLL|ES_AUTOVSCROLL|
                                    WS_HSCROLL|ES_AUTOHSCROLL;

DWORD          EDIT_STYLE_NOSCROLL = WS_CHILD|WS_VISIBLE|ES_MULTILINE|ES_WANTRETURN|
                                     WS_VSCROLL|ES_AUTOVSCROLL;

_TCHAR         szDbg[255];

// font variables
int            font_pt_size;
_TCHAR         font_name[255];
HFONT          hfont_text = NULL;

// common dialog variables
OPENFILENAME   open_file_struct;
CHOOSEFONT     choose_font_struct;
LOGFONT        log_font_struct;

// find/replace variables
FINDREPLACE    find_replace;
UINT           find_message_id;
_TCHAR         find_str[100];
int            find_str_len = 0;
_TCHAR         replace_str[100];
int            replace_str_len = 0;
int            find_index = 0;
int            find_buf_start_index = 0;
int            find_char_x_start = 0;
int            find_char_y_start = 0;
int            find_char_x_end = 0;
int            find_char_y_end = 0;
BOOL           find_text_highlighted = FALSE;


// highlight variables
COLORREF       highlight_color = RGB(216,200,228);
COLORREF       highlight_text_color = RGB(0,0,0);
HBRUSH         highlight_brush = 0;

// prototypes
LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK AboutDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
BOOL DisplayOpenFileDialog(HWND hwndOwner, _TCHAR *szFullPath, int nSizeFullPath);
BOOL DisplaySaveFileDialog(HWND hwndOwner, _TCHAR *szFullPath, int nSizeFullPath);
LRESULT CALLBACK SubClassProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
int CALLBACK EditWordBreakProc(LPTSTR lpch, int ichCurrent, int cch, int code);
int  OnCreate(HWND hwnd);
void OnCommand(HWND hwnd, WORD wNotifyCode, WORD wID, HWND hwndCtl);
void OnPaint(HWND hwnd, HDC hdc);
void OnSize(HWND hwnd, WPARAM fwSizeType, WORD nWidth, WORD nHeight);
void OnMouseWheel(HWND hwnd, WORD fwKeys, short zDelta, short xPos, short yPos);
void OnClose(HWND hwnd);
void OnDestroy(HWND hwnd);

void update_status(HWND hwnd_edit, HWND hwnd_status);
HFONT create_font(HDC hdc, int ptsize, const _TCHAR *face_name);
WORD get_caret_pos(HWND hwnd_edit, int *col_num);
void restore_caret(void);
void set_word_wrap(BOOL new_word_wrap);
BOOL open_file(_TCHAR *full_path);
BOOL save_file(_TCHAR *full_path);
void update_caption(HWND hwnd, _TCHAR *full_path);

_TCHAR *my_strstr(const _TCHAR *string, const _TCHAR *strCharSet, BOOL case_sensitive);
_TCHAR *my_strrstr(_TCHAR *start, int len, _TCHAR *szFind, BOOL case_sensitive);
void highlight_selection(HWND hwnd_edit, DWORD *start, DWORD *end, BOOL highlight);
//void highlight_text(HWND hwnd_edit, _TCHAR *szFind, int start_index, int end_index, BOOL highlight);


int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInstnce, LPSTR lpCmdLine, int nShowCmd)
{
   WNDCLASS wc;
   MSG  msg;
   HDC hdc;
   //RECT rect;
   LPTSTR cmdLine;
   LPWSTR *cmdLineArray;
   int numArgs;

   hInstance = hInst;

   //InitCommonControls();

   wc.cbClsExtra = 0;
   wc.cbWndExtra = 0;
   wc.hbrBackground = (HBRUSH)(COLOR_APPWORKSPACE+1);
   wc.hCursor = 0;
   wc.hIcon = LoadIcon(hInstance,MAKEINTRESOURCE(IDI_APP_ICON)); // TBD is this right?
   wc.hInstance = 0;
   wc.lpfnWndProc = WndProc;
   wc.lpszClassName = class_name;
   wc.lpszMenuName = MAKEINTRESOURCE(IDR_MAIN_MENU);
   wc.style = 0;
   if( !RegisterClass(&wc) )
   {
      MessageBox(NULL,_T("Error registering class"),app_name,MB_OK);
      return 0;
   }

   hwnd_main = CreateWindowEx(0,class_name,app_name,WS_OVERLAPPEDWINDOW,CW_USEDEFAULT,CW_USEDEFAULT,window_x_size,window_y_size,NULL,NULL,hInst,NULL);
   if( !hwnd_main )
   {
      MessageBox(NULL,_T("Error creating window"),app_name,MB_OK);
      return 0;
   }
   
   hwnd_edit = CreateWindowEx(WS_EX_CLIENTEDGE,_T("EDIT"),_T(""),EDIT_STYLE_SCROLL,0,0,0,0,hwnd_main,NULL,hInst,NULL);
   if( !hwnd_edit )
   {
      MessageBox(NULL,_T("Error creating edit box"),app_name,MB_OK);
      return 0;
   }

   //hwnd_status = CreateStatusWindow(WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS,_T(""),hwnd_main,IDC_STATUSBAR);
   //GetClientRect(hwnd_main,&rect);
   hwnd_status =   CreateWindowEx(0, 
                                  STATUSCLASSNAME, 
                                  _T(""), 
                                  WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS, 
                                  0, 0, 0, 20, 
                                  hwnd_main, (HMENU) IDC_STATUSBAR, 
                                  hInstance, NULL); 
   if( !hwnd_status )
   {
      MessageBox(NULL,_T("Error creating status bar"),app_name,MB_OK);
      return 0;
   }
   
   // subclass the edit control
   oldSubClassProc = (WNDPROC)GetWindowLong(hwnd_edit,GWL_WNDPROC);
   SetWindowLong(hwnd_edit,GWL_WNDPROC,(LONG)SubClassProc);

   // setup word break proc
   default_word_break_func = (long)SendMessage(hwnd_edit,EM_GETWORDBREAKPROC,(WPARAM)0,(LPARAM)EditWordBreakProc); 
   SendMessage(hwnd_edit,EM_SETWORDBREAKPROC,(WPARAM)0,(LPARAM)EditWordBreakProc);

   // default font characteristics
   _tcscpy(font_name,_T("Courier"));
   font_pt_size = 12;

   // create a font for the edit control
   hdc = GetDC(hwnd_edit);
   hfont_text = create_font(hdc,font_pt_size,font_name);
   SendMessage(hwnd_edit,WM_SETFONT,(WPARAM)hfont_text,0);
   ReleaseDC(hwnd_edit,hdc);

   // setup the status bar control
   //SendMessage(hwnd_status,SB_SIMPLE,(WPARAM)TRUE,0);
   //SendMessage(hwnd_status,SB_SETMINHEIGHT,0,0);

   ShowWindow(hwnd_main,SW_SHOW);
   UpdateWindow(hwnd_main);

   // load the accelerator table
   haccel = LoadAccelerators(hInst, MAKEINTRESOURCE(IDR_ACCELERATOR));

   // process command line
   cmdLine = GetCommandLine();
   cmdLineArray = CommandLineToArgvW(cmdLine,&numArgs);
   if( numArgs > 1 )
   {
      if( _tcslen(cmdLineArray[1]) > 0 )
      {
         _TCHAR *cmd_ptr;
         int last_char;

         cmd_ptr = cmdLine;

         if( cmdLine[0] == '"' )
         {
            cmd_ptr = &cmdLine[1];
         }

         _tcscpy(full_path,cmd_ptr);
         last_char = (int)_tcslen(full_path) - sizeof(_TCHAR);

         if( full_path[last_char] == '"' )
            full_path[last_char] = 0;

         if( open_file(full_path) )
         {
            num_lines = (long)SendMessage(hwnd_edit,EM_GETLINECOUNT,0,0);
            update_status(hwnd_edit,hwnd_status);
            update_caption(hwnd_main,full_path);
            restore_caret();
         }
         else
         {
            MessageBox(HWND_DESKTOP,_T("Error opening file"),app_name,MB_OK|MB_ICONSTOP);
         }
      }
   }
   LocalFree(cmdLineArray);

   SetFocus(hwnd_edit);
   
   //
   // Message processing loop
   //
   while( GetMessage(&msg, NULL, 0, 0) ) 
   {
	   if (TranslateAccelerator(hwnd_main, haccel, &msg))
	   {
		   continue;
	   }

		if ((hwnd_find == 0) || !IsDialogMessage(hwnd_find, &msg))
	   {
		   {
			   TranslateMessage(&msg);
			   DispatchMessage(&msg);
		   }
	   }

	   //if ((hwnd_find == 0) || !IsDialogMessage(hwnd_find, &msg))
	   //{
	//	   if (!TranslateAccelerator(hwnd_main, haccel, &msg))
	//	   {
	//		   TranslateMessage(&msg);
	//		   DispatchMessage(&msg);
	//	   }
	 //  }

      //if( hwnd_find == 0 || 
		//  !IsDialogMessage(hwnd_find, &msg) == 0 &&
        //  !TranslateAccelerator(hwnd_main, haccel, &msg) ) 
      //{ 
      //   TranslateMessage(&msg);
      //   DispatchMessage(&msg);
      //}
   }

   return 0;
}

_TCHAR *my_strstr(const _TCHAR *string, const _TCHAR *strCharSet, BOOL case_sensitive) 
{
   return _tcsstr(string, strCharSet);
}

_TCHAR *my_strrstr(_TCHAR *start, int len, _TCHAR *szFind, BOOL case_sensitive)
{
   _TCHAR *p;
   int i;
   int find_len;

   p = start;
   i = len;
   find_len = (int)_tcslen(szFind);

   while( i > 0 )
   {
      if( case_sensitive )
      {
         if( _tcsncmp(p,szFind,find_len) == 0 )
         {
            return p;
         }
      }
      else
      {
         if( _tcsnicmp(p,szFind,find_len) == 0 )
         {
            return p;
         }
      }
      p--;
      i--;
   }

   return 0;
}

int my_find(_TCHAR *find_buf, int len, int start_index, _TCHAR *szFind, BOOL search_down, BOOL case_sensitive)
{
   _TCHAR *p;
   _TCHAR *start;

   start = find_buf + start_index;

   if( search_down )
   {
      p = my_strstr(start,szFind,case_sensitive);
      if( p )
      {
         if( p >= find_buf )
            return (int)(p - find_buf);
      }
   }
   else
   {
      p = my_strrstr(start,len,szFind,case_sensitive);
      if( p )
      {
         if( p >= find_buf )
            return (int)(p - find_buf);
      }
   
   }
   return -1;
}

void highlight_selection(HWND hwnd_edit, DWORD *start, DWORD *end, BOOL highlight)
{
   DWORD   pos;
   DWORD   dwStart = 0;
   DWORD   dwEnd = 0;
   WORD    char_x_start = 0;
   WORD    char_y_start = 0;
   WORD    char_x_end = 0;
   WORD    char_y_end = 0;
   WORD    char_x_end_line = 0;
   WORD    char_y_end_line = 0;
   TEXTMETRIC tm;
   HDC     hdc;
   SIZE    size;
   HLOCAL  hand;
   _TCHAR    *ptr;
   int     len;
   RECT    rect_client;
   int     i;
   DWORD   x;
   int     pos_y;
   DWORD   start_pos;
   int     num_lines;
   RECT    rect_highlight;
   _TCHAR    *text_line;
   int     text_line_len;
   //SCROLLINFO si;

   SendMessage(hwnd_edit,EM_GETSEL,(WPARAM)&dwStart,(LPARAM)&dwEnd);
   //_stprintf(szDbg,_T("start %d end %d\n"),dwStart,dwEnd);
   //OutputDebugString(szDbg);
   *start = dwStart;
   *end   = dwEnd;
   if( dwStart == dwEnd )
      return;

   //si.cbSize = sizeof(si);
   //si.fMask = SIF_POS;
   //GetScrollInfo(hwnd_edit,SB_HORZ,&si);
   //if( si.nPos != 0 ) // bail out if we're scrolled horizontally
   //   return;

   GetClientRect(hwnd_edit,&rect_client);
   // set clipping ?
   //rgn = CreateRectRgn();
   //SelectObject(hdc,rgn);
   //DeleteObject(rgn);

   pos = (int)SendMessage(hwnd_edit,EM_POSFROMCHAR,(WPARAM)dwStart,(LPARAM)0);
   char_x_start = LOWORD(pos);
   char_y_start = HIWORD(pos);
   _stprintf(szDbg,_T("char start %d %d\n"),char_x_start, char_y_start);
   OutputDebugString(szDbg);
   if( char_x_start > rect_client.right || (short)char_x_start < 0 )
   {
      //char_x_start = 0;
      //OutputDebugString(_T("char_x_start = 0\n"));
      //si.cbSize = sizeof(si);
      //si.fMask = SIF_POS;
      //si.nPos  = 0;
      //SetScrollInfo(hwnd_edit,SB_HORZ,&si,TRUE);
      //ScrollWindowEx(hwnd_edit,-20,0,NULL,&rect_client,NULL,NULL,SW_INVALIDATE);

      //EM_LINESCROLL 
      //wParam = (WPARAM) cxScroll; // characters to scroll horizontally 
      //lParam = (LPARAM) cyScroll
      int num_chars;
      num_chars = (65536 - (int)char_x_start)/10 + 2;// TBD width = 10?
      if( num_chars > 0 )
      {
         _stprintf(szDbg,_T("scrolling left %d chars\n"),num_chars);
         OutputDebugString(szDbg);
         SendMessage(hwnd_edit,EM_LINESCROLL,-num_chars,0);
         //InvalidateRect(hwnd_edit,NULL,TRUE);
         SendMessage(hwnd_edit,EM_SETSEL,(WPARAM)dwStart,(LPARAM)dwEnd);
      }
   }
   if( char_y_start > rect_client.bottom )
   {
      //char_y_start = 0;
      //OutputDebugString(_T("char_y_start = 0\n"));
   }

   pos = (int)SendMessage(hwnd_edit,EM_POSFROMCHAR,(WPARAM)dwEnd,(LPARAM)0);
   char_x_end = LOWORD(pos);
   char_y_end = HIWORD(pos);
   _stprintf(szDbg,_T("char pos end %d %d\n"),char_x_end, char_y_end);
   OutputDebugString(szDbg);
   if( char_x_end == 0xFFFF || char_y_end == 0xFFFF )
   {
      int last_char;
      last_char = GetWindowTextLength(hwnd_edit);
      pos = (int)SendMessage(hwnd_edit,EM_POSFROMCHAR,(WPARAM)last_char-1,(LPARAM)0);
      char_x_end = LOWORD(pos) + 16;//tbd fix this constant
      char_y_end = HIWORD(pos);
   }

   if( char_x_start > char_x_end )
      return;
   if( char_y_start > char_y_end )
      return;


   hand = (HLOCAL)SendMessage(hwnd_edit,EM_GETHANDLE,0,0);
   if( hand )
   {
      ptr = LocalLock(hand);
      if( ptr )
      {
         len = dwEnd - dwStart;

         hdc = GetDC(hwnd_edit);
         if( hdc )
         {
            GetTextMetrics(hdc,&tm);
            line_height = tm.tmHeight;
            if( highlight )
            {
               SelectObject(hdc,highlight_brush);
               SetTextColor(hdc,highlight_text_color);
            }
            else
            {
            }
            SelectObject(hdc,GetStockObject(NULL_PEN));
            SetBkMode(hdc,TRANSPARENT);
            SelectObject(hdc,hfont_text);
            GetTextExtentPoint32(hdc,&ptr[dwStart],len,&size);
            //_stprintf(szDbg,_T("str size %d %d\n"),size.cx,size.cy);
            //OutputDebugString(szDbg);

            num_lines = (char_y_end - char_y_start) / size.cy + 1;
            //_stprintf(szDbg,_T("num lines selected %d\n"),num_lines);
            //OutputDebugString(szDbg);

            start_pos = dwStart;
            pos_y = char_y_start;

            for( i=0; i<num_lines; i++ )
            {
               // find end of line -- look for CR
               for( x=start_pos; x<dwEnd; x++ )
               {
                  if( ptr[x] == 0x0a || ptr[x] == 0 )
                     break;
               }

               text_line_len = x - start_pos;
               text_line = malloc((text_line_len+1)*sizeof(_TCHAR));
               _tcsncpy(text_line,&ptr[start_pos],text_line_len);
               text_line[text_line_len] = 0;
               //OutputDebugString(_T("text line: "));
               //OutputDebugString(text_line);
               //OutputDebugString(_T("\n"));
               start_pos = x+1;

               if( i == 0 )
               {
                  //
                  // First line of multiple line selection
                  //

                  // get end line coords
                  pos = (int)SendMessage(hwnd_edit,EM_POSFROMCHAR,(WPARAM)x,(LPARAM)0);
                  char_x_end_line = LOWORD(pos);

                  if( char_x_end_line == 0xFFFF )
                  {
                     int last_char;
                     last_char = GetWindowTextLength(hwnd_edit);
                     pos = (int)SendMessage(hwnd_edit,EM_POSFROMCHAR,(WPARAM)last_char-1,(LPARAM)0);
                     char_x_end_line = LOWORD(pos) + line_height;
                  }
                  else if( char_x_end_line > rect_client.right )
                  {
                     char_x_end_line = (WORD)rect_client.right;
                  }

                  rect_highlight.left = char_x_start;
                  rect_highlight.top = char_y_start;
                  rect_highlight.right = char_x_end_line;
                  rect_highlight.bottom = char_y_start + line_height;
                  pos_y += line_height;
               }
               else if( i == (num_lines-1) )
               {
                  //
                  // Last line of multiple line selection
                  //
                  pos = (int)SendMessage(hwnd_edit,EM_POSFROMCHAR,(WPARAM)dwEnd,(LPARAM)0);
                  char_x_end_line = LOWORD(pos);
                  char_y_end_line = HIWORD(pos);

                  if( char_x_end_line == 0xFFFF )
                  {
                     int last_char;
                     last_char = GetWindowTextLength(hwnd_edit);
                     pos = (int)SendMessage(hwnd_edit,EM_POSFROMCHAR,(WPARAM)last_char-1,(LPARAM)0);
                     char_x_end_line = LOWORD(pos) + 16;//tbd fix this constant
                  }
                  else if( char_x_end_line > rect_client.right )
                  {
                     char_x_end_line = (WORD)rect_client.right;
                  }

                  rect_highlight.left = 1;
                  rect_highlight.top = char_y_end;
                  rect_highlight.right = char_x_end_line;
                  rect_highlight.bottom = char_y_end + line_height;
               }
               else
               {
                  //
                  // Middle lines of multiple line selection
                  //

                  // get end line coords
                  pos = (int)SendMessage(hwnd_edit,EM_POSFROMCHAR,(WPARAM)x,(LPARAM)0);
                  char_x_end_line = LOWORD(pos);

                  if( char_x_end_line == 0xFFFF )
                  {
                     int last_char;
                     last_char = GetWindowTextLength(hwnd_edit);
                     pos = (int)SendMessage(hwnd_edit,EM_POSFROMCHAR,(WPARAM)last_char-1,(LPARAM)0);
                     char_x_end_line = LOWORD(pos) + line_height;
                  }
                  else if( char_x_end_line > rect_client.right )
                  {
                     char_x_end_line = (WORD)rect_client.right;
                  }

                  rect_highlight.left = 1;
                  rect_highlight.top = pos_y;
                  rect_highlight.right = char_x_end_line;
                  rect_highlight.bottom = pos_y + line_height;
                  pos_y += line_height;
               }

               Rectangle(hdc,rect_highlight.left,rect_highlight.top,rect_highlight.right,rect_highlight.bottom);
               DrawText(hdc,text_line,text_line_len,&rect_highlight,DT_LEFT|DT_EXPANDTABS);
               free(text_line);
            }
            ReleaseDC(hwnd_edit,hdc);
         }

         LocalUnlock(hand);
      }
   }
}
#if 0
void highlight_text(HWND hwnd_edit, _TCHAR *szFind, 
                    int start_index, int end_index, BOOL highlight)
{
   //_TCHAR    szOut[100];
   DWORD   pos;
   TEXTMETRIC tm;
   HDC     hdc;
   RECT    rect_highlight;

   //InvalidateRect(hwnd_edit,NULL,TRUE);

   //_stprintf(szOut,_T("highlight_text %d %d\n"),start_index,end_index);
   //OutputDebugString(szOut);

   pos = SendMessage(hwnd_edit,EM_POSFROMCHAR,(WPARAM)start_index,(LPARAM)0);
   find_char_x_start = LOWORD(pos);
   find_char_y_start = HIWORD(pos);
   //_stprintf(szDbg,_T("char pos start %d at %d,%d\n"),start_index, find_char_x_start, find_char_y_start);
   //OutputDebugString(szDbg);

   pos = SendMessage(hwnd_edit,EM_POSFROMCHAR,(WPARAM)end_index,(LPARAM)0);
   find_char_x_end = LOWORD(pos);
   find_char_y_end = HIWORD(pos);
   //_stprintf(szDbg,_T("char pos start %d at %d,%d\n"), end_index, find_char_x_end, find_char_y_end);
   //OutputDebugString(szDbg);

   hdc = GetDC(hwnd_edit);
   if( hdc )
   {
      GetTextMetrics(hdc,&tm);
      line_height = tm.tmHeight;
      SelectObject(hdc,highlight_brush);
      SelectObject(hdc,GetStockObject(NULL_PEN));
      SetBkMode(hdc,TRANSPARENT);
      SetTextColor(hdc,highlight_text_color);
      SelectObject(hdc,hfont_text);

      rect_highlight.left = find_char_x_start;
      rect_highlight.top = find_char_y_start;
      rect_highlight.right = find_char_x_end;
      rect_highlight.bottom = find_char_y_end+line_height;

      if( highlight )
      {
         Rectangle(hdc,rect_highlight.left,rect_highlight.top,rect_highlight.right,rect_highlight.bottom);
         DrawText(hdc,szFind,-1,&rect_highlight,DT_LEFT|DT_EXPANDTABS);
      }
      else
      {
         SetTextColor(hdc,RGB(0,0,0));
         SelectObject(hdc,GetStockObject(WHITE_BRUSH));
         Rectangle(hdc,rect_highlight.left,rect_highlight.top,rect_highlight.right,rect_highlight.bottom);
         DrawText(hdc,szFind,-1,&rect_highlight,DT_LEFT|DT_EXPANDTABS);
      }

      ReleaseDC(hwnd_edit,hdc);
   }
}
#endif

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) 
{
   HLOCAL hand;
   _TCHAR   *find_ptr;
   DWORD  dwStart;
   DWORD  dwEnd;
   BOOL   search_down;
   int    len = 0;
   int    start_index;
   BOOL   case_sensitive;
   DWORD  start;
   DWORD  end;

   if( message == find_message_id )
   {
      LPFINDREPLACE lpfr = (LPFINDREPLACE) lParam;
      if( lpfr )
      {
         if( lpfr->Flags & FR_DIALOGTERM )
         {
            //OutputDebugString(_T("FR_DIALOGTERM\n"));
            restore_caret();
            if( find_buf_start_index )
            {
               SendMessage(hwnd_edit,EM_SETSEL,(WPARAM)find_buf_start_index,(LPARAM)find_buf_start_index+find_str_len);
               SendMessage(hwnd_edit,EM_SCROLLCARET,0,0);
            }
            hwnd_find = 0;
            find_text_highlighted = FALSE;
         }
         else if( lpfr->Flags & FR_FINDNEXT )
         {
            //OutputDebugString(_T("FR_FINDNEXT\n"));

            //
            // Get location of caret so we know where to start searching.
            //
            SendMessage(hwnd_edit,EM_GETSEL,(WPARAM)(LPDWORD)&dwStart,(LPARAM)(LPDWORD)&dwEnd);

            //
            // Setup depending on direction
            //
            if( lpfr->Flags & FR_DOWN )
            {
               search_down = TRUE;
               len = GetWindowTextLength(hwnd_edit);
               start_index = dwEnd;
            }
            else
            {
               search_down = FALSE;
               len = dwEnd;
               start_index = dwStart - 1;;
            }

            if( lpfr->Flags & FR_MATCHCASE )
            {
               case_sensitive = TRUE;
            }
            else
            {
               case_sensitive = FALSE;
            }
            

            // Dehighlight old text if any
            if( find_text_highlighted )
            {
               //highlight_text(hwnd_edit,find_str,find_buf_start_index,find_buf_start_index+find_str_len,FALSE);
               highlight_selection(hwnd_edit, &start, &end, FALSE);
               find_text_highlighted = FALSE;
            }
            
            hand = (HLOCAL)SendMessage(hwnd_edit,EM_GETHANDLE,0,0);
            if( hand )
            {
               find_ptr = LocalLock(hand);
               if( find_ptr )
               {
                  // get the find text from the dialog
                  _tcscpy(find_str,lpfr->lpstrFindWhat);
                  find_str_len = (int)_tcslen(find_str);

                  find_index = my_find(find_ptr,len,start_index,find_str,search_down,case_sensitive);
                  //_stprintf(szDbg,_T("find_index %d\n"),find_index);
                  //OutputDebugString(szDbg);

                  if( find_index >= 0 )
                  {
                     find_buf_start_index = find_index;
                     SendMessage(hwnd_edit,EM_SETSEL,(WPARAM)find_index,(LPARAM)find_index + find_str_len);
                     SendMessage(hwnd_edit,EM_SCROLLCARET,0,0);
                     update_status(hwnd_edit,hwnd_status);
                     //find_buf_start_index++;
                     //highlight_text(hwnd_edit,find_str,find_index,find_index+find_str_len,TRUE);
                     highlight_selection(hwnd_edit, &start, &end, TRUE);
                     find_text_highlighted = TRUE;
                  }
                  else
                  {
                     MessageBox(hwnd_edit,_T("Text not found"),app_name,MB_OK);
                     SetFocus(hwnd_find);
                  }
                  LocalUnlock(hand);
               }
               else
               {
                  MessageBox(hwnd_edit,_T("Error!"),app_name,MB_OK);
               }
            }
            else
            {
               MessageBox(hwnd_edit,_T("Error!"),app_name,MB_OK);
            }
         }
         else if( lpfr->Flags & FR_REPLACE )
         {
            //OutputDebugString(_T("FR_REPLACE\n"));
         }
         else if( lpfr->Flags & FR_REPLACEALL )
         {
            //OutputDebugString(_T("FR_REPLACEALL\n"));
         }
      }
   }
   else
   {
      switch (message)
      { 
         case WM_CREATE:
            OnCreate(hwnd);
            return 0;

         case WM_PAINT:
            OnPaint(hwnd,(HDC)wParam);
            break;

         case WM_SIZE:
            OnSize(hwnd,wParam,LOWORD(lParam),HIWORD(lParam));
            break;

         case WM_ERASEBKGND:
            // prevent flicker
            return 0;

         case WM_COMMAND:
            OnCommand(hwnd,HIWORD(wParam),LOWORD(wParam),(HWND)lParam);
            return 0;

         case WM_SETFOCUS:
            SetFocus(hwnd_edit);
            break;

         //case WM_KILLFOCUS:
         //   return 0;

         case WM_CLOSE:
            OnClose(hwnd);
            break;

         case WM_DESTROY:
            OnDestroy(hwnd);
            break;

         default:
            break;
      } 
   }

   return DefWindowProc(hwnd, message, wParam, lParam);
}


int OnCreate(HWND hwnd)
{
   find_message_id = RegisterWindowMessage(FINDMSGSTRING);
   highlight_brush = CreateSolidBrush(highlight_color);
   return 0;
}


void OnPaint(HWND hwnd, HDC hdc)
{
   PAINTSTRUCT ps;
   HDC hdcDmy;
   DWORD start;
   DWORD end;

   hdcDmy = BeginPaint(hwnd, &ps);
   if( GetForegroundWindow() != hwnd )
   {
      highlight_selection(hwnd_edit, &start, &end, TRUE);
      //if( find_text_highlighted )
      //   highlight_text(hwnd_edit,find_str,find_buf_start_index,find_buf_start_index+find_str_len,TRUE);
   }
   EndPaint(hwnd, &ps);
} 

void OnSize(HWND hwnd, WPARAM fwSizeType, WORD nWidth, WORD nHeight)
{
   RECT rect;
   RECT rect_status;
   int nParts = 2;
   int aWidths[2];

   SendMessage(hwnd_status,WM_SIZE,fwSizeType,MAKELPARAM(nWidth,nHeight));
   aWidths[0] = nWidth - 200;
   aWidths[1] = -1;
   SendMessage(hwnd_status,SB_SETPARTS,(WPARAM) nParts,(LPARAM) (LPINT) aWidths);
   GetClientRect(hwnd,&rect);
   GetClientRect(hwnd_status,&rect_status);
   SetWindowPos(hwnd_edit,NULL,0,0,rect.right,rect.bottom-rect_status.bottom,SWP_NOMOVE|SWP_NOZORDER);
}

void OnMouseWheel(HWND hwnd, WORD fwKeys, short zDelta, short xPos, short yPos)
{
   //OutputDebugString(_T("OnMouseWheel\n"));
}

void OnClose(HWND hwnd)
{
   int resp = IDYES;
   if( file_modified )
   {
      resp = MessageBox(hwnd,_T("File has changed. Save?"),app_name,MB_ICONQUESTION|MB_YESNOCANCEL);
      if( resp == IDYES )
      {
         save_file(full_path);
         file_modified = FALSE;
         update_caption(hwnd,full_path);
      }
   }
   if( resp == IDYES || resp == IDNO )
      DestroyWindow(hwnd);
}

void OnDestroy(HWND hwnd)
{
   if( hfont_text )
   {
      //SelectObject(...);
      DeleteObject(hfont_text);
      hfont_text = 0;
   }
   if( highlight_brush )
   {
      //SelectObject(...);
      DeleteObject(highlight_brush);
      highlight_brush = 0;
   }
   PostQuitMessage(0); 
}

void OnCommand(HWND hwnd, WORD wNotifyCode, WORD wID, HWND hwndCtl)
{
   BOOL status;
   HDC  hdc;

   if( wNotifyCode == EN_CHANGE )
   {
      update_status(hwnd_edit,hwnd_status);
   }
   else if( wID == IDM_FILE_OPEN )
   {
      if( DisplayOpenFileDialog(hwnd,full_path,sizeof(full_path)) )
      {
         if( open_file(full_path) )
         {
            num_lines = (int)SendMessage(hwnd_edit,EM_GETLINECOUNT,0,0);
            update_status(hwnd_edit,hwnd_status);
            update_caption(hwnd, full_path);
            //EnableWindow(hwnd_hscroll,TRUE);
         }
         else
         {
            MessageBox(hwnd,_T("Error opening file"),app_name,MB_OK|MB_ICONSTOP);
         }
      }
      restore_caret();
   }
   else if( wID == IDM_FILE_EXIT )
   {
      OnClose(hwnd);
   }
   else if( wID == IDM_FILE_SAVE )
   {
      if( _tcslen(full_path) == 0 )
      {
         status = DisplaySaveFileDialog(hwnd,full_path,sizeof(full_path));
      }
      else
      {
         status = TRUE;
      }
      if( status )
      {
         save_file(full_path);
         file_modified = FALSE;
         update_caption(hwnd,full_path);
      }
      restore_caret();
   }
   else if( wID == IDM_FILE_SAVE_AS )
   {
      if( DisplaySaveFileDialog(hwnd,full_path,sizeof(full_path)) )
      {
         save_file(full_path);
         file_modified = FALSE;
         update_caption(hwnd, full_path);
      }
      restore_caret();
   }
   else if( wID == IDM_FILE_PRINT )
   {
      MessageBox(hwnd_main,_T("Printing not implemented yet"),app_name,MB_ICONINFORMATION|MB_OK);
   }
   else if( wID == IDM_EDIT_FIND )
   {
      DWORD dwStart;
      DWORD dwEnd;
      HLOCAL hand;

      SendMessage(hwnd_edit,EM_GETSEL,(WPARAM)(LPDWORD)&dwStart,(LPARAM)(LPDWORD)&dwEnd);
      //_stprintf(szDbg,_T("GetSel start %ld end %ld\n"),dwStart,dwEnd);
      //OutputDebugString(szDbg);
      if( ((dwStart > 0) || (dwEnd > 0)) && (dwStart != dwEnd) )
      {
         _TCHAR *ptr;
         hand = (HLOCAL)SendMessage(hwnd_edit,EM_GETHANDLE,0,0);
         if( hand )
         {
            ptr = LocalLock(hand);
            if( ptr )
            {
               find_str_len = dwEnd - dwStart;
               if( find_str_len > sizeof(find_str) )
                  find_str_len = sizeof(find_str) - 1;
               _tcsncpy(find_str,&ptr[dwStart],find_str_len);
               find_str[find_str_len] = 0;
               LocalUnlock(hand);
            }
         }
      }

      find_index = 0;
      find_buf_start_index = 0;
      find_text_highlighted = FALSE;

      ZeroMemory(&find_replace,sizeof(find_replace));
      find_replace.lStructSize = sizeof(FINDREPLACE);
      find_replace.hwndOwner = hwnd;
      find_replace.hInstance = NULL;
      find_replace.Flags     = FR_DOWN|FR_HIDEWHOLEWORD;
      find_replace.lpstrFindWhat = find_str;
      find_replace.lpstrReplaceWith = replace_str;
      find_replace.wFindWhatLen = sizeof(find_str);
      find_replace.wReplaceWithLen = sizeof(replace_str);
      find_replace.lCustData = 0; 
      find_replace.lpfnHook = NULL; 
      find_replace.lpTemplateName = NULL; 
      hwnd_find = FindText(&find_replace);
   }
   else if( wID == IDM_FORMAT_WORD_WRAP )
   {
      HMENU hmenu = GetMenu(hwnd);
      UINT  menu_state;

      menu_state = GetMenuState(hmenu,IDM_FORMAT_WORD_WRAP,MF_BYCOMMAND);

      if( menu_state == MF_CHECKED )
      {
         CheckMenuItem(hmenu,IDM_FORMAT_WORD_WRAP,MF_BYCOMMAND|MF_UNCHECKED);
         set_word_wrap(FALSE);
      }
      else
      {
         CheckMenuItem(hmenu,IDM_FORMAT_WORD_WRAP,MF_BYCOMMAND|MF_CHECKED);
         set_word_wrap(TRUE);
      }
      restore_caret();
   }
   else if( wID == IDM_FORMAT_FONT )
   {
      hdc = GetDC(hwnd);
      ZeroMemory(&log_font_struct,sizeof(LOGFONT));
      log_font_struct.lfHeight = -MulDiv(font_pt_size, GetDeviceCaps(hdc, LOGPIXELSY), 72); 
      _tcscpy(log_font_struct.lfFaceName,font_name);

      ZeroMemory(&choose_font_struct,sizeof(CHOOSEFONT));
      choose_font_struct.lStructSize = sizeof(CHOOSEFONT);
      choose_font_struct.hwndOwner   = hwnd;
      choose_font_struct.lpLogFont   = &log_font_struct; 
      choose_font_struct.Flags       = CF_SCREENFONTS|CF_INITTOLOGFONTSTRUCT; 
      if( ChooseFont(&choose_font_struct) )
      {
         // new font chosen
         //OutputDebugString(_T("new font choosen: "));
         //OutputDebugString(log_font_struct.lfFaceName);
         //OutputDebugString(_T("\n"));
         SelectObject(hdc,GetStockObject(SYSTEM_FONT));
         DeleteObject(hfont_text);
         font_pt_size = choose_font_struct.iPointSize/10;
         _tcscpy(font_name,log_font_struct.lfFaceName); 
         hfont_text = create_font(hdc,font_pt_size,font_name);
         SendMessage(hwnd_edit,WM_SETFONT,(WPARAM)hfont_text,0);
         InvalidateRect(hwnd_edit,NULL,TRUE);
      }
      ReleaseDC(hwnd,hdc);
      restore_caret();
   }
   else if( wID == IDM_EDIT_SELECTALL )
   {
      SendMessage(hwnd_edit,EM_SETSEL,0,-1);
   }
   else if( wID == IDM_HELP_ABOUT )
   {
      DialogBox(hInstance,MAKEINTRESOURCE(IDD_ABOUT),hwnd,(DLGPROC)AboutDlgProc);
   }
}

BOOL DisplayOpenFileDialog(HWND hwndOwner, _TCHAR *szFullPath, int nSizeFullPath)
{
   // variables for Open File common dialog
   _TCHAR   *szFilter = _T("Text files\0*.txt\0All files\0*.*\0");
   _TCHAR   *szDefaultExt = _T("txt");
   _TCHAR   szImageFile[84];

   szFullPath[0] = 0;
   open_file_struct.lStructSize = sizeof(OPENFILENAME);
   open_file_struct.hwndOwner = hwndOwner;
   open_file_struct.hInstance = NULL; 
   open_file_struct.lpstrFilter = szFilter;
   open_file_struct.lpstrCustomFilter = NULL; 
   open_file_struct.nMaxCustFilter = 0;
   open_file_struct.nFilterIndex = 0; 
   open_file_struct.lpstrFile = szFullPath;
   open_file_struct.nMaxFile = nSizeFullPath; 
   open_file_struct.lpstrFileTitle = szImageFile;
   open_file_struct.nMaxFileTitle = sizeof(szImageFile); 
   open_file_struct.lpstrInitialDir = NULL;
   open_file_struct.lpstrTitle = NULL; 
   open_file_struct.Flags = OFN_FILEMUSTEXIST | OFN_NONETWORKBUTTON; 
   open_file_struct.nFileOffset = 0; 
   open_file_struct.nFileExtension = 0;
   open_file_struct.lpstrDefExt = szDefaultExt; 
   open_file_struct.lCustData = 0;
   open_file_struct.lpfnHook = NULL; 
   open_file_struct.lpTemplateName = NULL;
   if( GetOpenFileName(&open_file_struct) )
      return TRUE;
   return FALSE;
}

BOOL DisplaySaveFileDialog(HWND hwndOwner, _TCHAR *szFullPath, int nSizeFullPath)
{
   // variables for Open File common dialog
   _TCHAR   *szFilter = _T("Text files\0*.txt\0");
   _TCHAR   *szDefaultExt = _T("txt");
   _TCHAR   szImageFile[84];

   szFullPath[0] = 0;
   open_file_struct.lStructSize = sizeof(OPENFILENAME);
   open_file_struct.hwndOwner = hwndOwner;
   open_file_struct.hInstance = NULL; 
   open_file_struct.lpstrFilter = szFilter;
   open_file_struct.lpstrCustomFilter = NULL; 
   open_file_struct.nMaxCustFilter = 0;
   open_file_struct.nFilterIndex = 0; 
   open_file_struct.lpstrFile = szFullPath;
   open_file_struct.nMaxFile = nSizeFullPath; 
   open_file_struct.lpstrFileTitle = szImageFile;
   open_file_struct.nMaxFileTitle = sizeof(szImageFile); 
   open_file_struct.lpstrInitialDir = NULL;
   open_file_struct.lpstrTitle = NULL; 
   open_file_struct.Flags = OFN_FILEMUSTEXIST | OFN_NONETWORKBUTTON; 
   open_file_struct.nFileOffset = 0; 
   open_file_struct.nFileExtension = 0;
   open_file_struct.lpstrDefExt = szDefaultExt; 
   open_file_struct.lCustData = 0;
   open_file_struct.lpfnHook = NULL; 
   open_file_struct.lpTemplateName = NULL;
   if( GetSaveFileName(&open_file_struct) )
      return TRUE;
   return FALSE;
}

//
// this doesn't work if file is large
//
#if 0
WORD get_caret_pos(HWND hwnd_edit, int *col_num)
{
   WORD wLineNumber;
   WORD wLineIndex;
   DWORD dwGetSel;
   DWORD dwFirstVisibleLine;
   WORD wStart;

   dwFirstVisibleLine = (DWORD)SendMessage(hwnd_edit,EM_GETFIRSTVISIBLELINE,0,0L);
   //_stprintf(szDbg,_T("dwFirstVisibleLine %ld\n"),dwFirstVisibleLine);
   //OutputDebugString(szDbg);

   // Send the EM_GETSEL message to the edit control. 
   // The high-order word of the return value is the 
   // character position of the caret relative to the 
   // first character in the control. 
   dwGetSel = (DWORD)SendMessage(hwnd_edit,EM_GETSEL,0,0L);
   wStart = HIWORD(dwGetSel);
   //_stprintf(szDbg,_T("dwGetSel 0x%08x wStart 0x%04x (%d)\n"),dwGetSel,wStart,wStart);
   //OutputDebugString(szDbg);

   // Send the EM_LINEFROMCHAR message to the edit control 
   // and specify the value returned from step 1 as wParam. 
   // Add 1 to the return value to get the line number of the 
   // caret position because Windows numbers the lines starting 
   // at zero. 
   wLineNumber = (WORD)SendMessage(hwnd_edit,EM_LINEFROMCHAR,wStart,0L);
   wLineNumber++;
   //_stprintf(szDbg,_T("wLineNumber %d\n"),wLineNumber);
   //OutputDebugString(szDbg);

   // Send the EM_LINEINDEX message with the value of -1 in wParam.
   // The return value is the absolute number of characters
   // that precede the first character in the line containing
   // the caret.
   wLineIndex = (WORD)SendMessage(hwnd_edit,EM_LINEINDEX,-1,0L);
   //_stprintf(szDbg,_T("wLineIndex %d\n"),wLineIndex);
   //OutputDebugString(szDbg);

   // Subtract the LineIndex from the start of the selection,
   // and then add 1 (since the column is zero-based).
   // This result is the column number of the caret position.
   *col_num = wStart - wLineIndex + 1;
   //_stprintf(szDbg,_T("col_num %d\n"),*col_num);
   //OutputDebugString(szDbg);
   
   return wLineNumber;
}
#endif

WORD get_caret_pos(HWND hwnd_edit, int *col_num)
{
   WORD  wLineNumber;
   WORD  wLineIndex;
   DWORD dwStart;
   DWORD dwEnd;
   DWORD dwGetSel;
   WORD  wStart;
   DWORD dwFirstVisibleLine;
   DWORD pos;
   int   char_y_start;
   WORD  line;
   

   dwFirstVisibleLine = (DWORD)SendMessage(hwnd_edit,EM_GETFIRSTVISIBLELINE,0,0L);
   //_stprintf(szDbg,_T("dwFirstVisibleLine %ld\n"),dwFirstVisibleLine);
   //OutputDebugString(szDbg);

   SendMessage(hwnd_edit,EM_GETSEL,(WPARAM)&dwStart,(LPARAM)&dwEnd);
   //_stprintf(dbg_buf,_T("start %d end %d\n"),dwStart,dwEnd);
   //OutputDebugString(szDbg);

   pos = (int)SendMessage(hwnd_edit,EM_POSFROMCHAR,(WPARAM)dwStart,(LPARAM)0);
   char_y_start = HIWORD(pos);
   if( char_y_start == 0xFFFF )
   {
      int last_char;
      last_char = GetWindowTextLength(hwnd_edit);
      pos = (int)SendMessage(hwnd_edit,EM_POSFROMCHAR,(WPARAM)last_char-1,(LPARAM)0);
      char_y_start = HIWORD(pos);
      line = char_y_start / 16 + 1; // TBD fix this
   }
   else
   {
      line = char_y_start / 16; // TBD fix this
   }

   wLineNumber = line + (WORD)dwFirstVisibleLine + 1;
   //_stprintf(szDbg,_T("line %d wLineNumber %d\n"),line,wLineNumber);
   //OutputDebugString(szDbg);


   // Send the EM_GETSEL message to the edit control. 
   // The high-order word of the return value is the 
   // character position of the caret relative to the 
   // first character in the control. 
   dwGetSel = (DWORD)SendMessage(hwnd_edit,EM_GETSEL,0,0L);
   wStart = HIWORD(dwGetSel);

   // Send the EM_LINEINDEX message with the value of -1 in wParam.
   // The return value is the absolute number of characters
   // that precede the first character in the line containing
   // the caret.
   wLineIndex = (WORD)SendMessage(hwnd_edit,EM_LINEINDEX,-1,0L);
   //_stprintf(szDbg,_T("wLineIndex %d\n"),wLineIndex);
   //OutputDebugString(szDbg);

   // Subtract the LineIndex from the start of the selection,
   // and then add 1 (since the column is zero-based).
   // This result is the column number of the caret position.
   *col_num = wStart - wLineIndex + 1;

   return wLineNumber;
}

static void OnKeyDown(HWND hwnd, WPARAM nVirtKey, LPARAM lKeyData)
{
   if( nVirtKey == VK_UP || nVirtKey == VK_DOWN || nVirtKey == VK_LEFT || nVirtKey == VK_RIGHT || nVirtKey == VK_HOME || nVirtKey == VK_END || nVirtKey == VK_NEXT || nVirtKey == VK_PRIOR )
   {
      update_status(hwnd_edit,hwnd_status);
   }
   else
   {
      if( (file_modified == FALSE) && (_tcslen(full_path) > 0) )
      {
         BOOL mod_status;
         mod_status = (BOOL)SendMessage(hwnd,EM_GETMODIFY,0,0);
         if( mod_status )
         {
            file_modified = TRUE;
            update_caption(hwnd_main,full_path);
         }
      }
   }
   //_stprintf(szDbg,_T("nVirtKey %x lKeyData %x\n"),nVirtKey,lKeyData);
   //OutputDebugString(szDbg);
}

LRESULT CALLBACK SubClassProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
   LRESULT result;
   static DWORD dwStart = 0;
   static DWORD dwEnd = 0;

   // Don't allow edit box to process kill focus message, it will
   // remove the current selection
   //if( message == WM_KILLFOCUS )
   //   return 0;

   // Call default window proc before processing so edit control is updated with
   // current info.
   result = CallWindowProc(oldSubClassProc, hwnd, message, wParam, lParam);

   switch(message)
   {
      case WM_KEYDOWN:
         OnKeyDown(hwnd,wParam,lParam);
         break;

      case WM_LBUTTONDOWN:
         update_status(hwnd_edit,hwnd_status);
         break;

      case WM_MOUSEWHEEL:
         OnMouseWheel(hwnd,LOWORD(wParam),HIWORD(wParam),(short) LOWORD(lParam),(short) HIWORD(lParam));  
         break;

      case WM_SETFOCUS:
         //_stprintf(szDbg,_T("WM_SETFOCUS\n"));
         //OutputDebugString(szDbg);
         if( hwnd_find == 0 )
            SendMessage(hwnd_edit,EM_SETSEL,(WPARAM)dwStart,(LPARAM)dwEnd);
         break;

      case WM_KILLFOCUS:
         highlight_selection(hwnd_edit,&dwStart,&dwEnd, TRUE);
         break;
   }

   return result;
}

void update_status(HWND hwnd_edit, HWND hwnd_status)
{
   _TCHAR buf[64];

   line_num = (LONG)get_caret_pos(hwnd_edit,&col_num);
   _stprintf(buf,_T("Line: %d   Col: %d"),line_num,col_num);
   //SendMessage(hwnd_status,SB_SETTEXT,(WPARAM)255,(LPARAM)(LPSTR)buf);// simple
   SendMessage(hwnd_status,SB_SETTEXT,(WPARAM)1,(LPARAM)(LPSTR)buf);// two parts
}


HFONT create_font(HDC hdc, int ptsize, const _TCHAR *face_name)
{
   HFONT hfont;
   int   height;
   DWORD pitch_ff = FF_DONTCARE|VARIABLE_PITCH;
   DWORD fdwQuality = DEFAULT_QUALITY;// was: NONANTIALIASED_QUALITY;

   height = -MulDiv(ptsize, GetDeviceCaps(hdc, LOGPIXELSY), 72);
   hfont = CreateFont(height,             // logical height of font
                      0,                  // logical average character width
                      0,                  // angle of escapement
                      0,                  // base-line orientation angle
                      FW_NORMAL,            // font weight
                      FALSE,                  // italic attribute flag
                      FALSE,                  // underline attribute flag
                      FALSE,                  // strikeout attribute flag
                      DEFAULT_CHARSET,        // character set identifier
                      OUT_TT_PRECIS, // output precision
                      CLIP_DEFAULT_PRECIS,  // clipping precision
                      fdwQuality,        // output quality
                      pitch_ff,  // pitch and family
                      face_name);         // pointer to typeface name string

   return hfont;
}

BOOL open_file(_TCHAR *full_path)
{
   HANDLE hfile;
   LONG number_of_bytes_read;
   BOOL status;
   BYTE first2Bytes[2];
   BOOL unicode_file = FALSE;
   int  start_offset = 0;
   int  last_offset = 0;

   hfile = CreateFile(full_path,          // pointer to name of the file
                      GENERIC_READ,       // access (read-write) mode
                      FILE_SHARE_READ,    // share mode
                      NULL,               // pointer to security attributes
                      OPEN_EXISTING,      // how to create
                      FILE_ATTRIBUTE_NORMAL,  // file attributes
                      NULL                // handle to file with attributes to copy
                      );
   if( hfile != INVALID_HANDLE_VALUE )
   {
      LONG file_size_low;
      LONG file_size_high;
      _TCHAR *read_buf;
      char *read_buf_char;

      file_size_low = GetFileSize(hfile,&file_size_high);
      if( file_size_high != 0 )
      {
         MessageBox(NULL,_T("File too big!"),app_name,MB_OK);
         CloseHandle(hfile);
         return FALSE;
      }
      else if( file_size_low != 0xFFFFFFFF )
      {
         //
         // Determine if we are dealing with a Unicode file
         //
         status = ReadFile(hfile,first2Bytes,2,&number_of_bytes_read,NULL);
         if( status && number_of_bytes_read == 2 )
         {
            if( first2Bytes[0] == 0xff && first2Bytes[1] == 0xfe )
            {
               // Unicode file
               unicode_file = TRUE;
            }
         }

         if( unicode_file )
         {
            read_buf = malloc((file_size_low+1)*sizeof(_TCHAR));
            status = ReadFile(hfile,read_buf,file_size_low,&number_of_bytes_read,NULL);
            if( number_of_bytes_read >= 0 && number_of_bytes_read <= file_size_low )
            {
               last_offset = number_of_bytes_read/sizeof(_TCHAR);
               read_buf[last_offset] = 0;
            }
            SetWindowText(hwnd_edit,read_buf);
            free(read_buf);
         }
         else
         {
            // If this is not a Unicode file we need to keep the first two chars
            read_buf_char = malloc(file_size_low+1);
            memcpy(read_buf_char,first2Bytes,2);
            start_offset = 2;
            status = ReadFile(hfile,&read_buf_char[2],file_size_low,&number_of_bytes_read,NULL);
            if( number_of_bytes_read >= 0 && number_of_bytes_read <= file_size_low )
            {
               last_offset = number_of_bytes_read;
               read_buf_char[last_offset] = 0;
            }

            // TODO: convert to TCHAR for display
            // For now call the ASCII specific function
            SetWindowTextA(hwnd_edit,read_buf_char);
            free(read_buf_char);
         }
      }
      else
      {
         CloseHandle(hfile);
         return FALSE;
      }

      CloseHandle(hfile);
   }
   else
   {
      return FALSE;
   }

   return TRUE;

}

BOOL save_file(_TCHAR *full_path)
{
   HANDLE hfile;

   hfile = CreateFile(full_path,          // pointer to name of the file
                      GENERIC_WRITE,       // access (read-write) mode
                      FILE_SHARE_WRITE,    // share mode
                      NULL,               // pointer to security attributes
                      CREATE_ALWAYS,      // how to create
                      FILE_ATTRIBUTE_NORMAL,  // file attributes
                      NULL                // handle to file with attributes to                                // copy
                      );
   if( hfile != INVALID_HANDLE_VALUE )
   {
      _TCHAR *write_buf;
      DWORD bytes_written;
      int buf_size;
      int actual_size;
      buf_size = GetWindowTextLength(hwnd_edit) + 1;
      write_buf = malloc(buf_size*sizeof(_TCHAR));
      if( write_buf )
      {
         actual_size = GetWindowText(hwnd_edit,write_buf,buf_size);

         WriteFile(hfile,            // handle to file to write to
                   write_buf,        // pointer to data to write to file
                   actual_size,      // number of bytes to write
                   &bytes_written,   // pointer to number of bytes written
                   NULL              // pointer to structure for overlapped I/O
                   );

         //if( number_of_bytes_read >= 0 && number_of_bytes_read <= file_size_low )
         //   read_buf[number_of_bytes_read] = 0;
         //SetWindowText(hwnd_edit,read_buf);

         free(write_buf);
      }

      CloseHandle(hfile);
   }
   
   return TRUE;
}

// set window title
void update_caption(HWND hwnd, _TCHAR *full_path)
{
   _TCHAR *window_title;
   _TCHAR *ptr;
   _TCHAR *file_name;
   _TCHAR mod_char = ' ';

   // TODO: I think there's a function to do this
   file_name = full_path;
   ptr = _tcsrchr(full_path,'\\');
   if( ptr )
   {
      file_name = ptr + 1;
   }

   window_title = malloc((_tcslen(file_name) + _tcslen(app_name) + 5)*sizeof(_TCHAR));
   if( window_title )
   {
      if( file_modified )
      {
         mod_char = '*';
      }
      _stprintf(window_title,_T("%s%c - %s"),file_name,mod_char,app_name);
      SetWindowText(hwnd,window_title);
      free(window_title);  
   }
}

void restore_caret(void)
{
   // weird thing I have to do so edit box displays its caret again.
   SetFocus(hwnd_edit);
   SendMessage(hwnd_edit,WM_SETFONT,(WPARAM)hfont_text,0);
   InvalidateRect(hwnd_edit,NULL,TRUE);
}

void set_word_wrap(BOOL new_word_wrap)
{
   _TCHAR *save_buf;
   int buf_size;
   int actual_size;
   DWORD dwStart;
   DWORD dwEnd;

   //
   // Copy all the text from the window
   //
   buf_size = GetWindowTextLength(hwnd_edit) + 1;
   save_buf = malloc(buf_size*sizeof(_TCHAR));
   if( save_buf )
   {
      actual_size = GetWindowText(hwnd_edit,save_buf,buf_size);
      //free(save_buf);
   }
   else
   {
      return; // error
   }

   //
   // Get the caret position
   //
   SendMessage(hwnd_edit,EM_GETSEL,(WPARAM)&dwStart,(LPARAM)&dwEnd);

   //
   // Destroy old edit control and create new one
   //
   DestroyWindow(hwnd_edit);

   if( new_word_wrap )
   {
      hwnd_edit = CreateWindowEx(WS_EX_CLIENTEDGE,_T("EDIT"),_T(""), EDIT_STYLE_NOSCROLL,0,0,0,0,hwnd_main,NULL,hInstance,NULL);
      SendMessage(hwnd_edit,EM_SETWORDBREAKPROC,(WPARAM)0,(LPARAM)default_word_break_func);
   }
   else
   {
      hwnd_edit = CreateWindowEx(WS_EX_CLIENTEDGE,_T("EDIT"),_T(""), EDIT_STYLE_SCROLL,0,0,0,0,hwnd_main,NULL,hInstance,NULL);
      SendMessage(hwnd_edit,EM_SETWORDBREAKPROC,(WPARAM)0,(LPARAM)EditWordBreakProc);
   }

   SetWindowText(hwnd_edit,save_buf);
   free(save_buf);

   OnSize(hwnd_main,0,0,0);

   //
   // Set the caret position
   //
   SendMessage(hwnd_edit,EM_SETSEL,(WPARAM)(int)dwStart,(LPARAM)(int)dwEnd);

}

int CALLBACK EditWordBreakProc(LPTSTR lpch, int ichCurrent, int cch, int code)
{
   int i;

   //
   // I think we'll only get two codes for standard edit boxes:
   // WB_ISDELIMITER and WB_LEFT
   //
   if( code == WB_ISDELIMITER )
      return FALSE;

   for( i=ichCurrent; i<cch; i++ )
   {
      if( lpch[i] == 0x0A )
         return i;
   }
   return 0;
}

BOOL CALLBACK AboutDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
   BOOL rtn = FALSE;
 
	switch(message)
	{
      case WM_COMMAND:
         EndDialog(hwnd,0);
         break;

      //case WM_DESTROY:
         //OnDestroy(hDlg);
         //break;

	}
	return rtn;
}
