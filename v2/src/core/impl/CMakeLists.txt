add_subdirectory(codegen)

set(core_impl_src

        feature_codegen.cc
        feature_impl_combine.cc
        feature_impl_compute.cc
        feature_impl_ngram_partial.cc
        feature_impl_prim.cc
        field_import.cc
        global_beam_position_fmt.cc
        graphviz_format.cc
        model_io.cc

        )

set(core_impl_tsrc

        feature_impl_combine_test.cc
        feature_impl_compute_test.cc
        feature_impl_ngram_partial_test.cc
        feature_impl_prim_test.cc
        feature_test.cc
        field_import_test.cc
        field_reader_test.cc
        graphviz_format_test.cc

        )

set(core_impl_hdrs

        feature_codegen.h
        feature_impl_combine.h
        feature_impl_compute.h
        feature_impl_ngram_partial.h
        feature_impl_prim.h
        feature_impl_types.h
        feature_types.h
        field_import.h
        field_reader.h
        global_beam_position_fmt.h
        graphviz_format.h
        model_format.h
        model_format_ser.h
        model_io.h
        perceptron_io.h
        runtime_ser.h

        )


jpp_add_core_objlib(jpp_core_impl core_impl_src)
jpp_add_core_objlib(jpp_core_impl_tobj core_impl_tsrc)
