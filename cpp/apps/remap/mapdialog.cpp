

#include <qmessagebox.h>
#include <qcheckbox.h>
#include <qlineedit.h>
#include <qpushbutton.h>

#include "mapdialog.h"
#include "windows.h"
MapDialog::MapDialog( QWidget * parent )
: QDialog( parent )
{
	setupUi(this);
	connect( mMapButton, SIGNAL( clicked() ), SLOT( map() ) );
	connect( mUnmapButton, SIGNAL( clicked() ), SLOT( unmap() ) );
	connect( mDriveEdit, SIGNAL( textChanged( const QString & ) ), SLOT( updateStatus() ) );
}


QString getWindowsErrorMessage( DWORD id )
{
	WCHAR * buffer;
	QString ret;

	int len = FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER, 0, id, 0, (WCHAR*)&buffer, 0, 0 );
	
	if( len )
		ret = QString::fromUtf16( (const short unsigned int*)buffer );
	return ret;
}

void MapDialog::updateStatus()
{
	char share[1024];
	DWORD size = sizeof(share);
	QString drive = mDriveEdit->text() + ":";
	DWORD result = WNetGetConnectionA( drive.toLatin1(), share, &size );
	QString status;
	bool useShare = true;
	if( result != NO_ERROR ) {
		useShare = false;
		switch( result ) {
			case ERROR_BAD_DEVICE:
				status = "ERROR_BAD_DEVICE";
				break;
			case ERROR_NOT_CONNECTED:
				status = "ERROR_NOT_CONNECTED";
				break;
			case ERROR_MORE_DATA:
				status = "ERROR_MORE_DATA";
				break;
			case ERROR_CONNECTION_UNAVAIL:
				status = "ERROR_CONNECTION_UNAVAIL";
				useShare = true;
				break;
			case ERROR_NO_NETWORK:
				status = "ERROR_NO_NETWORK";
				break;
			case ERROR_EXTENDED_ERROR:
				status = "ERROR_EXTENDED_ERROR";
				break;
			default:
				status = "Unknown Error";
		};
	}
	if( useShare ) {
		if( !status.isEmpty() ) status += ": ";
		status += QString::fromLatin1(share);
	}
	mStatusEdit->setText(status);
}

void MapDialog::unmap()
{
	QString drive = mDriveEdit->text() + ":";
	DWORD flags = 0;
	if( mUnmapUpdateProfileCheck->isChecked() )
		flags = CONNECT_UPDATE_PROFILE;
	DWORD dwResult = WNetCancelConnection2( (TCHAR*)drive.utf16(), flags, mForceUnmountCheck->isChecked() );

	if( dwResult != NO_ERROR && dwResult != ERROR_NOT_CONNECTED ) {
		QMessageBox::warning( this, "Unmap Failed", "Unable to unmap drive: " + drive + " error was: " + getWindowsErrorMessage(dwResult) );
	}
	updateStatus();
}

void MapDialog::map()
{
	QString drive = mDriveEdit->text();
	if( drive.size() == 1 )
		drive += ':';

	NETRESOURCE nr; 
	nr.dwScope = RESOURCE_CONNECTED;
	nr.dwType = RESOURCETYPE_DISK;
	nr.dwDisplayType = RESOURCEDISPLAYTYPE_SHARE;
	nr.dwUsage = RESOURCEUSAGE_CONNECTABLE;
	nr.lpLocalName = (TCHAR*)drive.utf16();
	nr.lpRemoteName = (TCHAR*)mShareEdit->text().utf16();
	nr.lpComment = 0;
	nr.lpProvider = 0;

	DWORD flags = 0;
	if( mMapUpdateProfileCheck->isChecked() )
		flags = CONNECT_UPDATE_PROFILE;

	// Call the WNetAddConnection2 function to make the connection,
	DWORD dwResult = WNetAddConnection2(&nr,
		0,                  // no password 
		0,                  // logged-in user 
		flags);
	
	delete [] (TCHAR*)nr.lpLocalName;
	delete [] (TCHAR*)nr.lpRemoteName;

	// Process errors.
	//  The local device is already connected to a network resource.
	//
	if( dwResult != NO_ERROR )
		QMessageBox::warning( this, "Map Failed", getWindowsErrorMessage( dwResult ) );

	updateStatus();
}