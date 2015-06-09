

#include <qfiledialog.h>
#include <qfontdialog.h>
#include <qprinter.h>
#include <qprintdialog.h>
#include <qprocess.h>

#include "blurqt.h"
#include "scheduleprintdialog.h"
#include "schedulewidget.h"

#ifdef Q_OS_WIN
#include "windows.h"
#endif // Q_OS_WIN

// Taken from qt qpagesetupdialog_unix.cpp
static const char * const pageSizeNames[] = {
    "A4 (210 x 297 mm, 8.26 x 11.7 inches)",
    "B5 (176 x 250 mm, 6.93 x 9.84 inches)",
    "Letter (8.5 x 11 inches, 216 x 279 mm)",
    "Legal (8.5 x 14 inches, 216 x 356 mm)",
    "Executive (7.5 x 10 inches, 191 x 254 mm)",
    "A0 (841 x 1189 mm)",
    "A1 (594 x 841 mm)",
    "A2 (420 x 594 mm)",
    "A3 (297 x 420 mm)",
    "A5 (148 x 210 mm)",
    "A6 (105 x 148 mm)",
    "A7 (74 x 105 mm)",
    "A8 (52 x 74 mm)",
    "A9 (37 x 52 mm)",
    "B0 (1000 x 1414 mm)",
    "B1 (707 x 1000 mm)",
    "B2 (500 x 707 mm)",
    "B3 (353 x 500 mm)",
    "B4 (250 x 353 mm)",
    "B6 (125 x 176 mm)",
    "B7 (88 x 125 mm)",
    "B8 (62 x 88 mm)",
    "B9 (44 x 62 mm)",
    "B10 (31 x 44 mm)",
    "C5E (163 x 229 mm)",
    "US Common #10 Envelope (105 x 241 mm)",
    "DLE (110 x 220 mm)",
    "Folio (210 x 330 mm)",
    "Ledger (432 x 279 mm)",
    "Tabloid (279 x 432 mm)",
	//"Custom",
    0
};

class FontItem : public QTreeWidgetItem
{
public:
	FontItem( const QString & name, QFont * font, QTreeWidget * tw )
	: QTreeWidgetItem( tw )
	, mFont( font )
	{
		setText( 0, name );
		refreshFont();
	}

	void refreshFont()
	{
		setText( 1, QString::number( mFont->pointSize() ) + " Point, " + mFont->family() );
		setFont( 1, *mFont );
	}
		
	QFont * mFont;
};

static void setupFontItem( QTreeWidget * tw, const QString & name, QFont * font )
{
	new FontItem( name, font, tw );
}

SchedulePrintDialog::SchedulePrintDialog( ScheduleWidget * widget )
: QDialog( widget )
, mScheduleWidget( widget )
{
	setupUi(this);
	for( int i=0; pageSizeNames[i]; i++ )
		mPageSizeCombo->addItem( pageSizeNames[i] );
	connect( mPreviewButton, SIGNAL( clicked() ), SLOT( preview() ) );
	connect( mPrintButton, SIGNAL( clicked() ), SLOT( print() ) );
	connect( mSavePdfButton, SIGNAL( clicked() ), SLOT( savePdf() ) );

	mFontTree->setHeaderLabels( QStringList() << "Location" << "Font" );
	connect( mFontTree, SIGNAL( itemDoubleClicked( QTreeWidgetItem *, int ) ), SLOT( updateFont( QTreeWidgetItem * ) ) );
	
	mDisplayOptions = widget->mDisplayOptions;
	ScheduleDisplayOptions & dops = mDisplayOptions;
	setupFontItem( mFontTree, "Header Month/Year", &dops.headerMonthYearFont );
	setupFontItem( mFontTree, "Header Day/Week", &dops.headerDayWeekFont );
	setupFontItem( mFontTree, "Panel", &dops.panelFont );
	setupFontItem( mFontTree, "Cell Day", &dops.cellDayFont );
	setupFontItem( mFontTree, "Entry", &dops.entryFont );
	setupFontItem( mFontTree, "Entry Hours", &dops.entryHoursFont );
}

void SchedulePrintDialog::updateFont( QTreeWidgetItem * item )
{
	FontItem * fi = (FontItem*)item;
	bool ok;
	QFont newFont = QFontDialog::getFont( &ok, *fi->mFont, this );
	if( ok ) {
		*fi->mFont = newFont;
		fi->refreshFont();
	}
}

void SchedulePrintDialog::setupPrinter( QPrinter & printer )
{
	printer.setPageSize( QPrinter::PageSize(mPageSizeCombo->currentIndex()) );
	printer.setOrientation( mOrientationCombo->currentText() == "Landscape" ? QPrinter::Landscape : QPrinter::Portrait );
}

void SchedulePrintDialog::setupPdfPrinter( QPrinter & printer )
{
	printer.setOutputFormat( QPrinter::PdfFormat );
	setupPrinter(printer);

	printer.setFullPage( true );
	printer.setResolution( mDpiSpin->value() );
	printer.setFontEmbeddingEnabled(false);
}

void SchedulePrintDialog::preview()
{
	QPrinter printer;
	setupPdfPrinter(printer);
#ifdef Q_OS_WIN
	QString filename = "c:\\temp\\print.pdf";
#else
	QString filename = "/tmp/print.pdf";
#endif

	printer.setOutputFileName( filename );
	doPrint(&printer);
	
#ifdef Q_OS_WIN
	ShellExecuteW( 0, L"open", (WCHAR*)filename.utf16(), 0, L".", SW_SHOWNORMAL );
#else
	QProcess::startDetached( "kfmclient openURL " + filename );
#endif // Q_OS_WIN
}

void SchedulePrintDialog::savePdf()
{
	QPrinter printer;
	setupPdfPrinter(printer);
	QString fileName = QFileDialog::getSaveFileName(this, "Choose File Name for Pdf", QString(), "Pdf Documents (*.pdf)" );
	printer.setOutputFileName( fileName );
	doPrint(&printer);
}

void SchedulePrintDialog::print()
{
	QPrinter printer;
	setupPrinter(printer);
	QPrintDialog pd(&printer,this);
	if( pd.exec() == QDialog::Accepted )
		doPrint(&printer);
}

void SchedulePrintDialog::doPrint(QPrinter * printer)
{
	ScheduleWidget::PrintOptions po;
	if( mHeaderGroup->isChecked() ) {
		po.leftHeaderText = mLeftHeaderText->text();
		po.middleHeaderText = mMiddleHeaderText->text();
		po.rightHeaderText = mRightHeaderText->text();
	}
	if( mFooterGroup->isChecked() ) {
		po.leftFooterText = mLeftFooterText->text();
		po.middleFooterText = mMiddleFooterText->text();
		po.rightFooterText = mRightFooterText->text();
	}
	po.avoidSplitRows = true;
	po.printHeader = !mNoHeaderRadio->isChecked();
	po.printPanel = !mNoTreeRadio->isChecked();
	po.headerOnEveryPage = mEveryHeaderRadio->isChecked();
	po.panelOnEveryPage = mEveryTreeRadio->isChecked();
	po.horizontalPages = mHorizontalPagesSpin->value();
	po.verticalPages = mVerticalPagesSpin->value();
	ScheduleDisplayOptions saved = mScheduleWidget->mDisplayOptions;
	mScheduleWidget->mDisplayOptions = mDisplayOptions;
	mScheduleWidget->print(printer,po);
	mScheduleWidget->mDisplayOptions = saved;
}

