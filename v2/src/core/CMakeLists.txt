configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/core_config.h.in
        ${CMAKE_CURRENT_BINARY_DIR}/core_config.h

)

set(core_srcs darts_trie.cc dictionary.cc impl/field_import.cc dic_builder.cc
        analysis/dictionary_node_creator.cc analysis/lattice_types.cc
        analysis/lattice_builder.cc analysis/analysis_input.cc spec/spec_dsl.cc impl/feature_impl_prim.cc
        analysis/unk_nodes_creator.cc analysis/extra_nodes.cc analysis/dic_reader.cc analysis/output.cc
        core.cc analysis/analyzer.cc analysis/analyzer_impl.cc impl/feature_impl_compute.cc
        spec/spec_types.cc spec/spec_serialization.cc impl/model_io.cc analysis/unk_nodes.cc features_api.cc
        analysis/perceptron.cc analysis/score_processor.cc analysis/analysis_result.cc
        env.cc impl/graphviz_format.cc impl/feature_codegen.cc analysis/onomatopoeia_creator.cc
        analysis/rnn_id_resolver.cc analysis/numeric_creator.cc)

set(core_train_srcs
        training/training_io.h training/loss.h training/gold_example.h training/trainer.h training/scw.h
        training/training_io.cc training/loss.cc training/gold_example.cc training/trainer.cc training/scw.cc
        training/training_types.h training/training_test_common.h training/training_executor.cc
        training/training_executor.h training/training_env.cc training/training_env.h
        training/trainer_base.h training/partial_example.h training/partial_example.cc
        training/full_example.h training/full_example.cc)

set(core_hdrs
        ${CMAKE_CURRENT_BINARY_DIR}/core_config.h
        darts.h darts_trie.h dictionary.h impl/field_import.h impl/field_reader.h dic_entries.h
        dic_builder.h spec/spec_grammar.h spec/spec_dsl.h analysis/analysis_input.h analysis/lattice_builder.h
        analysis/dictionary_node_creator.h spec/spec_types.h analysis/lattice_types.h analysis/extra_nodes.h
        impl/feature_types.h impl/feature_impl_prim.h core_types.h analysis/unk_nodes_creator.h
        analysis/dic_reader.h analysis/output.h core.h analysis/analyzer.h analysis/lattice_config.h
        analysis/analyzer_impl.h features_api.h impl/feature_impl_types.h impl/feature_impl_compute.h
        impl/feature_impl_combine.h spec/spec_serialization.h runtime_info.h analysis/unk_maker_types.h
        impl/model_format.h impl/model_io.h impl/model_format_ser.h impl/runtime_ser.h spec/spec_ser.h
        analysis/unk_nodes.h analysis/score_api.h analysis/perceptron.h
        analysis/score_processor.h analysis/analysis_result.h analysis/rnn_scorer.cc analysis/rnn_scorer.h
        impl/perceptron_io.h env.h impl/graphviz_format.h analysis/onomatopoeia_creator.h impl/feature_codegen.h
        analysis/rnn_id_resolver.h analysis/rnn_serialization.h analysis/numeric_creator.h  )

set(core_test_srcs darts_trie_test.cc dictionary_test.cc impl/field_import_test.cc
        impl/field_reader_test.cc impl/feature_impl_prim_test.cc impl/feature_impl_compute_test.cc
        impl/feature_impl_combine_test.cc impl/feature_test.cc runtime_info_test.cc impl/graphviz_format_test.cc
        )

set(core_train_test_srcs training/gold_example_test.cc training/gold_example2_test.cc training/trainer_test.cc
        training/partial_example_io_test.cc)

set(spec_test_srcs spec/spec_grammar_test.cc spec/spec_dsl_test.cc spec/spec_dsl_feature_test.cc
        spec/spec_dsl_kvlist_test.cc)
set(analysis_test_srcs

        test/test_analyzer_env.h

        analysis/dictionary_node_creator_test.cc analysis/lattice_types_test.cc
        analysis/unk_nodes_creator_test.cc analysis/lattice_builder_test.cc
        analysis/perceptron_test.cc analysis/score_processor_test.cc analysis/analyzer_impl_test.cc
        analysis/lattice_compactor_test.cc analysis/onomatopoeia_creator_test.cc
        analysis/rnn_scorer_test.cc analysis/numeric_creator_test.cc)

add_library(jpp_core ${core_srcs} ${core_hdrs} ${libs3p_pegtl_headers})
add_library(jpp_core_train ${core_train_srcs} ${core_hdrs})

target_include_directories(jpp_core PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

jpp_test_executable(jpp_core_tests ${core_test_srcs} ${core_hdrs})
jpp_test_executable(jpp_core_analysis_tests ${analysis_test_srcs} ../testing/test_analyzer.h)
jpp_test_executable(jpp_core_spec_tests ${spec_test_srcs})
jpp_test_executable(jpp_core_train_tests ${core_train_test_srcs})

target_link_libraries(jpp_core jpp_util jpp_rnn)
target_link_libraries(jpp_core_train jpp_core ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(jpp_core_tests jpp_core jpp_core_train)
target_link_libraries(jpp_core_analysis_tests jpp_core)
target_link_libraries(jpp_core_spec_tests jpp_core)
target_link_libraries(jpp_core_train_tests jpp_core_train)

add_subdirectory(impl/codegen)
