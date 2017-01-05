#!/bin/zsh

local -A opthash
zparseopts  -D -M -A opthash -- h -help=h v -version=v -prefix: -PREFIX:=-prefix 

if [[ -n "${opthash[(i)-h]}" ]]; then
    echo "--prefix PATH, --PREFIX PATH   install files in PATH [/usr/local/share/]"
    echo "--help, -h   show this message"
    echo "--version, -v   print version"
    exit
fi

if [[ -n "${opthash[(i)-v]}" ]]; then
    echo "install script v0.1"
    exit
fi

if [[ -n "${opthash[(i)--prefix]}" ]]; then
    PREFIX=${opthash[--prefix]}
    PREFIX=`echo $PREFIX | sed "s/^=//"`
else
    PREFIX=/usr/local/
fi

mkdir -p $PREFIX/share/jumanpp/
DICDIR=./jumanpp_dic

echo cp $DICDIR/dic.base $DICDIR/dic.bin $DICDIR/dic.da $DICDIR/dic.form $DICDIR/dic.form_type $DICDIR/dic.imis $DICDIR/dic.pos $DICDIR/dic.read $DICDIR/dic.rep $DICDIR/dic.spos $PREFIX/share/jumapp/.
cp $DICDIR/dic.base $DICDIR/dic.bin $DICDIR/dic.da $DICDIR/dic.form $DICDIR/dic.form_type $DICDIR/dic.imis $DICDIR/dic.pos $DICDIR/dic.read $DICDIR/dic.rep $DICDIR/dic.spos $PREFIX/share/jumanpp/. 

echo cd $PREFIX/share/jumanpp/
cd $PREFIX/share/jumanpp/

echo rm -f dic.imis.map dic.form.map dic.base.map dic.form_type.map dic.pos.map dic.read.map dic.rep.map dic.spos.map 
rm -f dic.imis.map dic.form.map dic.base.map dic.form_type.map dic.pos.map dic.read.map dic.rep.map dic.spos.map 

echo "Creating cache files"
echo "" | jumanpp -D $PREFIX/share/jumanpp/

