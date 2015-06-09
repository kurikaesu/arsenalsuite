package Qt::signals;
use Carp;
#
# Proposed usage:
#
# use Qt::signals fooActivated => ['int'];
#
# use Qt::signals fooActivated => {
#     name => 'fooActivated(int)',
#     args => ['int']
# };
#
# sub whatever { emit fooActivated(10); }
#

sub import {
    no strict 'refs';
    my $self = shift;
    my $caller = $self eq "Qt::signals" ? (caller)[0] : $self;
    my $parent = ${ $caller . '::ISA' }[0];

    my $meta = \%{ $caller . '::META' };
    croak "Odd number of arguments in signal declaration" if @_%2;

    my(%signals) = @_;
    for my $signalname (keys %signals) {
		my $signal = { name => $signalname };
		my $args = $signals{$signalname};
		$signal->{arguments} = [map { s/\s(?=[*&])//; { type => $_, name => "" } } @$args];
		my $arglist = join ',', @$args;
		$signal->{prototype} = $signalname . "($arglist)";
		$signal->{returns} = 'void';
		$signal->{method} = $signalname;
		push @{$meta->{signals}}, $signal;
		my $signal_index = $#{ $meta->{signals} } + $#{ $meta->{slots} };
	
		my $argcnt = scalar @$args;
		my $i = 0;
		for my $arg (@$args) {
			my $a = $arg;
			$a =~ s/^const\s+//;
			if($a =~ /^(bool|int|double|char\*|QString)&?$/) {
				$a = $1;
			} else {
				$a = 'ptr';
			}
			$i++;
		}
	
		$meta->{signal}{$signalname} = $signal;
		$signal->{index} = $signal_index;
		$signal->{argcnt} = $argcnt;
		
		*{"$caller\::$signalname"} = sub {
			my @args = ($signal_index,$argcnt,@_);
			Qt::_internal::signal(@args);
		}
    }
    @_ and $meta->{changed} = 1;
}

1;
