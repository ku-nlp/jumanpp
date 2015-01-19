#! ruby -Ku
# coding:utf-8 
#

require 'rubygems'

while line = STDIN.gets
    line = line.encode("utf-8","utf-8")

    if(line =~ /^EOS/)
        # 一文出力
        print("\n")
    elsif(line =~ /^([^\*\+\#]|EOS)/ ) #単語なら
        #見通し みとおし 見通し 名詞 6 普通名詞 1 * 0 * 0 "品詞変更:見通し-みとおし-見通す-動詞-*-子音動詞サ行-基本連用形"
        sp = line.split(/\s/)
        if(line =~ /品詞変更:/)
            if(sp[11] =~ /\"品詞変更:(.*)\"/)
                new_sp = $1.split(/-/)
                print "#{new_sp[0]}_#{new_sp[3]} "
            end
        else
            print "#{sp[0]}_#{sp[3]} " 
#            if(sp[5] == '*') # 活用がある場合
#                form = sp[9].gsub(/\p{katakana}*系/,'').gsub(/\p{katakana}*列/,'')
#                print "#{sp[0]}_#{sp[3]}:#{form} " #surface_pos
#            else
#                print "#{sp[0]}_#{sp[3]}:#{sp[5]} " #surface_pos
#            end
        end
    elsif(line =~ /^#/) 
        STDERR.puts line
    end
end

