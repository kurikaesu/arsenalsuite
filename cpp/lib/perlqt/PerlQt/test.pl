
use Qt;
use Blur::Model::User;
use Blur::Model::Job;

warn "here1\n";
my $jobtable = Blur::Model::Job->table();
warn "here\n";
my $field = Blur::Model::Field->new($jobtable, 'statusLastChanged', 0, 6, 0, 'statusLastChanged' );
warn "there\n";

#my $job = Blur::Model::Job->new();
#print $job->statusLastChanged()->toString() . "\n";


__END__
