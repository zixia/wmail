#!/usr/bin/perl

my $revhash=1050;

open (SET, "gunzip -cd <Unihan-3.2.0.txt.gz |") || die "gb2312.txt: $!\n";
while (<SET>)
{
        chomp;
        s/\#.*//;

	next unless /^U\+(....)\s+kIRG_GSource\s+0\-(....)/;

	my ($code, $unicode)=("0x$2", "0x$1");

        next unless $code ne "";

        eval "\$code=$code;";
        eval "\$unicode=$unicode;";

        die if $code < 0 || $code > 65535;

	$code |= 0x8080;

	my $codeh= int($code/256);
	my $codel= $code % 256;

	my $unicodehash= int($unicode % $revhash);

	die if $codeh < 0xB0 || $codeh > 0xF7;
	die if $codel < 0xA1 || $codel > 0xFE;

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
** Copyright 2000-2001 Double Precision, Inc.
** See COPYING for distribution information.
**
** $Id: gb2312.pl,v 1.1.1.1 2003/05/07 02:13:57 lfan Exp $
*/

#include "unicode.h"
';

foreach (sort keys %fwd)
{
    my $h=$_;
    my $l;

    printf ("static const unicode_char gb2312_%02x[94]={", $h);

    for ($l=0xA1; $l < 0xFF; $l++)
    {
	print "\n" if ($l % 16) == 0;
	printf ("%d", $fwd{$h}{$l});
	print "," unless $l >= 0xFE;
    }
    print "};\n";

}

print "static const unsigned gb2312_revhash_size=$revhash;
static const unicode_char gb2312_revtable_uc[]={\n";

my $index=0;

for ($i=0; $i<$revhash; $i++)
{
    my $a= $rev[$i];

    $revindex[$i]=$index;

    my $v;

    my @aa=@$a;

    while (defined ($v=shift @aa))
    {
	print "," if $index > 0;
	print "\n" if $index && ($index % 16) == 0;

	$v =~ s/ .*//;
	print $v;
	++$index;
    }
}

print "};\nstatic const unsigned gb2312_revtable_octets[]={\n";

$maxl=0;
$index=0;
for ($i=0; $i<$revhash; $i++)
{
    my $a= $rev[$i];

    my $v;

    my @aa=@$a;

    $maxl=$#aa if $#aa > $maxl;
    while (defined ($v=shift @aa))
    {
	print "," if $index > 0;
	print "\n" if $index && ($index % 16) == 0;

	$v =~ s/.* //;
	print $v;
	++$index;
    }
}

print "};\nstatic const unsigned gb2312_revtable_index[]={\n";

for ($i=0; $i<$revhash; $i++)
{
    print "," if $i > 0;
    print "\n" if $i && ($i % 16) == 0;
    print $revindex[$i];
}

print "};\n";
