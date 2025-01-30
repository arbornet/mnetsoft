#
# Useful grex functions.
#

package Grex;

sub yesno
{
	my @question = @_;
	return 0 unless @question;
	print @question, " [Yn]: ";
	my $response = <>;
	chomp $response;
	return $response !~ /[Nn]/;
}

sub noyes
{
	my @question = @_;
	return 0 unless @question;
	print @question, " [Ny]: ";
	my $response = <>;
	chomp $response;
	return $response !~ /[Yy]/;
}

1;
