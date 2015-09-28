#!/usr/bin/perl

my $revhash=1501;

open (SET, "gunzip -cd <Unihan-3.2.0.txt.gz |") || die "big5.txt: $!\n";
while (<SET>)
{
        chomp;
        s/\#.*//;

	next unless /^U\+(....)\s+kBigFive\s+(....)/i;

	my ($unicode, $code)=("0x$1", "0x$2");

        eval "\$code=$code;";
        eval "\$unicode=$unicode;";

        die if $code < 0 || $code > 65535;

	my $codeh= int($code/256);
	my $codel= $code % 256;

	my $unicodehash= int($unicode % $revhash);

	die if $codeh < 162 || $codeh == 163 || $codeh > 249 || $codeh == 199 || $codeh == 200;
	die if $codel < 64 || ($codel >= 128 && $codel < 161) || $codel >= 255;

	if (! defined $fwd{$codeh})
	{
	    my %dummy;

	    $fwd{$codeh}= \%dummy;
	}

	$fwd{$codeh}{$codel}=$unicode;

	if (! defined $rev[$unicodehash])
	{
	    my @dummy;

	    $rev[$unicodehash]= \@dummy;
	}

	my $r=$rev[$unicodehash];

	push @$r, "$unicode $code";
}
close(SET);

print '
/*
** Copyright 2000 Double Precision, Inc.
** See COPYING for distribution information.
**
** $Id: big5.pl,v 1.1.1.1 2003/05/07 02:13:57 lfan Exp $
*/

#include "unicode.h"
';

foreach (sort keys %fwd)
{
    my $h=$_;
    my $l;

    printf ("static const unicode_char big5_%02x_lo[64]={", $h);

    for ($l=64; $l < 128; $l++)
    {
	print "\n" if ($l % 16) == 0;
	printf ("%d", $fwd{$h}{$l});
	print "," unless $l >= 127;
    }
    print "};\n";

    printf ("static const unicode_char big5_%02x_hi[94]={\n", $h);

    for ($l=161; $l < 255; $l++)
    {
	print "\n" if ($l % 16) == 0;
	printf ("%d", $fwd{$h}{$l});
	print "," unless $l >= 254;
    }
    print "};\n";
}

print "static const unsigned big5_revhash_size=$revhash;
static const unicode_char big5_revtable_uc[]={\n";

my $index=0;

my $maxl=0;

for ($i=0; $i<$revhash; $i++)
{
    my $a= $rev[$i];

    $revindex[$i]=$index;

    my $v;

    my @aa=@$a;

    $maxl= $#aa if $#aa > $maxl;

    while (defined ($v=shift @aa))
    {
	print "," if $index > 0;
	print "\n" if $index && ($index % 16) == 0;

	$v =~ s/ .*//;
	print $v;
	++$index;
    }
}

print "};\nstatic const unsigned big5_revtable_octets[]={\n";

$index=0;
for ($i=0; $i<$revhash; $i++)
{
    my $a= $rev[$i];

    my $v;

    my @aa=@$a;

    while (defined ($v=shift @aa))
    {
	print "," if $index > 0;
	print "\n" if $index && ($index % 16) == 0;

	$v =~ s/.* //;
	print $v;
	++$index;
    }
}

print "};\nstatic const unsigned big5_revtable_index[]={\n";

for ($i=0; $i<$revhash; $i++)
{
    print "," if $i > 0;
    print "\n" if $i && ($i % 16) == 0;
    print $revindex[$i];
}

print "};\n";
