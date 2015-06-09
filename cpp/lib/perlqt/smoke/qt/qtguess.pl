#!/usr/bin/perl

# qtguess.pl : check how Qt was compiled. Issue a list of all defined QT_NO_* macros, one per line.
#
# author:  germain Garand <germain@ebooksfrance.com>
# licence: GPL v.2

# options: -q: be quieter
#	   -o file: redirect output to "file". (default: ./qtdefines)
#	   -t [0..15]: set the testing threshold (see below)
#	   -f "flags": additional compiler flags/parameters

use Getopt::Std;

use vars qw/$opt_f $opt_o $opt_p/;

getopts('qo:f:t:');

# Threshold :
#	 0 - test basic Qt types/classes
#	 5 - test higher level, non-gui classes
#	 8 - test options of the above (ex: QT_NO_IMAGE_SMOOTHSCALE)
#	10 - test basic widgets
#	12 - test composite widgets
#	13 - test widgets inheriting composite widgets
#	15 - test goodies (default)

my $default_threshold = 0;
my $cc = "g++";
my $ccflags = $opt_f || "-Wnon-virtual-dtor -Wno-long-long -Wundef -Wall -pedantic -W -DQT_DLL -DUNICODE -DQT_THREAD_SUPPORT -DQT_NO_DEBUG -Wpointer-arith -Wwrite-strings -O2 -fno-exceptions -fno-check-new -fno-common";

my $nspaces = 50;

my %qtdefs=();
my %qtundefs=();

my $tmp = gettmpfile();
my $QTDIR = $ENV{QTDIR};
my $qtinc = $QTDIR . '/include';
my $allinc = "-I$qtinc";
my $alllib = "-L$QTDIR/lib";
my $qtflags ='-lqt-mt -ldl   $(LIBRESOLV) $(LIBPNG) $(X_PRE_LIBS) -lXext $(LIB_X11) $(LIBSM) -lpthread';
my %x;
$x{'LIBPNG'}  =   '-lpng -lz -lm';
$x{'LIBJPEG'} =   '-ljpeg';
$x{'LIBSM'}   =   '-lSM -lICE';
$x{'LIBSOCKET'} = '';
$x{'LIBDL'}      = '-ldl';
$x{'LIBRESOLV'}  = '-lresolv';
$x{'LIB_X11'} =   '-lX11 $(LIBSOCKET)';
$x{'X_PRE_LIBS'} = ' -lSM -lICE';
$x{'LIB_X11'} =~ s/\$\((.*?)\)/$x{$1}/g;

$qtflags =~ s/\$\((.*?)\)/$x{$1}/g;

 -e "$qtinc/qglobal.h" or die "Invalid Qt directory.\n";

my $ccmd = "$cc $ccflags $allinc $alllib -o $tmp $tmp.cpp $qtflags";

my $threshold = defined($opt_t)?$opt_t : $default_threshold;
$threshold >= 0 or die "invalid testing threshold: $threshold\n";

print "Checking how Qt was built... \n";
print "Threshold is set to $threshold\n" unless $opt_q;

my($count, $used, $total);
map{ $tests{$_}->[2]>=$threshold ? ($used++, $total++):$total++ } keys %tests;

print "Number of defines to be tested : $used/$total\n\n" unless $opt_q;
open( QTDEFS, ">".($opt_o || "qtdefines") ) or die "Can't open output file: $!\n";

grab_qglobal_symbols();
preliminary_test();
perform_all_tests();

print +scalar(keys %qtdefs) . " defines found.\n";

print QTDEFS join("\n", keys %qtdefs), "\n";
close;

#--------------------------------------------------------------#

sub gettmpfile
{
	my $tmpdir = $ENV{'TMP'} || ".";
	my $tmpname = $$."-qtguess";
	while( -e "$tmpdir/$tmpname" || -e "$tmpdir/${tmpname}.cpp" )
	{
		$tmpname .= int (rand * 9);
	}
	return "$tmpdir/$tmpname";
}

