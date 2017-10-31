jpp_core_files(core_srcs
  darts_trie.cc
  dic_builder.cc
  dictionary.cc
  field_import.cc
  )

jpp_core_files(core_tsrcs
  darts_trie_test.cc
  dictionary_test.cc
  field_import_test.cc
  field_reader_test.cc

  )

jpp_core_files(core_hdrs
  darts.h
  darts_trie.h
  dic_builder.h
  dic_entries.h
  dictionary.h
  field_import.h
  field_reader.h
  progress.h
  )

