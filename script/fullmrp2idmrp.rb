#!/bin/ruby

# full-mrp 形式 の出力から morph の出力形式に変換するスクリプト
#usage: cat file | fullmrp2mrp.rb 

#村山_むらやま_村山_名詞_人名_*_* 
#富市_とみいち_富市_名詞_人名_*_* 
#首相_しゅしょう_首相_名詞_普通名詞_*_* 
#は_は_は_助詞_副助詞_*_* 
#年頭_ねんとう_年頭_名詞_普通名詞_*_* 
#に_に_に_助詞_格助詞_*_* 
#あたり_あたり_あたる_動詞_*_子音動詞ラ行_基本連用形 
#首相_しゅしょう_首相_名詞_普通名詞_*_* 
#官邸_かんてい_官邸_名詞_普通名詞_*_*

morph_str = []
while line=gets
  if( line =~ /^(.*) # (.*)/ )  
    sent = $1
    id = $2
    puts "# #{id}"
  else
    STDERR.puts "idがついていない文: #{line}"
    exit 1
  end
  sp = sent.split(/[ \n]/)
  sp.each{|fmrp|
    msp = fmrp.split('_')
    print "#{msp[0]}_#{msp[3]}:#{msp[4]} "
  }
  print "\n"
end

