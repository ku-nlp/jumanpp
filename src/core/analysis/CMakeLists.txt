jpp_core_files(core_srcs

  analysis_input.cc
  analysis_result.cc
  analyzer.cc
  analyzer_impl.cc
  charlattice.cc
  dic_reader.cc
  dictionary_node_creator.cc
  extra_nodes.cc
  innode_features.cc
  lattice_builder.cc
  lattice_config.cc
  lattice_types.cc
  ngram_computations.cc
  normalized_node_creator.cc
  numeric_creator.cc
  onomatopoeia_creator.cc
  output.cc
  perceptron.cc
  rnn_id_resolver.cc
  rnn_scorer.cc
  rnn_scorer_gbeam.cc
  score_processor.cc
  unk_nodes.cc
  unk_nodes_creator.cc
  )

set(core_analysis_tsrc

  analyzer_impl_test.cc
  charlattice_test.cc
  dictionary_node_creator_test.cc
  lattice_builder_test.cc
  lattice_compactor_test.cc
  lattice_types_test.cc
  normalized_node_creator_test.cc
  numeric_creator_test.cc
  onomatopoeia_creator_test.cc
  perceptron_test.cc
  rnn_id_resolver_test.cc
  rnn_scorer_test.cc
  score_processor_test.cc
  unk_nodes_creator_test.cc

  )

set(core_analysis_hdrs

  analysis_input.h
  analysis_result.h
  analyzer.h
  analyzer_impl.h
  charlattice.h
  dic_reader.h
  dictionary_node_creator.h
  extra_nodes.h
  innode_features.h
  lattice_builder.h
  lattice_config.h
  lattice_types.h
  ngram_computations.h
  normalized_node_creator.h
  numeric_creator.h
  onomatopoeia_creator.h
  output.h
  perceptron.h
  rnn_id_resolver.h
  rnn_scorer.h
  rnn_scorer_gbeam.h
  rnn_serialization.h
  score_api.h
  score_plugin.h
  score_processor.h
  unk_maker_types.h
  unk_nodes.h
  unk_nodes_creator.h

  )
install(FILES ${core_analysis_hdrs}
  DESTINATION "${JPP_INSTALL_INCLUDE_DIR}/core/analysis")

jpp_core_files(core_hdrs ${core_analysis_hdrs})


jpp_test_executable(jpp_core_analysis_tests ${core_analysis_tsrc})
target_link_libraries(jpp_core_analysis_tests jpp_core)
