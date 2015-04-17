# $Id: Sexp.pm,v 1.2 2004/12/24 13:23:51 kawahara Exp kawahara $

package JumanSexp;

use strict;

sub new {
    my ($this, $str) = @_;

    chomp($str);
    $this = {data => &Sparse($str)};
    bless $this;
}


# S parser
sub Sparse {
    my ($str) = @_;
    my (@dat, @stack);

    # @stack: 親の管理
    # @dat: 全データ

    while ($str) {
	# 開き括弧
	if ($str =~ s/^ *\( *([^ \(\)]*)//) {
	    my $elem = $1;
	    push(@dat, {element => $elem, children => [], parent => scalar(@stack) == 0 ? -1 : $stack[-1]});
	    push(@{$dat[$stack[-1]]{children}}, $#dat) if scalar(@stack);
	    push(@stack, $#dat);
	}
	# 閉じ括弧
	elsif ($str =~ s/^ *([^ \(\)]*)\)//) {
	    my $elem = $1;
	    if ($elem) {
		push(@dat, {element => $elem, children => [], parent => $stack[-1], single => 1});
		push(@{$dat[$stack[-1]]{children}}, $#dat);
	    }
	    pop(@stack);
	}
	# 要素のみ
	elsif ($str =~ s/^ *([^ \(\)]+)//) {
	    my $elem = $1;
	    push(@dat, {element => $elem, children => [], parent => $stack[-1], single => 1});
	    push(@{$dat[$stack[-1]]{children}}, $#dat);
	}else{
        print STDERR "Error\@Sparsing ".$str;
        print $str;
        exit 1;
    }
    }
    return \@dat;
}

sub print {
    my ($this) = @_;

    &Sprint($this->{data}, 0);
    print "\n";
}

# S式をprint
sub Sprint {
    my ($sdat, $i, $lastflag) = @_;

    # singleのときは括弧を出力しない
    # $lastflagかつsingleのときはスペースを出力しない

    print '(' unless $sdat->[$i]{single};
    if ($sdat->[$i]{element}) {
	print $sdat->[$i]{element};
	print ' ' unless $lastflag && $sdat->[$i]{single};
    }
    for my $c (0 .. $#{$sdat->[$i]{children}}) {
	&Sprint($sdat, $sdat->[$i]{children}[$c], $c == $#{$sdat->[$i]{children}} ? 1 : 0);
    }
    print ')' unless $sdat->[$i]{single};
}

sub get_single {
    my ($this, $flag) = @_;

    if ($flag) {
	return &GetSingleWithCar($this->{data}, 0);
    }
    else {
	return &GetSingle($this->{data}, 0);
    }
}

# Singleを抽出
sub GetSingle {
    my ($sdat, $i) = @_;
    my (@ret);

    if ($sdat->[$i]{element} and defined($sdat->[$i]{single})) {
	return ($sdat->[$i]{element});
    }
    for my $c (0 .. $#{$sdat->[$i]{children}}) {
	push(@ret, &GetSingle($sdat, $sdat->[$i]{children}[$c]));
    }
    return @ret;
}

# Singleを抽出(car付き)
sub GetSingleWithCar {
    my ($sdat, $i, $base_elems) = @_;
    my (@ret, $elems);

    if ($sdat->[$i]{element} and defined($sdat->[$i]{children}) and @{$sdat->[$i]{children}} == 1 and 
	defined($sdat->[$sdat->[$i]{children}[0]]{single})) {
	return (sprintf("%s/%s/%s", $sdat->[$sdat->[$i]{children}[0]]{element}, $sdat->[$i]{element}, $base_elems));
    }
    for my $c (0 .. $#{$sdat->[$i]{children}}) {
	$elems = $sdat->[$i]{element} ? ($base_elems ? $base_elems . '|' . $sdat->[$i]{element} : $sdat->[$i]{element}) : $base_elems;
	push(@ret, &GetSingleWithCar($sdat, $sdat->[$i]{children}[$c], $elems));
    }
    return @ret;
}

# S式データをhashへ (不完全)
sub S2hash {
    my ($sdat) = @_;
    my (%ret);

    $ret{pos} = $sdat->[0]{element};
    $ret{spos} = $sdat->[1]{element};
    $ret{yomi} = [&GetElemFromS($sdat, '読み')];
    $ret{midashi} = [&GetElemFromS($sdat, '見出し語')];
    $ret{imi} = [&GetElemFromS($sdat, '意味情報')];

    return \%ret;
}

# S式データに要素を追加
sub add {
    my ($this, $car, $cdr) = @_;
    my (@children);
    my $sdat = $this->{data};

    # 追加するもの
    for my $e (@$cdr) {
	push(@$sdat, {element => $e, children => [], single => 1});
	push(@children, $#{$sdat});
    }

    # $carを検索して追加
    for my $i (0 .. $#{$sdat}) {
	if ($sdat->[$i]{element} eq $car) {
	    push(@{$sdat->[$i]{children}}, @children);
	    for my $j (@children) {
		$sdat->[$j]{parent} = $i;
	    }
	    return;
	}
    }

    # $carがないときは「見出し語」の下に作る
    my $p = &GetParentFromS($sdat, '見出し語');
    push(@$sdat, {element => $car, children => [], parent => $p});
    push(@{$sdat->[$p]{children}}, $#{$sdat});
    push(@{$sdat->[$#{$sdat}]{children}}, @children);
    for my $j (@children) {
	$sdat->[$j]{parent} = $#{$sdat};
    }
}

sub get_elem {
    my ($this, $car) = @_;
    &GetElemFromS($this->{data}, $car);
}

# S式データから指定されたキーの子供を得る
sub GetElemFromS {
    my ($sdat, $car) = @_;

    for my $e (@$sdat) {
	if ($e->{element} eq $car) {
	    return map({$sdat->[$_]{element}} @{$e->{children}});
	}
    }
    return ();
}

# S式データから指定されたキーの親番号を得る
sub GetParentFromS {
    my ($sdat, $car) = @_;

    for my $e (@$sdat) {
	if ($e->{element} eq $car) {
	    return $e->{parent};
	}
    }
    return undef;
}

1;
