#!/home/morita/.rbenv/shims/ruby -Ku
#coding: utf-8
#
require "socket"
require 'optparse'

opts = {'host'=>'localhost',
        'port'=>12000}
# オプション
OptionParser.new("Usage: ruby #{__FILE__} [options] [input_tsv]") do |opt|
  opt.on('--host name', 'hostname') do |hostname|
    opts['host'] = hostname
  end
  opt.on('-p number', 'port number') do |number|
    opts['port'] = number.to_i
  end
  opt.parse!
end

#port = 8001
#if(ARGV.size > 0)
#  port = ARGV[0].to_i
#end

s = TCPSocket.open(opts['host'], opts['port'])

while line = STDIN.gets
  next if(line =~ /^\s*$/)
  if(line =~ /^##KKN\t.*$/) # KKN用 動的オプション
    s.write(line)
    puts s.gets # 動的オプションの戻り値表示
    next
  elsif(line =~ /^#.*$/)
    s.write(line) # コメント行
    puts s.gets # リビジョンの付いたコメント行を出力
    next
  end

  s.write(line)
  while true
    f = s.gets
    puts f
    break if f.to_s == "EOS\n"
  end
end
s.close

