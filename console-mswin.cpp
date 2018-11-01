#include <windows.h>
#include <iostream>

void retractCursor()
{
  HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
  CONSOLE_SCREEN_BUFFER_INFO csbi;
  BOOL completed=FALSE;
  if (hStdout && GetConsoleScreenBufferInfo(hStdout, &csbi))
  {
    COORD coord;
    coord.X = 0;
    coord.Y = csbi.dwCursorPosition.Y;
    completed=SetConsoleCursorPosition(hStdout, coord);
  }
  
  if (completed==FALSE) 
  {
    std::cout << "\r";
  }
}

