#! /usr/bin/ruby
# coding:utf-8

sent = []
while line=gets do
  #客 きゃく 客 名詞 6 普通名詞 1 * 0 * 0 "代表表記:客/きゃく 漢字読み:音 カテゴリ:人 ドメイン:家庭・暮らし;ビジネス"
  next if line =~ /^[*+#\@!]/
  next if line =~ /^\s*$/
  if line =~ /(^EOS|^EOP)/
    puts sent.join(" ")
    sent = []
    next
  end
  sp = line.split(" ")
  if(line =~ /代表表記:([^\s"]*)/)
      sent << ($1)
  else
      sent << (sp[2] + "/" + sp[2])
  end
end