#--------------------------------------------------------------#

sub grab_qglobal_symbols
{
	my $cmd = "$cc -E -D__cplusplus -dM -I$qtinc $qtinc/qglobal.h 2>/dev/null";
	my $symbols = `$cmd`;
        for(0..1)
        {
	    if( check_exit_status($?) )
	    {
		while( $symbols =~/^#\s*define\s*(QT_\S+)/gm )
		{
			$qtdefs{$1} = 1;
		}
		print "Found ". scalar( keys %qtdefs )." predefined symbol".((scalar( keys %qtdefs ) -1)?"s":"")." in qglobal.h\n" unless ($opt_q or !(keys %qtdefs));
		while( $symbols =~/^#\s*define\s*QT_MODULE_(\S+)/gm )
		{
			$qtundefs{"QT_NO_$1"} = 1;
		}
		print "Found ". scalar( keys %qtundefs )." undefined symbol".((scalar( keys %qtundefs ) -1)?"s":"")." in qglobal.h\n" unless ($opt_q or !(keys %qtundefs));
                last;
	    }
	    elsif(! $_) # first try
	    {
		print  "Failed to run $cmd.\nTrying without __cplusplus (might be already defined)\n";
                $cmd = "$cc -E -dM -I$qtinc $qtinc/qglobal.h 2>/dev/null";
                $symbols = `$cmd`;
                next;
	    }
        }
}

#--------------------------------------------------------------#

sub preliminary_test
{
	my $msg = "Trying to compile and link a small program...";
	print $msg, " " x ($nspaces - length($msg) + 8);
	open( OUT, ">${tmp}.cpp" ) or die "Failed to open temp file ${tmp}.cpp: $!\n";
	my $simple=q�
		#include <qapplication.h>
		int main( int argc, char **argv )
		{
			QApplication foo( argc, argv );
			return 0;
		}
	�;
	print OUT $simple;
	close OUT;
        my $out = `$ccmd 2>&1`;
	if( !check_exit_status($?) )
	{
		die <<"EOF";

FAILED : check your configuration.
Failed program was:
$simple
Compiled with:
$ccmd
Compiler output:
$out
EOF
	}
	else
	{
		print "OK\n";
	}
}

#--------------------------------------------------------------#

sub perform_all_tests
{
	foreach ( sort { $tests{$a}->[2] <=> $tests{$b}->[2]} keys %tests)
	{
		$tests{$_}->[2] < $threshold and next;
		($qtdefs{$_} || $qtundefs{$_}) and do
		{
			print "\rSkipping $_ (in qglobal.h)".( " " x (($nspaces-16) - length($_)) ).($qtundefs{$_}?"*Undefined*":" [Defined]").($opt_q?"":"\n");
			next
		};
		print "\rTesting $_".( " " x ($nspaces - length($_)) );
		open( OUT, ">${tmp}.cpp" ) or die "Failed to open temp file ${tmp}.cpp: $!\n";
		foreach $def(keys %qtdefs)
		{
			print OUT "#define $def\n";
		}
		foreach $inc(split /,\s*/, $tests{$_}->[0])
		{
			print OUT "#include <$inc>\n";
		}
		print OUT "#include <qfeatures.h>\n";
		print OUT $tests{$_}->[3] if $tests{$_}->[3]; # need to define some classes ?
		print OUT qq�

		int main( int argc, char **argv )
		{
		$tests{$_}->[1]
		return 0;
		}
		�;
		close OUT;

                my $out = `$ccmd 2>&1`;

		my $ok = check_exit_status($?);
		if( !$ok )
		{
			$qtdefs{$_} = 1;
		}
		print +$opt_q ? ++$count."/$used" : ( $ok ? "*Undefined*\n" : " [Defined]\n" );
	}
	$opt_q && print "\n";
}

#--------------------------------------------------------------#

sub check_exit_status
{
	my $a = 0xFFFF & shift;
	if( !$a )
	{
		return 1;
	}
	elsif( $a == 0xFF00 )
	{
		die "\nSystem call failed: $!\n";
	}
	elsif( $a > 0x80 )
	{
		# non-zero status.
	}
	else
	{
		if( $a & 0x80 )
		{
			die "\n$cc coredumped with signal ". ($a & ~0x80);
		}
		die "\n$cc interrupted by signal $a\n";
	}
	return 0;
}

#--------------------------------------------------------------#

END
{
	unlink $tmp if -e $tmp;
	unlink "${tmp}.cpp" if -e "${tmp}.cpp";
}

#--------------------------------------------------------------#

BEGIN {

# "DEFINE" => ["header-1.h,... header-n.h", "main() code", priority, "Definitions (if needed)"]

our %tests = (
	"QT_NO_ACCEL" => 		["qaccel.h", "QAccel foo( (QWidget*)NULL );", 5],
	"QT_NO_ACTION" =>		["qaction.h", "QAction foo( (QObject*)NULL );", 5],
	"QT_NO_ASYNC_IO" =>		["qasyncio.h", "QAsyncIO foo();", 5],
	"QT_NO_ASYNC_IMAGE_IO"=>	["qasyncimageio.h", "QImageDecoder foo( (QImageConsumer*) NULL );", 5],
	"QT_NO_BIG_CODECS" => 		["qbig5codec.h", "QBig5Codec foo();", 5],
	"QT_NO_BUTTON" =>		["qbutton.h", "QButton foo( (QWidget*)NULL );", 10],
 	"QT_NO_BUTTONGROUP" =>		["qbuttongroup.h", "QButtonGroup foo( (QWidget*)NULL );", 12],
 	"QT_NO_CANVAS" =>		["qcanvas.h", "QCanvas foo( (QObject*)NULL );", 10],
 	"QT_NO_CHECKBOX" =>		["qcheckbox.h", "QCheckBox( (QWidget*)NULL );", 10],
	"QT_NO_CLIPBOARD" => 		["qapplication.h, qclipboard.h", q�
						QApplication foo( argc, argv );
						QClipboard *baz= foo.clipboard();
					�, 5],
 	"QT_NO_COLORDIALOG" =>		["qcolordialog.h", "QColorDialog::customCount();", 12],
 	"QT_NO_COMBOBOX" =>		["qcombobox.h", "QComboBox( (QWidget*)NULL );", 10],
	"QT_NO_COMPAT" =>		["qfontmetrics.h", q�
						QFontMetrics *foo= new QFontMetrics( QFont() );
						int bar = foo->width( 'c' );
					�, 0],
	"QT_NO_COMPONENT" =>		["qapplication.h", q�
 						QApplication foo( argc, argv );
 						foo.addLibraryPath( QString::null );
					�, 5],
 	"QT_NO_CURSOR" =>		["qcursor.h", "QCursor foo;", 5],
 	"QT_NO_DATASTREAM" =>		["qdatastream.h", "QDataStream foo;", 5],
 	"QT_NO_DATETIMEEDIT" =>		["qdatetimeedit.h", "QTimeEdit foo;", 12],
	"QT_NO_DIAL" =>			["qdial.h", "QDial foo;", 10],
	"QT_NO_DIALOG" =>		["qdialog.h", "QDialog foo;", 12],
	"QT_NO_DIR" =>			["qdir.h", "QDir foo;", 5],
	"QT_NO_DNS" =>			["qdns.h", "QDns foo;", 5],
	"QT_NO_DOM" =>			["qdom.h", "QDomDocumentType foo;", 5],
	"QT_NO_DRAGANDDROP" =>		["qevent.h", "QDropEvent foo( QPoint(1,1) );", 5],
	"QT_NO_DRAWUTIL" =>		["qdrawutil.h, qcolor.h", "qDrawPlainRect( (QPainter *) NULL, 0, 0, 0, 0, QColor() );", 10],
	"QT_NO_ERRORMESSAGE" => 	["qerrormessage.h", "QErrorMessage foo( (QWidget*) NULL );", 13],
	"QT_NO_FILEDIALOG" =>		["qfiledialog.h", "QFileIconProvider foo;", 13],

	"QT_NO_FONTDATABASE" =>		["qfontdatabase.h", "QFontDatabase foo;", 5],
	"QT_NO_FONTDIALOG" => 		["qfontdialog.h",   "QFontDialog::getFont( (bool *)NULL );", 12],
	"QT_NO_FRAME" => 		["qframe.h", "QFrame foo;", 10],
	"QT_NO_GRID" =>			["qgrid.h", "QGrid foo(5);", 12],
	"QT_NO_GRIDVIEW" =>		["qgridview.h", "QFoo foo;", 13, q�
						class QFoo: public QGridView
						{
						public:
							QFoo(){};
							~QFoo(){};
							void paintCell(QPainter *, int, int){};
						};
					�],
	"QT_NO_GROUPBOX" =>		["qgroupbox.h", "QGroupBox foo;", 12],
	"QT_NO_HBOX" =>			["qhbox.h", "QHBox foo;", 12],
	"QT_NO_HBUTTONGROUP" =>		["qhbuttongroup.h", "QHButtonGroup foo;", 13],
	"QT_NO_HEADER" =>		["qheader.h", "QHeader foo;", 10],
	"QT_NO_HGROUPBOX" =>		["qhgroupbox.h", "QHGroupBox foo;", 13],
	"QT_NO_ICONSET" =>		["qiconset.h", "QIconSet foo;", 8],

	"QT_NO_ICONVIEW" =>		["qiconview.h", "QIconView foo;", 13],
	"QT_NO_IMAGEFORMATPLUGIN" =>	["qimageformatplugin.h, qstringlist.h", "QFoo foo;", 5, q�

						class QFoo: public QImageFormatPlugin
						{
						public:
						    QFoo() {};
						    ~QFoo() {};
						    QStringList keys() const { return QStringList(); };
						    bool installIOHandler( const QString &format ) { return true; };
						};
						Q_EXPORT_PLUGIN( QFoo )
					�],
 	"QT_NO_IMAGE_DITHER_TO_1" =>	["qimage.h", q�
						QImage *foo = new QImage;
						foo->createAlphaMask();
					�, 8],
 	"QT_NO_IMAGE_HEURISTIC_MASK" =>	["qimage.h", q�
						QImage *foo = new QImage;
						foo->createHeuristicMask();
					�, 8],
	"QT_NO_IMAGE_MIRROR" =>	["qimage.h", q�
						QImage *foo = new QImage;
						foo->mirror();
					�, 8],
 	"QT_NO_IMAGE_SMOOTHSCALE" =>	["qimage.h", q�
						QImage *foo = new QImage;
						foo->smoothScale( 10, 10);
					�, 8],
 	"QT_NO_IMAGE_TEXT" =>		["qimage.h", "QImageTextKeyLang foo;", 8],
 	"QT_NO_IMAGE_TRANSFORMATION" =>	["qimage.h", q�
						QImage *foo = new QImage;
						foo->scale( 10, 10);
					�, 8],
 	"QT_NO_IMAGE_TRUECOLOR" =>	["qimage.h", q�
						QImage *foo = new QImage;
						foo->convertDepthWithPalette( 1, (QRgb*) NULL, 1 );
					�, 8],
	"QT_NO_INPUTDIALOG" =>		["qinputdialog.h, qstring.h", q�QInputDialog::getText( QString::null, QString::null);�, 13],
	"QT_NO_IMAGEIO" => 		["qbitmap.h, qstring.h", q�
						QBitmap foo( QString::fromLatin1("foobar") );
					�, 5],
	"QT_NO_IMAGEIO_JPEG" =>		["qjpegio.h", "qInitJpegIO();", 8],
	"QT_NO_IMAGEIO_MNG" =>		["qmngio.h", "qInitMngIO();", 8],
	"QT_NO_IMAGEIO_PNG" =>		["qpngio.h", "qInitPngIO();", 8],
	"QT_NO_LABEL" =>		["qlabel.h", "QLabel foo( (QWidget*) NULL );", 10],
	"QT_NO_LAYOUT" =>		["qlayout.h", "QFoo foo;", 10, q�

 						class QFoo: public QLayout
 						{
 						public:
 						    QFoo() {};
 						    ~QFoo() {};
 						    void addItem( QLayoutItem * ) { };
						    QSize sizeHint() const { return QSize(); }
 						    QLayoutIterator iterator() { return QLayoutIterator( (QGLayoutIterator *) NULL ); };
						    void setGeometry( const QRect & ) { };
 						};
 					�],
	"QT_NO_LCDNUMBER" =>		["qlcdnumber.h", "QLCDNumber foo;", 12],
	"QT_NO_LINEEDIT" =>		["qlineedit.h", "QLineEdit foo( (QWidget *) NULL );", 12],
	"QT_NO_LISTBOX" =>		["qlistbox.h", "QListBox foo;", 13],
	"QT_NO_LISTVIEW" =>		["qlistview.h", "QListView foo;", 13],
	"QT_NO_MAINWINDOW" =>		["qmainwindow.h", "QMainWindow foo;", 13],
	"QT_NO_MENUBAR" =>		["qmenubar.h", "QMenuBar foo;", 13],
	"QT_NO_MOVIE" =>		["qmovie.h", "QMovie foo;", 5],
	"QT_NO_MENUDATA" =>		["qmenudata.h", "QMenuData foo;", 9],
	"QT_NO_MESSAGEBOX" =>		["qmessagebox.h", "QMessageBox foo;", 13],
	"QT_NO_MIME" =>			["qmime.h", "QMimeSourceFactory foo;", 5],
 	"QT_NO_MIMECLIPBOARD" => 	["qapplication.h, qclipboard.h", q�
 						QApplication foo( argc, argv );
 						QClipboard *baz= foo.clipboard();
						baz->data();
					�, 8],

	"QT_NO_MULTILINEEDIT" =>	["qmultilineedit.h", "QMultiLineEdit foo;", 14],
	"QT_NO_NETWORK" =>		["qnetwork.h", "qInitNetworkProtocols();", 5],
	"QT_NO_NETWORKPROTOCOL" =>	["qnetworkprotocol.h", "QNetworkProtocol foo;", 8],
 	"QT_NO_NETWORKPROTOCOL_FTP" =>	["qftp.h", "QFtp foo;", 9],
	"QT_NO_PALETTE" =>		["qpalette.h", "QColorGroup foo;", 5],
	"QT_NO_PICTURE" =>		["qpicture.h", "QPicture foo;", 5],
	"QT_NO_PIXMAP_TRANSFORMATION" =>["qbitmap.h, qwmatrix.h", q�
						QBitmap *foo= new QBitmap();
						QWMatrix bar;
						foo->xForm( bar );
					�, 5],
	"QT_NO_POPUPMENU" =>		["qpopupmenu.h", "QPopupMenu foo;", 12],
	"QT_NO_PRINTER" =>		["qprinter.h", "QPrinter foo;", 5],
	"QT_NO_PRINTDIALOG" =>		["qprintdialog.h", "QPrintDialog foo( (QPrinter*) NULL );", 13],
	"QT_NO_PROCESS" =>		["qprocess.h", "QProcess foo;", 5],
	"QT_NO_PROGRESSBAR" =>		["qprogressbar.h", "QProgressBar foo;", 12],
	"QT_NO_PROGRESSDIALOG" =>	["qprogressdialog.h", "QProgressDialog foo;", 13],
	"QT_NO_PUSHBUTTON" =>		["qpushbutton.h", "QPushButton foo( (QWidget *) NULL );", 12],
	"QT_NO_PROPERTIES" =>		["qmetaobject.h", "QMetaProperty foo;", 0],
#	"QT_NO_QTMULTILINEEDIT" =>	["qtmultilineedit.h", "QtMultiLineEdit foo;", 15],
#	"QT_NO_QTTABLEVIEW" =>		["qttableview.h", "QFoo foo;", 16, q�
#						class QFoo: public QtTableView
#						{
#						public:
#							QFoo() {};
#							~QFoo() {};
#							void paintCell( QPainter *, int, int) {};
#						};
#						�],
	"QT_NO_QUUID_STRING" =>		["quuid.h", "QUuid foo( QString::null );", 8],
	"QT_NO_RANGECONTROL" =>		["qrangecontrol.h", "QRangeControl foo;", 10],
	"QT_NO_REGEXP" =>		["qregexp.h", "QRegExp foo;", 5],
	"QT_NO_REGEXP_WILDCARD" =>	["qregexp.h", q�
						QRegExp foo;
						foo.wildcard();
					�, 8],
	"QT_NO_REMOTE" =>		["qapplication.h", q�
   						QApplication foo( argc, argv );
   						foo.remoteControlEnabled();
					�, 15],
	"QT_NO_RADIOBUTTON" =>		["qradiobutton.h", "QRadioButton foo( (QWidget *) NULL );", 12],
	"QT_NO_RICHTEXT" =>		["qsimplerichtext.h, qstring.h, qfont.h", "QSimpleRichText foo( QString::null, QFont() );", 10],
	"QT_NO_SCROLLBAR" =>		["qscrollbar.h", "QScrollBar foo( (QWidget *) NULL );", 12],
	"QT_NO_SCROLLVIEW" =>		["qscrollview.h", "QScrollView foo;", 12],
	"QT_NO_SEMIMODAL" =>		["qsemimodal.h", "QSemiModal foo;", 10],
	"QT_NO_SESSIONMANAGER" =>	["qapplication.h", q�
  						QApplication foo( argc, argv );
  						foo.sessionId();
					�, 15],
	"QT_NO_SETTINGS" =>		["qsettings.h", "QSettings foo;", 5],
	"QT_NO_SIGNALMAPPER" =>		["qsignalmapper.h", "QSignalMapper foo( (QObject *) NULL );", 0],
	"QT_NO_SIZEGRIP" =>		["qsizegrip.h", "QSizeGrip foo( (QWidget *) NULL );", 10],
	"QT_NO_SLIDER" =>		["qslider.h", "QSlider foo( (QWidget *) NULL );", 12],
	"QT_NO_SOUND" =>		["qsound.h", "QSound foo( QString::null );", 5],


	"QT_NO_SPINWIDGET" =>		["qrangecontrol.h", "QSpinWidget foo;", 10],
	"QT_NO_SPRINTF" =>		["qcolor.h", q�
						QColor foo;
						foo.name();
					�, 0],



	"QT_NO_SQL" =>			["qsqlcursor.h", "QSqlCursor foo;", 5],
	"QT_NO_STRINGLIST" =>		["qstringlist.h", "QStringList foo;", 0],
	"QT_NO_STYLE" =>		["qapplication.h", q�
   						QApplication foo( argc, argv );
   						foo.style();
 					�, 15],

#	"QT_NO_STYLE_CDE" =>		["qcdestyle.h", "QCDEStyle foo;", 16],
# 	"QT_NO_STYLE_COMPACT" =>	["qcompactstyle.h", "QCompactStyle foo;", 16],
#	"QT_NO_STYLE_INTERLACE" =>	["qinterlacestyle.h", "QInterlaceStyle foo;", 16],
#	"QT_NO_STYLE_PLATINUM" =>	["qplatinumstyle.h", "QPlatinumStyle foo;", 16],
#	"QT_NO_STYLE_MOTIF" =>		["qmotifstyle.h", "QMotifStyle foo;", 16],
#	"QT_NO_STYLE_MOTIFPLUS" =>	["qmotifplusstyle.h", "QMotifPlusStyle foo;", 16],
#	"QT_NO_STYLE_SGI" =>		["qsgistyle.h", "QSGIStyle foo;", 16],
#	"QT_NO_STYLE_WINDOWS" =>	["qwindowsstyle.h", "QWindowsStyle foo;", 16],
        "QT_NO_TABBAR" =>               ["qtabbar.h", "QTabBar foo;", 10],
        "QT_NO_TABDIALOG" =>            ["qtabdialog.h", "QTabDialog foo;", 12],
        "QT_NO_TABLE" =>                ["qtable.h", "QTable foo;", 10],
        "QT_NO_TABWIDGET" =>            ["qtabwidget.h", "QTabWidget foo;", 10],
        "QT_NO_TEXTBROWSER" =>          ["qtextbrowser.h", "QTextBrowser foo;", 14],
        "QT_NO_TEXTCODEC" =>            ["qtextcodec.h", "QTextCodec::codecForIndex(1);", 5],
        "QT_NO_TEXTCODECPLUGIN" =>      ["qtextcodecplugin.h, qstringlist.h, qvaluelist.h, qtextcodec.h", "QFoo foo;", 6, q� 
	
						class QFoo: public QTextCodecPlugin
						{
						public:
						    QFoo() {};
						    ~QFoo() {};
                                                    QStringList names() const {return QStringList();}
                                                    QValueList<int>mibEnums() const {return QValueList<int>();}
                                                    QTextCodec *createForName( const QString & name ) {return (QTextCodec *)NULL;}
                                                    QTextCodec *createForMib( int mib ) {return (QTextCodec *)NULL;}
						};
						Q_EXPORT_PLUGIN( QFoo )
					�],
 	"QT_NO_TEXTEDIT" =>		["qtextedit.h", "QTextEdit foo;", 13], 
        "QT_NO_TEXTSTREAM" =>           ["qtextstream.h", "QTextStream foo;", 5],
        "QT_NO_TEXTVIEW" =>             ["qtextview.h", "QTextView foo;", 14], #Obsolete
        "QT_NO_TOOLBAR" =>              ["qtoolbar.h", "QToolBar foo;", 10],
        "QT_NO_TOOLBUTTON" =>           ["qtoolbutton.h", "QToolButton foo((QWidget *) NULL );", 12],
        "QT_NO_TOOLTIP" =>              ["qtooltip.h", "QToolTip::hide();", 10],
        
	"QT_NO_TRANSFORMATIONS" =>	["qpainter.h", q�
						QPainter *foo= new QPainter();
						foo->setViewXForm( true );�, 5],
        "QT_NO_VARIANT" =>              ["qvariant.h", "QVariant foo;", 0],
        "QT_NO_WHATSTHIS" =>            ["qwhatsthis.h", "QWhatsThis::inWhatsThisMode();", 10],
	"QT_NO_WHEELEVENT" =>		["qevent.h", "QWheelEvent foo( QPoint(1,1), 1, 1 );", 5],
        "QT_NO_WIDGET_TOPEXTRA" =>      ["qwidget.h", "QWidget foo; foo.caption();", 9],
        "QT_NO_WIDGETSTACK" =>          ["qwidgetstack.h", "QWidgetStack foo;", 13],
        "QT_NO_WIZARD" =>               ["qwizard.h", "QWizard foo;", 13],
	"QT_NO_WMATRIX" =>		["qwmatrix.h", "QWMatrix foo;", 0],
	"QT_NO_XML" =>			["qxml.h", "QXmlNamespaceSupport foo;", 5],
	);

}
