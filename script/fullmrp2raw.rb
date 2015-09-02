#!/bin/ruby

# full-mrp 形式 の出力から raw 形式に変換するスクリプト
#usage: cat file | fullmrp2mrp.rb 

morph_str = []
while line=gets
  line.gsub!(/#.*$/,"")
  sp = line.split(/[ \n]/)
  sp.each{|fmrp|
    msp = fmrp.split('_')
    print "#{msp[0]}"
  }
  print "\n"
end

