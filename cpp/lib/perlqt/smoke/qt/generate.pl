#!/usr/bin/perl -w

## Run this first, to generate the x_*.cpp files from the Qt headers
## using kalyptus

my $kalyptusdir = "../../kalyptus";

use File::Basename;
use File::Copy qw|cp|;
use File::Compare;
use Cwd;

my $here = getcwd;
my $outdir = $here . "/generate.pl.tmpdir";
my $finaloutdir = $here;
my $defines = "qtdefines";
my $headerlist = "header_list";
my $definespath = "$here/$defines";
my $headerlistpath = "$here/$headerlist";

## Note: outdir and finaloutdir should NOT be the same dir!

# Delete all x_*.cpp files under outdir (or create outdir if nonexistent)
if (-d $outdir) { system "rm -f $outdir/x_*.cpp"; } else { mkdir $outdir; }

mkdir $finaloutdir unless (-d $finaloutdir);

#  Load the QT_NO_* macros found in "qtdefines". They'll be passed to kalyptus
my $macros="";
if ( -e $definespath ){
    print "Found '$defines'. Reading preprocessor symbols from there...\n";
    $macros = " --defines=$definespath ";
}

# Need to cd to kalyptus's directory so that perl finds Ast.pm etc.
chdir "$kalyptusdir" or die "Couldn't go to $kalyptusdir (edit script to change dir)\n";

# Find out which header files we need to parse
# We don't want all of them - e.g. not template-based stuff
my %excludes = (
    'qaccessible.h' => 1,  # Accessibility support is not compiled by defaut
    'qassistantclient.h' => 1, # Not part of Qt (introduced in Qt-3.1)
    'qmotif.h' => 1,       # 
    'qmotifwidget.h' => 1, # Motif extension (introduced in Qt-3.1)
    'qmotifdialog.h' => 1, #
    'qxt.h' => 1, # Xt
    'qxtwidget.h' => 1, # Xt
    'qdns.h' => 1, # internal
    'qgl.h' => 1, # OpenGL
    'qglcolormap.h' => 1, # OpenGL
    'qnp.h' => 1, # NSPlugin
    'qttableview.h' => 1,  # Not in Qt anymore...
    'qtmultilineedit.h' => 1,  # Not in Qt anymore...
    'qwidgetfactory.h' => 1,  # Just an interface
    'qsharedmemory.h' => 1, # "not part of the Qt API" they say
    'qwindowsstyle.h' => 1, # Qt windowsstyle, plugin
    'qmotifstyle.h' => 1,
    'qcompactstyle.h' => 1,
    'qinterlacestyle.h' => 1,
    'qmotifplusstyle.h' => 1,
    'qsgistyle.h' => 1,
    'qplatinumstyle.h' => 1,
    'qcdestyle.h' => 1,
    'qwindowsxpstyle.h' => 1 # play on the safe side 
);

# Some systems have a QTDIR = KDEDIR = PREFIX
# We need a complete list

my %includes;
open(HEADERS, $headerlistpath) or die "Couldn't open $headerlistpath: $!\n";
map { chomp ; $includes{$_} = 1 } <HEADERS>;
close HEADERS;

# Can we compile the OpenGl module ?
if("no" eq "yes")
{
    open(DEFS, $definespath);
    my @defs = <DEFS>;
    close DEFS;
    if(!grep(/QT_NO_OPENGL/, @defs))
    {
      $excludes{'qgl.h'} = undef;
      $excludes{'qglcolormap.h'} = undef;
    }
    else
    {
      print STDERR "Qt was not compiled with OpenGL support...\n Skipping QGL Classes.\n";
    }
}

# List Qt headers, and exclude the ones listed above
my @headers = ();

my @inc_dirs;

if( -d '/usr/include/stone' ) {
	push @inc_dirs, "/usr/include/stone/";
} else {
	push @inc_dirs, "../../stone/include/";
}

if( -d '/usr/include/classes/' ) {
	push @inc_dirs, "/usr/include/classes/";
	push @inc_dirs, "/usr/include/classes/base/";
} else {
	push @inc_dirs, "../../classes/autocore/";
	push @inc_dirs, "../../classes/base/";
	push @inc_dirs, "../../classes/";
}

