use strict;

use Digest::MD4 qw(md4_hex);
use DBD::Pg;
use Encode;

my $dbh = DBI->connect("dbi:Pg:dbname=bach;host=sql01.drd.int;port=5432;", "barry", "sonja", {AutoCommit => 1});
my $insertSql =qq|INSERT INTO bachasset (exif,path,imported,cacheprefix,directory,filesize,filetype) VALUES (?,?,?,?,now(),?,?,?,2);\n|;
my $sth = $dbh->prepare($insertSql);

my $updateSql =qq|UPDATE bachasset SET exif=? WHERE path = ?|;
my $sth2 = $dbh->prepare($updateSql);

while (my $path = <STDIN>) {
        chomp $path;
        my $realPath = $path;
        my $directory = `dirname "$realPath"`;
        chomp($directory);

        my $width = 0;
        my $height = 0;
        my $exif = "";
        open(EXIF,"exiftool \"$realPath\"|");
        while(my $line = <EXIF>) {
            $exif .= $line;
        }
        close EXIF;

        my $digest = md4_hex($realPath);
        my $first = substr($digest, 0, 1);
        my $second = substr($digest, 1, 1);
        my $cachePrefix = "/$first/$second/";

        my $rv = $sth->execute(encode("UTF-8", $exif), $path, $cachePrefix, $directory, -s $path);
        unless( $rv ) {
            $sth2->execute(encode("UTF-8", $exif), $path);
        }
}

