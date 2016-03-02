#!/usr/bin/env perl

use JumanSexp;
use Inflection;
use utf8;
use Encode;
use Getopt::Long;
binmode STDIN, ':encoding(utf-8)';
binmode STDOUT, ':encoding(utf-8)';
binmode STDERR, ':encoding(utf-8)';

use strict;
#(接尾辞 (名詞性名詞接尾辞 ((見出し語 ら)(読み ら)(意味情報 "代表表記:ら/ら"))))

# TODO: オプション
my $opt_nominalize = 0;
my $opt_debug = 0;

GetOptions('nominalize' => \$opt_nominalize, 'debug+' => \$opt_debug);


my %dictionary; # 重複チェック用
my @hikkomi_candidates;

my $number = 0;
while (<STDIN>) {
    print STDERR "\r".$number;
    $number = $number +1;
    if($_ =~ /^\s*;/){ next;}
    if($_ =~ /^\s*$/){ next;}
    $_ =~ s/\s*$//; # 末尾の空白削除

    #print STDERR $_;
    #print STDERR "=";
    # (名詞 (副詞的名詞 ((読み とこ)(見出し語 とこ)(意味情報 "代表表記:とこ/とこ"))))
    my $s = new JumanSexp($_);
    my $pos = $s->{data}[0]{element};

    if($pos eq "連語"){next;} # 連語は飛ばす
    # 連語の場合は分解して保存し，後で重複のないものだけを追加

    my $spos = $s->{data}[1]{element};
    my $yomi = ($s->get_elem('読み'))[0];
    my $type = ($s->get_elem('活用型'))[0];
    my $imis_str = join(" ", ($s->get_elem('意味情報'))); # TODO: 配列のまま扱う
    my @imis = $s->get_elem('意味情報');
    my $rep = "*";
    $imis_str =~ s/^"//; #最初と最後のクォーテーションを除く
    $imis_str =~ s/"$//;
    if($imis_str =~ s/代表表記:([^ "]*) ?//){
        $rep = $1;
    }
        
    if($imis_str =~ /,/){ #フォーマットが崩れる
        print STDERR "\n".$number."\n";
        print STDERR "err: 意味情報が','を含む.";
        exit(1);
    }

    my $imis_index = 0;
    my @del_imis_indexs;
    for my $imi (@imis){
        if ($imi =~ /代表表記:([^ "]*) ?/){
            $rep = $1;
            unshift @del_imis_indexs, $imis_index; # 配列の要素を削除する
        }
        $imi =~ s/^"//;
        $imi =~ s/"$//;
        # print STDERR "_".$imi."_";
        $imis_index++;
    }
    for my $ind (@del_imis_indexs){ #index に影響を与えないように後から削除
        splice @imis, $ind, 1;
    }
         
    my $index = 0;
    for my $midasi ($s->get_elem('見出し語')) {# 連語から来たものは，同じ表層で同じ品詞のものがない場合のみ使う
        if ($type) {
            if(!$spos){$spos= "*";}
            my @ms =(&get_inflected_forms(Encode::encode('utf-8',$midasi), Encode::encode('utf-8',$type)));
            my @ys =(&get_inflected_forms(Encode::encode('utf-8',$yomi), Encode::encode('utf-8',$type)));
            #my %hash = map {$ms[$_], $ys[$_]}(0..$#ms); 
            my %hash; @hash{@ms}=@ys;
            for my $m_key (@ms){ # 活用形ごと
                my $mstr = Encode::decode('utf-8', $m_key->{str});
                my $ystr = Encode::decode('utf-8', $hash{$m_key}->{str});
                next unless $mstr;
                my $form = Encode::decode('utf-8', $m_key->{form});
                
                if($pos eq "動詞" && 
                    $form eq "基本連用形" && 
                    not($imis_str =~ /可能動詞/) &&
                    $type ne "サ変動詞" &&
                    # $type ne "子音動詞サ行" && # 愛する等を除きたいが
                    (not ($type eq "子音動詞サ行" && $imis_str =~ /同義:動詞:[^ ]*する/)) && # 愛する, 汗する，などを除く
                    $index == 0 && # 見出しの先頭に出現した単語のみ
                    (not $imis_str =~ /濁音化D/) # 濁音化と送り仮名の引っ込み両方は適応しない
                ){
                    # 送り仮名の処理 
                    my @replace_candidate;
                    my @segments = split(/(\p{Han}\p{Hiragana})(?!\p{Hiragana})/,$mstr);
                        
                    my $seg_index = 0;
                    for my $seg (@segments){
                        if($seg =~ /^\p{Han}\p{Hiragana}$/){
                            # indexを置換えのために保存しておく
                            push @replace_candidate, $seg_index;
                        }
                        $seg_index++;
                    }
                        
                    my @replaced = [ @segments ];
                    for my $pos (@replace_candidate){
                        my @tmp;
                        for my $sp (@replaced){
                            my @sp_2 = @$sp;
                            $sp_2[$pos] = substr($sp_2[$pos],0,1); # 一文字目のみに置き換える
                            push @tmp, \@sp_2;
                        }
                        for my $tmp_2 (@tmp){
                            push @replaced, $tmp_2;
                        }
                    }
                    for my $reps (@replaced){
                        push @hikkomi_candidates, [join("",@$reps),$pos,$spos,$form,$type,$midasi,$ystr,$rep,\@imis];
                    }
                }
                &print_entry($mstr, $pos, $spos ,$form, $type, $midasi, $ystr, $rep, \@imis);
                # 辞書に追加
                $dictionary{$mstr.$pos.$form}=1;
                #$dictionary{$mstr ."/". $ystr . "/". $pos . "/".$spos} = 1;
            }
        } else {# 活用無し
            &print_entry($midasi, $pos, $spos,'*', '*',$midasi, $yomi, $rep, \@imis);
            $dictionary{$midasi.$pos}=1;
            #$dictionary{$midasi . "/". $yomi . "/". $pos . "/".$spos} = 1;
        }
        $index++;
    }

}

# 重複をチェックして問題なければ出力
for my $rep (@hikkomi_candidates){
    if($dictionary{$rep->[0].$rep->[1].$rep->[3]} == 1 || # 動詞自体が登録済み
        $dictionary{$rep->[0]."名詞"} == 1 || # 一致する名詞がある
        $dictionary{$rep->[0]."形容詞"."語幹"} == 1 # 対応する形容詞語幹がある　
    ){
        #print STDERR "skip: ".$rep->[0]."\n";
    }else{
        #print STDERR "register:".$rep->[0]."\n";
        &print_entry($rep->[0], $rep->[1], $rep->[2] ,$rep->[3], $rep->[4], $rep->[5], $rep->[6], $rep->[7], $rep->[8]);
    }
}

#(名詞 (普通名詞 ((読み き゜)(見出し語 (き゜ 1.6))(意味情報 "自動獲得:Wikipedia Wikipedia多義"))))
#あいまって,0,0,0,動詞:タ系連用テ形
#相俟った,0,0,0,動詞:タ形
#あいまいやったろ,0,0,0,形容詞:ヤ列タ系省略推量形
#
#(判定詞 ((見出し語 だ)(読み だ)(活用型 判定詞)))
#な,0,0,0,判定詞:ダ列基本連体形
#あいくるしくて,0,0,0,形容詞:タ系連用テ形
#あいこ,0,0,0,名詞:サ変名詞

sub print_entry {
    my ($h, $pos, $spos, $form, $form_type, $midasi, $yomi, $rep, $imis) = @_;
    my $imis_str;
    if($h eq ','){ $h = '","';}
    if($midasi eq ','){ $midasi = '","';}
    if($yomi eq ','){ $yomi = '","';}
    if( (not defined $imis) || scalar(@$imis) == 0){
        $imis_str = "NIL";
    }else{
        $imis_str = join(" ",@$imis);
    }

    print $h, ',0,0,0,', $pos,',', $spos, ',' , $form, ',', $form_type, ',' , $midasi , ',', $yomi, ',' , $rep, ',' , $imis_str,"\n"; 
    
    if($opt_nominalize && $pos eq "動詞" && $form eq "基本連用形"){
        push @$imis, "名詞化";
        $imis_str = join(" ", @$imis);
        print $h, ',0,0,0,', "名詞",',', "普通名詞", ',' , '*', ',', '*', ',' , $h , ',', $yomi, ',' , $rep."*", ',' , $imis_str,"\n"; 
    }
}

sub get_inflected_forms {
    my ($midasi, $type) = @_;

    my $inf = new Inflection($midasi, $type, '基本形');
    return $inf->GetAllForms();
}
