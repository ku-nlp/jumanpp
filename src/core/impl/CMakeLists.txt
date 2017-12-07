jpp_core_files(core_srcs

  feature_debug.cc
  feature_impl_combine.cc
  feature_impl_compute.cc
  feature_impl_ngram_partial.cc
  feature_impl_pattern.cc
  feature_impl_prim.cc
  global_beam_position_fmt.cc
  graphviz_format.cc
  model_io.cc
  segmented_format.cc

  )

jpp_core_files(core_tsrcs

  aligned_field_test.cc
  feature_impl_combine_test.cc
  feature_impl_compute_test.cc
  feature_impl_ngram_partial_test.cc
  feature_impl_prim_test.cc
  feature_test.cc
  graphviz_format_test.cc
  kvlist_test.cc
  model_io_test.cc

  )

jpp_core_files(core_hdrs

  feature_debug.h
  feature_impl_combine.h
  feature_impl_compute.h
  feature_impl_ngram_partial.h
  feature_impl_ngram_partial_kernels.h
  feature_impl_pattern.h
  feature_impl_prim.h
  feature_impl_types.h
  feature_types.h
  global_beam_position_fmt.h
  graphviz_format.h
  int_seq_util.h
  model_format.h
  model_format_ser.h
  model_io.h
  perceptron_io.h
  segmented_format.h

  )

