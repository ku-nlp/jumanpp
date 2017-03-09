#!/usr/bin/env perl

use lib "./lib/";
use Grammar qw/ $FORM $TYPE $HINSI $BUNRUI/;
use strict;
use warnings;

use utf8;
use Encode;
use Getopt::Long;
binmode STDIN, ':encoding(utf-8)';
binmode STDOUT, ':encoding(utf-8)';
binmode STDERR, ':encoding(utf-8)';


# 品詞リストの生成
my $pos_map_prefix  = "const std::unordered_map<std::string, int> Dic::pos_map{\n";
my $pos_map_postfix = "};\n";
my @pos_str_list;

while ( my ($key, $value) = each(%{ $HINSI->[0] }) ) {
    push( @pos_str_list, '{"'.Encode::decode('UTF-8',$key).'", '.$value.'}'); 
}
print $pos_map_prefix.join(",\n", @pos_str_list).$pos_map_postfix;

# 品詞細分類変換マップの生成
my $spos_map_prefix  = "const std::unordered_map<std::string, int> Dic::spos_map{\n{\"*\", 0},\n";
my $spos_map_postfix = "};\n";
my @spos_str_list;

while ( my ($spos_name, $spos_hash) = each(%{ $BUNRUI }) ) {
    while ( my ($key, $value) = each(%{ $spos_hash->[0] }) ) {
        push( @spos_str_list, '{"'.Encode::decode('UTF-8',$key).'", '.$value.'}'); 
    }
}
print $spos_map_prefix.join(",\n", @spos_str_list).$spos_map_postfix;

# 活用型変換マップの生成
my $type_map_prefix  = "const std::unordered_map<std::string, int> Dic::katuyou_type_map{\n";
my $type_map_postfix = "};\n";
my @type_str_list;

while ( my ($key, $value) = each(%{ $TYPE->[0] }) ) {
    push( @type_str_list, '{"'.Encode::decode('UTF-8',$key).'", '.$value.'}'); 
}
print $type_map_prefix.join(",\n", @type_str_list).$type_map_postfix;

# 活用形変換マップの生成
#const std::unordered_map<std::string, int> Dic::katuyou_form_map{ {"母音動詞:語幹", 1}, //{{{
#    {"*:*", 0},
#    {"母音動詞:基本形", 2},
#    {"母音動詞:未然形", 3},
#    {"母音動詞:意志形", 4},
#    {"母音動詞:省略意志形", 5},

my $form_map_prefix  = "const std::unordered_map<std::string, int> Dic::katuyou_form_map{\n";
my $form_map_postfix = "};\n";
my @form_str_list;

while ( my ($form_name, $form_hash) = each(%{ $FORM }) ) {
    while ( my ($key, $value) = each(%{ $form_hash->[0] }) ) {
        push( @form_str_list, '{"'.Encode::decode('UTF-8',$form_name).':'.Encode::decode('UTF-8',$key).'", '.$value.'}'); 
    }
}
print $form_map_prefix.join(",\n", @form_str_list).$form_map_postfix;


