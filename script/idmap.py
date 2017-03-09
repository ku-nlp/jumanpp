#!/bin/python3
#coding:utf-8

import sys
from sexp import SParser

class IdMap(object):
    def __init__(self):
        self.pos2id = {}
        self.pos_spos2id = {}
        self.type2id = {}
        self.type_form2id = {}
        self.sp = SParser()

    def parse_grammar(self,text) : 
        """
        >>> t = IdMap()
        >>> t.parse_grammar("((特殊)((句点)(読点)(括弧始)(括弧終)(記号)(空白)))")
        >>> print(sorted(t.pos2id.items()))
        [('特殊', 0)]
        >>> print(sorted(t.pos_spos2id.items()))
        [(('特殊', '句点'), 1), (('特殊', '括弧始'), 3), (('特殊', '括弧終'), 4), (('特殊', '空白'), 6), (('特殊', '記号'), 5), (('特殊', '読点'), 2)]
        
        >>> t.parse_grammar("((動詞 %))")
        >>> print(sorted(t.pos2id.items()))
        [('動詞', 1), ('特殊', 0)]

        >>> t.parse_grammar("((名詞)\\n ((普通名詞)\\n (サ変名詞)\\n (固有名詞)\\n (地名)\\n (人名)\\n (組織名)\\n (数詞)\\n (形式名詞)\\n (副詞的名詞)\\n (時相名詞)))")
        >>> print(sorted(t.pos2id.items()))
        [('動詞', 1), ('名詞', 2), ('特殊', 0)]
        >>> print(sorted(t.pos_spos2id.items()))
        [(('名詞', 'サ変名詞'), 2), (('名詞', '人名'), 5), (('名詞', '副詞的名詞'), 9), (('名詞', '固有名詞'), 3), (('名詞', '地名'), 4), (('名詞', '形式名詞'), 8), (('名詞', '数詞'), 7), (('名詞', '時相名詞'), 10), (('名詞', '普通名詞'), 1), (('名詞', '組織名'), 6), (('特殊', '句点'), 1), (('特殊', '括弧始'), 3), (('特殊', '括弧終'), 4), (('特殊', '空白'), 6), (('特殊', '記号'), 5), (('特殊', '読点'), 2)]
        
        >>> t.parse_grammar("((形容詞 %))\\n\\n((判定詞 %))")
        >>> print(sorted(t.pos2id.items()))
        [('判定詞', 4), ('動詞', 1), ('名詞', 2), ('形容詞', 3), ('特殊', 0)]

        """
        # Sample
        #((特殊)
        #       ((句点)
        #        (読点)
        #        (括弧始)        ; 追加 94/01/14 kuro
        #        (括弧終)        ; 追加 94/01/14 kuro
        #        (記号)
        #        (空白)))        ; 追加 96/08/23 kuro
        
        for s_exp in self.sp.parse(text):
            pos = s_exp[0][0]
            self.pos2id[pos] = len(self.pos2id)
            spos_id = 1
            if(len(s_exp) > 1):
                for spos in [x[0] for x in s_exp[1]]:
                    self.pos_spos2id[(pos, spos)] = spos_id
                    spos_id += 1
            else:
                self.pos_spos2id[(pos, '*')] = 0
        return 
    
    def parse_katuyou(self, text) :
        """
        katuyou.JUMAN を parseする
        >>> t = IdMap()
        >>> t.parse_katuyou("(母音動詞((語幹             *   ) (基本形           る  ) (未然形           *   ) (意志形           よう) (省略意志形       よ  ) (命令形           ろ  ) (基本条件形       れば) (基本連用形       *   ) (タ接連用形       *   ) (タ形             た  ) (タ系推量形       たろう) (タ系省略推量形   たろ) (タ系条件形       たら) (タ系連用テ形     て  ) (タ系連用タリ形   たり) (タ系連用チャ形   ちゃ) (音便条件形       りゃ) (文語命令形       よ  ) ;     (文語巳然形       れ  )\\n))")
        >>> print(sorted(t.type2id.items()))
        [('母音動詞', 0)]
        
        >>> print(sorted(t.type_form2id.items()))
        [(('母音動詞', 'タ形'), 10), (('母音動詞', 'タ接連用形'), 9), (('母音動詞', 'タ系推量形'), 11), (('母音動詞', 'タ系条件形'), 13), (('母音動詞', 'タ系省略推量形'), 12), (('母音動詞', 'タ系連用タリ形'), 15), (('母音動詞', 'タ系連用チャ形'), 16), (('母音動詞', 'タ系連用テ形'), 14), (('母音動詞', '命令形'), 6), (('母音動詞', '基本形'), 2), (('母音動詞', '基本条件形'), 7), (('母音動詞', '基本連用形'), 8), (('母音動詞', '意志形'), 4), (('母音動詞', '文語命令形'), 18), (('母音動詞', '未然形'), 3), (('母音動詞', '省略意志形'), 5), (('母音動詞', '語幹'), 1), (('母音動詞', '音便条件形'), 17)]

        >>> t.parse_katuyou("(子音動詞カ行\\n ((語幹             *     )\\n (基本形           く    )\\n (未然形           か    )\\n (意志形           こう  )\\n (省略意志形       こ  )\\n (命令形           け    )\\n (基本条件形       けば  )\\n (基本連用形       き    )\\n (タ接連用形       い    )\\n (タ形             いた  )\\n (タ系推量形       いたろう)\\n (タ系省略推量形   いたろ)\\n (タ系条件形       いたら)\\n (タ系連用テ形     いて  )\\n (タ系連用タリ形   いたり)\\n (タ系連用チャ形   いちゃ)\\n \\n (音便条件形       きゃ  )\\n \\n ;     (文語巳然形       け    )\\n))")
        >>> print(t.type2id['母音動詞'])
        0
        >>> print(t.type_form2id[('子音動詞カ行','基本形')])
        2
                
        """
        for s_exp in self.sp.parse(text):
            conj_type = s_exp[0]
            self.type2id[conj_type]=len(self.type2id)
            conj_id = 1
            if(len(s_exp)>1):
                for conj_form, *gobi in s_exp[1]:
                    self.type_form2id[(conj_type, conj_form)] = conj_id
                    conj_id += 1
            else:
                self.type_form2id[(conj_type,'*')] = 0

if __name__ == "__main__":
    import doctest
    doctest.testmod(extraglobs={'t': IdMap()})


