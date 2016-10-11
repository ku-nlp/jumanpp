#!/home/morita/.rbenv/shims/ruby -Ku
#coding: utf-8

require "socket"
require 'optparse'

opts = {'command'=>'jumanpp',
        'port'=>12000}
# オプション
OptionParser.new("Usage: ruby #{__FILE__} [options] [input_tsv]") do |opt|
  opt.on('--cmd <command>', 'command') do |command|
    opts['command'] = command
  end
  opt.on('-p number', 'port number') do |number|
    opts['port'] = number.to_i
  end
  opt.parse!
end

server = TCPServer.open(opts['port'])

rnnlm = IO.popen(opts['command'], "r+") 

while true
  socket = server.accept
  while buffer = socket.gets
    STDERR.puts buffer
      
    if (buffer =~ /^##JUMAN++\t.*/) # 動的オプション
      rnnlm.puts(buffer) 
      socket.puts rnnlm.gets # 動的オプションの戻り値(必ず1行)
    elsif(buffer =~ /^#.*/) # コメント行
      rnnlm.puts(buffer) 
      socket.puts rnnlm.gets 
    else
      begin
        responce = ""
        rnnlm.puts(buffer)
        
        while true
          f = rnnlm.gets
          responce += f.to_s
          break if f.to_s == "EOS\n"
        end
        socket.puts responce
      rescue
        socket.close
      end 
    end
  end

  socket.close
end
