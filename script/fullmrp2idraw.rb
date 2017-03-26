#!/bin/ruby

# id 付き full-mrp 形式 の出力から id 付きの raw 形式に変換するスクリプト
#usage: cat file | fullmrp2mrp.rb 
id_map = Hash.new

morph_str = []
while line=gets
  id = ""
  if( line =~ /^(.*) # (.*)/ )  
    sent = $1
    id = $2
  else
    STDERR.puts "idがついていない文: #{line}"
    exit 1
  end
  sp = sent.split(/[ \n]/)
  str = ""
  sp.collect{|fmrp|
    msp = fmrp.split('_')
    str += "#{msp[0]}"
  }
  id_map[id] = str

  puts "# #{id}"
  puts str
end


