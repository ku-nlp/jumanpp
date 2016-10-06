#/bin/env python2
#encoding: utf-8
from pyknp import KNP
import sys
import codecs
sys.stdin = codecs.getreader('utf_8')(sys.stdin)
sys.stdout = codecs.getwriter('utf_8')(sys.stdout)

# Use Juman with KNP
knp_with_juman = KNP()
result = knp_with_juman.parse(u"なのはがなのはなとごっちゃなのはよみにくい")
print ','.join(mrph.midasi for bunsetsu in result for mrph in bunsetsu.mrph_list() )

# Use Juman++ with KNP
knp_with_jumanpp = KNP(jumancommand="jumanpp")
result = knp_with_jumanpp.parse(u"なのはがなのはなとごっちゃなのはよみにくい")
print ','.join(mrph.midasi for bunsetsu in result for mrph in bunsetsu.mrph_list() )

# loop for tag (kihonku, basic phrase)
for tag in result.tag_list():
    print u"ID:%s, 見出し:%s, 係り受けタイプ:%s, 親文節ID:%s, 素性:%s" \
    % (tag.tag_id, "".join(mrph.midasi for mrph in tag.mrph_list()),
    tag.dpndtype, tag.parent_id, tag.fstring)

# loop for mrph
for mrph in result.mrph_list():
    print u"見出し:%s, 読み:%s, 原形:%s, 品詞:%s, 品詞細分類:%s, 活用型:%s, 活用形:%s, 意味情報:%s, 代表表記:%s" \
    % (mrph.midasi, mrph.yomi, mrph.genkei, mrph.hinsi, mrph.bunrui,
    mrph.katuyou1, mrph.katuyou2, mrph.imis, mrph.repname)
