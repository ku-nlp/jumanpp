#!/usr/bin/env perl

use strict;
use utf8;
use Getopt::Long;

binmode STDOUT, ":encoding(utf8)";
binmode STDERR, ":encoding(utf8)";

my (%opt); GetOptions(\%opt, 'verbose', 'all');

my $USAGE = "Usage:\t$0 output_file ref_file [OPTIONS]\n";
$USAGE .= "   -v (--verbose): output diff between output and ref\n";

if (@ARGV != 2) {print "$USAGE"; exit;}

&main();

sub main {
    open(OUTPUT, "<:encoding(utf8)", $ARGV[0]) || die;
    my @output = grep({$_ !~ /^\#/} <OUTPUT>);
    close OUTPUT;

    open(REF, "<:encoding(utf8)", $ARGV[1]) || die;
    my @ref = grep({$_ !~ /^\#/} <REF>);
    close REF;

    if (scalar(@output) != scalar(@ref)) {
	print STDERR "Error: number of sentences does not match\n";
	exit;
    }

    &calc_score(\@output, \@ref);
}

sub calc_score {
    my ($output, $ref) = @_;

    my $pre_c = 0;
    my $rec_c = 0;
    my $token_ok_c = 0;
    my $joint_ok_c = 0;

    my $numofsent = scalar(@$output);
    for (my $i = 0; $i < $numofsent; $i++) {
	my @output_token;
	my @output_pos;
	&parse_line($output->[$i], \@output_token, \@output_pos);

    #if (scalar(@output_token) == 0) { next; }# 解析できなかった文を評価から除外する

	$pre_c += scalar(@output_token);
	my @ref_token;
	my @ref_pos;
	&parse_line($ref->[$i], \@ref_token, \@ref_pos);
	$rec_c += scalar(@ref_token);

    if (scalar(@output_token) == 0) { next; }# 解析できなかった文の処理をスキップ

	my $output_length = 0;
	my $output_index = 0;
	my $output_buffer = "";
	my $ref_length = 0;
	my $ref_index = 0;
	my $ref_buffer = "";
	my $tmp_token_ok_c = 0;
	my $tmp_joint_ok_c = 0;


	while (1) {
	    # 単語が一致している
	    if ($output_token[$output_index] eq $ref_token[$ref_index]) {
		$tmp_token_ok_c++;
		if ($output_pos[$output_index] eq $ref_pos[$ref_index]) {
		    $tmp_joint_ok_c++;
		    $output_buffer .= " $output_token[$output_index]_$output_pos[$output_index]";
		} else {
		    $output_buffer .= " $output_token[$output_index]x$output_pos[$output_index]";
		}
		$output_length += length($output_token[$output_index]);
		$ref_length += length($ref_token[$ref_index]);
		$ref_buffer .= " $ref_token[$ref_index]_$ref_pos[$ref_index]";
		$output_index++;
		$ref_index++;
	    }
	    else {
		# 正解の単語の方が長い
		if ($output_length + length($output_token[$output_index]) <
		    $ref_length + length($ref_token[$ref_index])) {
		    $output_length += length($output_token[$output_index]);
		    $output_buffer .= "*$output_token[$output_index]_$output_pos[$output_index]";
		    $output_index++;
		}
		# 出力の単語の方が長い
		elsif ($output_length + length($output_token[$output_index]) >
		    $ref_length + length($ref_token[$ref_index])) {
		    $ref_length += length($ref_token[$ref_index]);
		    $ref_buffer .= " $ref_token[$ref_index]_$ref_pos[$ref_index]";
		    $ref_index++;
		}
		# 調整して一致した場合
		else {
		    $output_length += length($output_token[$output_index]);
		    $output_buffer .= "*$output_token[$output_index]_$output_pos[$output_index]";
		    $output_index++;
		    $ref_length += length($ref_token[$ref_index]);
		    $ref_buffer .= " $ref_token[$ref_index]_$ref_pos[$ref_index]";
		    $ref_index++;
		}
	    }
	    # どちらかが最後に到達したら
	    if ($output_index == scalar(@output_token) && $ref_index == scalar(@ref_token)) {
		last;
	    }
	    elsif ($output_index == scalar(@output_token)) {
		while ($ref_index != scalar(@ref_token)) {
		    $ref_length += length($ref_token[$ref_index]);
		    $ref_buffer .= " $ref_token[$ref_index]_$ref_pos[$ref_index]";
		    $ref_index++;
		}
		last;
	    } elsif ($ref_index == scalar(@ref_token)) {
		while ($output_index != scalar(@output_token)) {
		    $output_length += length($output_token[$output_index]);
		    $output_buffer .= "*$output_token[$output_index]_$output_pos[$output_index]";
		    $output_index++;
		}
		last;
	    }
	}

	if ($output_length != $ref_length) {
	    print STDERR "Error: sentence length does not match ($i) $output_length v.s. $ref_length\n";
	    print STDERR "@output_token \n";
	    print STDERR "@ref_token \n";
	    next;
	} else {
	    if ($opt{all} || ($opt{verbose} && $output_buffer ne $ref_buffer)) {
		print "REF:$ref_buffer\n";
		print "OUT:$output_buffer\n\n";
	    }
	    $token_ok_c += $tmp_token_ok_c;
	    $joint_ok_c += $tmp_joint_ok_c;
	}
    }

    my $token_precision = 100*$token_ok_c/$pre_c;
    my $token_recall = 100*$token_ok_c/$rec_c;
    my $joint_precision = 100*$joint_ok_c/$pre_c;
    my $joint_recall = 100*$joint_ok_c/$rec_c;
    printf "            Precision              Recall          F\n";
    printf "  Seg: %3.2f (%5d/%5d)  %3.2f (%5d/%5d)  %.2f\n",
	$token_precision, $token_ok_c, $pre_c,
	    $token_recall, $token_ok_c, $rec_c,
		(2*$token_precision*$token_recall)/($token_precision+$token_recall);
    printf "Joint: %3.2f (%5d/%5d)  %3.2f (%5d/%5d)  %.2f\n",
	$joint_precision, $joint_ok_c, $pre_c,
	    $joint_recall, $joint_ok_c, $rec_c,
		(2*$joint_precision*$joint_recall)/($joint_precision+$joint_recall);
}

sub parse_line {
    my ($line, $token_ref, $pos_ref) = @_;
    $line =~ s/^\s+//;
    $line =~ s/\s+$//;
    foreach (split(/ /, $line)) {
	my ($word, $pos) = split(/\_/, $_);
	next if ($pos eq "");
	next if ($pos =~ /^\<.+\>$/);
	$word = " " if ($word eq "");
	push(@$token_ref, $word);
	push(@$pos_ref, $pos);
    }
}

