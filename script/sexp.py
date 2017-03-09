#!/bin/python3
#coding:utf-8

import re

class SParser(object):
    def __init__(self):
        self.re_word = re.compile(r'^("(?:[^"\\]+|\\.)*")')
        self.re_symbol = re.compile(r'^([^\s"()]+)')
    
    def parse(self, input_text):
        """
        S 式のパーサ
        >>> t.parse(r'(名詞 (見出し語 犬)(意味情報))')
        [['名詞', ['見出し語', '犬'], ['意味情報']]]
        
        >>> t.parse(r'(名詞 (見出し語 犬)(意味情報 "意味情報の中身 空白もある \\"エスケープもできる "))')
        [['名詞', ['見出し語', '犬'], ['意味情報', '"意味情報の中身 空白もある \\\\"エスケープもできる "']]]
        
        >>> t.parse("(途中で (改行 しても)\\n(よい))")
        [['途中で', ['改行', 'しても'], ['よい']]]

        >>> t.parse("(途中で (改行 しても);コメントを入れても\\n(よい))")
        [['途中で', ['改行', 'しても'], ['よい']]]
        
        # 閉じていない文字列には例外を返す
        >>> t.parse(r'(名詞 (見出し語 犬)(意味情報 "閉じていない文字列))')
        Traceback (most recent call last):
          ...
        Exception: Syntax error: end of target during string.
        
        # 閉じていない括弧には例外を返す
        >>> t.parse(r'(名詞 (見出し語 犬(意味情報 "閉じていない括弧"))')
        Traceback (most recent call last):
            ...
        Exception: Syntax error: end of target during parsing.
        
        # 複数の式を入れても良い
        >>> t.parse(r'(名詞 犬)(名詞 猫)')
        [['名詞', '犬'], ['名詞', '猫']]
        
        # 閉じすぎは例外を返す
        >>> t.parse(r'(名詞 犬))(名詞 猫)')
        Traceback (most recent call last):
            ...
        Exception: Syntax error: too much close brackets.
        
        # テスト
        """
         
        texts = input_text.split('\n')
        string = ""
        stack = []
        offsets = []
        text_itr = iter(texts)
        while True:
            string = string.lstrip()
            if string.startswith(";"):
                # 行を飛ばす
                string = "" # 複数行の入力は無い
            if len(string) == 0:
                try:
                    string = next(text_itr)
                except StopIteration:
                    if(len(offsets) > 0):
                        raise Exception("Syntax error: end of target during parsing.")
                    else:
                        break
            elif string.startswith("("):
                string = string[1:]
                if(len(offsets) > 0):
                    offsets[0] -= 1
                offsets.insert(0,0)
            elif string.startswith('"'):
                while True:
                    match = self.re_word.search(string)
                    if match:
                        offsets[0] -=1
                        stack.append(match.group(1))
                        string = string[len(match.group(1)):]
                        break
                    else:
                        try:
                            string += next(text_itr)
                        except StopIteration:
                            raise Exception("Syntax error: end of target during string.")
            elif self.re_symbol.search(string):
                match = self.re_symbol.search(string)
                offsets[0] -= 1
                stack.append(match.group(1))
                string = string[len(match.group(1)):]
            elif string.startswith(")"):
                string = string[1:]
                if not (len(offsets) > 0):
                    raise Exception("Syntax error: too much close brackets.")
                else:
                    offset = offsets.pop(0)
                    if offset < 0:
                        sl = stack[offset:]
                        del stack[offset:]
                        stack.append(sl)
                    else:
                        stack.append([])
            else:
                raise Exception("Syntax error: unknown syntax element.")
        return stack

if __name__ == "__main__":
    import doctest
    doctest.testmod(extraglobs={'t': SParser()})


