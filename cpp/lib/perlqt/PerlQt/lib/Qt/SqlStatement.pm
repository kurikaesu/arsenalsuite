
package Qt::SqlStatement;

sub execute {
	my $self = shift;
	Qt::GlobalSpace::blurDb()->connection()->checkConnection();
	my $ret = Qt::GlobalSpace::blurDb()->exec($self->{sql},\@_);
	$self->{QUERY} = $ret;
}

sub fetchrow_hashref {
	my $self = shift;
	warn "QUERY not active\n" and return undef unless exists $self->{QUERY};
	my $q = $self->{QUERY};
	return undef unless $q->next();
	my $cnt = $q->fieldCount() - 1;
	my %ret;
	for(0..$cnt) {
		$ret{$q->fieldName($_)} = $q->value($_);
	}
	return \%ret;
}

sub fetchall_hashref {
	my ($self, $key) = @_;
	my %ret;
	while( my $row = $self->fetchrow_hashref ) {
		$ret{$row->{$key}} = $row;
	}
	return \%ret;
}

sub fetchrow_arrayref {
	my $self = shift;
	warn "QUERY not active\n" and return undef unless exists $self->{QUERY};
	my $q = $self->{QUERY};
	return undef unless $q->next();
	my $cnt = $q->fieldCount() - 1;
	my @ret;
	for(0..$cnt) {
		push @ret, $q->value($_);
	}
	return \@ret;
}

sub fetchrow_array {
	my $self = shift;
	warn "QUERY not active\n" and return undef unless exists $self->{QUERY};
	my $q = $self->{QUERY};
	my @ret;
	return @ret unless $q->next();
	my $cnt = $q->fieldCount() - 1;
	for(0..$cnt) {
		push @ret, $q->value($_);
	}
	return @ret;
}

sub finish {}

1;
