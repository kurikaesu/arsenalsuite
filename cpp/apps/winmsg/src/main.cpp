

#include <qlist.h>

#include <windows.h>

#include "process.h"

int main(int argc, char * argv[])
{
	QList<HWND> assburners = getWindowsByName( "Assburner" );
	foreach( HWND hwnd, assburners ) {
		SendMessage( hwnd, WM_QUERYENDSESSION, (WPARAM)0, (LPARAM)ENDSESSION_LOGOFF );
		Sleep(1000);
		SendMessage( hwnd, WM_ENDSESSION, (WPARAM)0, (LPARAM)0 );
	}
	return 0;
}

