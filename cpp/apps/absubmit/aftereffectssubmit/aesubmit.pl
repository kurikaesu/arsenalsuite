#!/usr/bin/perl -w

use strict;
$|++;

my $version = $ARGV[0];
my $proj = $ARGV[1];
unless( $proj ) { die "must provide an AE project file\n" }
unless( $proj =~ /.aep$/ ) { die "not an AE project file\n" }

my $frameStart;
my $frameEnd;
my $outputPath;
my $width;
my $height;
my $comp;

my $error;
my $jobInfoCmd;
if( $version eq '7' ) {
	$jobInfoCmd = q|sudo /Applications/Adobe\ After\ Effects\ 7.0/aerender -project |.$proj;
} elsif( $version eq '8' ) {
	$jobInfoCmd = q|sudo /Applications/Adobe\ After\ Effects\ CS3/aerender -project |.$proj;
} else {
	$jobInfoCmd = q|sudo /Applications/Adobe\ After\ Effects\ 6.5/aerender -project |.$proj;
}
warn "running $jobInfoCmd\n";
open(LOG, ">>/var/log/ab/aesubmit.log");
print LOG "running $jobInfoCmd\n";
open(AE, "$jobInfoCmd 2>/dev/null|");
while( my $line = <AE> ) {
	print "read: $line";
	print LOG "read: $line";
 #x5:Shows:smallville:season6:sv622_phantom:shot:eyes_01:comps:eyes01_comp_v03:eyes01_comp_v03.[####].sgi
	if( $line =~ /PROGRESS:\s+Output To: (.*)/ ) {
		$outputPath = $1;
		$outputPath =~ s/:/\//g;
		$outputPath =~ s/\[#+\]//;
		$outputPath = "/mnt/".$outputPath;
	}
	elsif( $line =~ /PROGRESS:\s+Start:\s+(\d+)/ ) {
		warn "got start message\n";
		$frameStart = $1;
		$frameStart =~ s/^0+//g;
		$frameStart ||= 0;
	}
	elsif( $line =~ /PROGRESS:\s+End:\s+(\d+)/ ) {
		warn "got end message\n";
		$frameEnd = $1;
		$frameEnd =~ s/^0+//g;
		$frameEnd ||= 0;
	}
	elsif( $line =~ /PROGRESS:\s+Final Size: (\d+) x (\d+)/ ) {
		warn "got size message\n";
		$width = $1;
		$height = $2;
	}
	elsif( $line =~ /Starting composition .([\w_]+).\./ ) {
	 #Finished composition blood_03_comp_02.
		warn "got comp message\n";
		$comp = $1;
	}
	elsif( $line =~ /PROGRESS:\s+Post-Render Action/ ) {
		# got all we need, bail!
		warn "got bail message\n";
		last;
	}
	elsif( $line =~ /(Error:.*)/ ) {
		$error .= $line;
	}
	elsif( $line =~ /INFO: After Effects error:(.*)/ ) {
		$error .= $line;
	}
}
warn "kill aerender now\n";
system("sudo killall -u root aerender");
system(q|sudo killall -u root "After Effects"|);

close( AE );

my ($job) = $proj =~ /.*\/([\w_]+).aep$/;
my $user = $ENV{USER};
if( $user eq 'root' ) { $user = 'barry' }
if( $user eq 'install' ) { $user = 'barry' }

my( $projectName ) = $proj =~ m#/Shows/([^/]+)/#;

my $arch = `arch`;
chomp $arch;
chdir("/mnt/x5/Global/infrastructure/ab/$arch/absubmit/");
my $cmd = "./absubmit";
if( defined $frameStart && defined $frameEnd && defined $outputPath && defined $job ) {
	if( $version eq '7' ) {
		$cmd .= " jobType AfterEffects7";
	} elsif( $version eq '8' ) {
		$cmd .= " jobType AfterEffects8";
	} else {
		$cmd .= " jobType AfterEffects";
	}

	$cmd .= " packetType continuous";
	$cmd .= " packetSize 0";
	$cmd .= " noCopy true";
	$cmd .= " user ".$user;
	$cmd .= " projectName \"".$projectName."\"";
	$cmd .= " fileName ".$proj;
	$cmd .= " outputPath ".$outputPath;
	$cmd .= " job ".$job;
	$cmd .= " width ".$width;
	$cmd .= " height ".$height;
	$cmd .= " frameStart ".$frameStart;
	$cmd .= " frameEnd ".$frameEnd;
	$cmd .= " comp \"$comp\"";
	$cmd .= " notifyOnComplete ${user}:j";
} else {
	$error .= "could not gather job info";
}

if( -e "/mnt/x5/_mnt_x5" ) {
	my $syncId;
	# we're in VCO so sync file to LA
	open(SYNC, "perl /mnt/x5/tools/perl/scripts/sync_AB arnold pollux $proj 2>/dev/null |");
	while( my $line = <SYNC> ) {
		#print LOG "sync_AB: $line";
		if( !$syncId && $line =~ m/^keyJob\|(\d+)/ ) {
			$syncId = $1;
			print LOG "sync_AB: found keyJob $syncId\n";
		}
	}
	if( $syncId ) {
		$cmd .= " deps ".$syncId;
	} else {
		$error .= "Could not submit the Sync job";
	}
}

if( $error ) {
	print LOG "ERROR: $error\n";
	die "$error\n";
}

print LOG "$cmd\n";
open(AESUBMIT, "$cmd 2>/dev/null |");

my $aeId;
while( my $line = <AESUBMIT> ) {
	#print LOG "absubmit: $line";
	if( !$aeId && $line =~ m/^keyJob\|(\d+)/ ) {
		$aeId = $1;
		print LOG "absubmit: found keyJob $aeId\n";
	}
}
if( $aeId ) {
	# all AE renders get a QT (if they're not a QT render)
	#
	if( $outputPath !~ /.mov$/ ) {
		my( $compDir, $baseName, $ext ) = $outputPath =~ m#(.*?)/([^/]+)\.\.(\w+)$#;
		my $cmdQT = "/mnt/x5/tools/perl/scripts/autoqt_AB $aeId $compDir $baseName $ext $frameStart $frameEnd";
		if( -e "/mnt/x5/_mnt_x5" ) { $cmdQT .= " 1"; }
		print LOG "$cmdQT\n";
		system($cmdQT);
	}

	# all AE renders go to PFPlay stations for review
	my $cmd = "/mnt/x5/tools/perl/scripts/pfsync_submit $outputPath $aeId $frameStart $frameEnd";
	print LOG "$cmd\n";
	system($cmd);
}
close LOG;

