
SUBDIRS = src 
ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS} -I m4
DIST_SUBDIRS = $(SUBDIRS) 

resourcedir=$(pkgdatadir)

if NODICT
DICT_DIST = 
DICT_NODIST = 
else
DICT_DIST = jumanpp-resource/dic.base jumanpp-resource/dic.bin jumanpp-resource/dic.da jumanpp-resource/dic.form jumanpp-resource/dic.form_type jumanpp-resource/dic.imis jumanpp-resource/dic.pos jumanpp-resource/dic.read jumanpp-resource/dic.rep jumanpp-resource/dic.spos jumanpp-resource/feature.def jumanpp-resource/freq_words.list jumanpp-resource/lang.mdl jumanpp-resource/lang.mdl.nnet jumanpp-resource/version jumanpp-resource/weight.mdl
DICT_NODIST = jumanpp-resource/lang.mdl.direct jumanpp-resource/lang.mdl.map jumanpp-resource/lang.mdl.nnet.map jumanpp-resource/weight.mdl.map jumanpp-resource/dic.base.map jumanpp-resource/dic.form.map jumanpp-resource/dic.form_type.map jumanpp-resource/dic.imis.map jumanpp-resource/dic.pos.map jumanpp-resource/dic.read.map jumanpp-resource/dic.rep.map jumanpp-resource/dic.spos.map
endif

dist_resource_DATA = $(DICT_DIST)
nodist_resource_DATA = $(DICT_NODIST)


EXTRA_DIST = README.md README_ja.md jumanpp-manual.pdf VERSION script/server.rb script/client.rb script/postprocess.py script/corpus2train.rb script/fullmrp2basep.rb script/jmn2base.py sample/part_sample.txt sample/python_juman.py sample/python_knp.py dict-build/userdic.mdic dict-build/Makefile dict-build/dic/Suffix.dic dict-build/dic/Lexicon_from_rengo.dic dict-build/dic/ContentW.marked_dic dict-build/dic/Townname.dic dict-build/dic/Assert.dic dict-build/dic/AuxV.dic dict-build/dic/Demonstrative.dic dict-build/dic/Emoticon.dic dict-build/dic/Noun.hukusi.dic dict-build/dic/Noun.keishiki.dic dict-build/dic/Noun.koyuu.dic dict-build/dic/Noun.suusi.dic dict-build/dic/Postp.dic dict-build/dic/Prefix.dic dict-build/dic/Rendaku.dic dict-build/dic/Rengo.dic dict-build/dic/Special.dic dict-build/wikipediadic/wikipedia.dic.orig dict-build/webdic/Web.dic dict-build/wiktionarydic/wiktionary.dic dict-build/grammar/JUMAN.grammar dict-build/grammar/JUMAN.kankei dict-build/grammar/JUMAN.katuyou dict-build/scripts/Juman.pm dict-build/scripts/hikkomi.rule dict-build/scripts/JumanSexp.pm dict-build/scripts/update.sh dict-build/scripts/juman2morph.rb dict-build/scripts/jumandic2morphdic.perl dict-build/scripts/.hikkomi.rule.swo dict-build/scripts/mkgrammarpm dict-build/scripts/lib/Sexp.pm dict-build/onomatopedic/Onomatopoeia.dic dict-build/install.sh

jumanpp-resource/lang.mdl.direct: $(dist_resource_DATA) src/jumanpp
	rm -f jumanpp-resource/lang.mdl.direct;\
	rm -f jumanpp-resource/*.map;\
	echo "" | src/jumanpp -D jumanpp-resource

jumanpp-resource/%.map: jumanpp-resource/%.mdl jumanpp-resource/lang.mdl.direct
	:

jumanpp-resource/lang.mdl.nnet.map: jumanpp-resource/lang.mdl.nnet jumanpp-resource/lang.mdl.direct
	:
	
