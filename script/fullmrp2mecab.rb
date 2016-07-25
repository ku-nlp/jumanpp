#!/bin/ruby

# full-mrp 形式 の出力から mecab の出力形式に変換するスクリプト
#usage: cat file | fullmrp2mecab.rb 

#output
# kn2mecab の出力
#コイン	名詞,6,普通名詞,1,コイン,こいん
# output
#官邸   かんてい,官邸,名詞,普通名詞,*,*,imis

#input
#官邸_かんてい_官邸_名詞_普通名詞_*_*
#
morph_str = []
while line=gets
  line.gsub!(/#.*$/,"")
  sp = line.split(/[ \n]/)
  sp.each{|fmrp|
    msp = fmrp.split('_')
    print "#{msp[0]}\t#{msp[3]},#{msp[4]},#{msp[5]},#{msp[6]},#{msp[2]},#{msp[1]},nil \n"
  }
  print "EOS\n"
end