#warn $ENV{QTDIR};
if( exists $ENV{PERLQT_QT_HEADERS} ) {
	push @inc_dirs, split( /:/, $ENV{PERLQT_QT_HEADERS} );
} else {
	if( exists $ENV{QTDIR} && -d $ENV{QTDIR} ) {
		my $qtd = $ENV{QTDIR};
		print "QTDIR found at " . $ENV{QTDIR} . "/include\n";
		if ( $qtd =~ '^/usr/lib/qt' ) {
			push @inc_dirs, "$qtd/include/Qt/";
		} else {
			push @inc_dirs, "$qtd/src/gui/kernel";
			push @inc_dirs, "$qtd/src/corelib/kernel";
			push @inc_dirs, "$qtd/src/corelib/io";
			push @inc_dirs, "$qtd/src/corelib/tools";
			push @inc_dirs, "$qtd/src/gui/image";
			push @inc_dirs, "$qtd/src/gui/text";
			push @inc_dirs, "$qtd/src/sql/kernel";
		}
	} elsif ( -d '/usr/lib/qt-4.1/' ) {
		push @inc_dirs, '/usr/lib/qt-4.1/'
	} else {
		die "QTDIR environment variable not set.\n";
	}
}

foreach $incdir (@inc_dirs) {
	opendir (TEMP, $incdir) or warn "Couldn't find $incdir" and next;
	foreach $filename (readdir(TEMP)) {
		$entry = $incdir."/".$filename;
		if ( ( -e $entry or -l $entry )         # A real file or a symlink
			&& ( ! -d _ ) && ( $filename =~ /\.h$/ ) )                     # Not a symlink to a dir though
		{
			push(@headers, $entry)
			if ( !defined $excludes{$filename} # Not excluded
				&& $includes{$filename}        # Known header
			&& $filename =~ /\.h$/ ); # Not a backup file etc. Only headers.
			undef $includes{$filename}
		}
	}
	closedir TEMP;

}

# Launch kalyptus
my @inc_dir_cmd = map { "--includedir=$_" } @inc_dirs;

system "perl kalyptus @ARGV --globspace -fsmoke --name=qt $macros --no-cache --outputdir=$outdir @inc_dir_cmd @headers";
my $exit = $? >> 8;
exit $exit if ($exit);

# Generate diff for smokedata.cpp
#unless ( -e "$finaloutdir/smokedata.cpp" ) {
#    open( TOUCH, ">$finaloutdir/smokedata.cpp");
#    close TOUCH;
#}
#system "diff -u $finaloutdir/smokedata.cpp $outdir/smokedata.cpp > $outdir/smokedata.cpp.diff";

# Copy changed or new files to finaloutdir
opendir (OUT, $outdir) or die "Couldn't opendir $outdir";
foreach $filename (readdir(OUT)) {
    next if ( -d "$outdir/$filename" ); # only files, not dirs
    my $docopy = 1;
    if ( -f "$finaloutdir/$filename" ) {
        $docopy = compare("$outdir/$filename", "$finaloutdir/$filename"); # 1 if files are differents
    }
    if ($docopy) {
	#print STDERR "Updating $filename...\n";
	cp("$outdir/$filename", "$finaloutdir/$filename");
    }
}
closedir OUT;

# Check for deleted files and warn
my $deleted = 0;
opendir(FINALOUT, $finaloutdir) or die "Couldn't opendir $finaloutdir";
foreach $filename (readdir(FINALOUT)) {
    next if ( -d "$finaloutdir/$filename" ); # only files, not dirs
    if ( $filename =~ /.cpp$/ && ! ($filename =~ /_la_closure.cpp/) && ! -f "$outdir/$filename" ) {
      print STDERR "Removing obsolete file $filename\n";
      unlink "$finaloutdir/$filename";
      $deleted = 1;
    }
}
closedir FINALOUT;

# Delete outdir
system "rm -rf $outdir";

