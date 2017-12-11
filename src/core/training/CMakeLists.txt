set(core_train_src

  full_example.cc
  gold_example.cc
  loss.cc
  partial_example.cc
  partial_trainer.cc
  scw.cc
  trainer.cc
  trainer_base.cc
  training_env.cc
  training_executor.cc
  training_io.cc

  )

set(core_train_tsrc

  gold_example2_test.cc
  gold_example_test.cc
  partial_example_io_test.cc
  partial_example_train_test.cc
  trainer_test.cc

  )

set(core_train_hdrs

  full_example.h
  gold_example.h
  loss.h
  partial_example.h
  partial_trainer.h
  scw.h
  trainer.h
  trainer_base.h
  training_env.h
  training_executor.h
  training_io.h
  training_test_common.h
  training_types.h

  )


add_library(jpp_core_train ${core_train_src} ${core_train_hdrs})
jpp_test_executable(jpp_core_train_tests ${core_train_tsrc})

target_link_libraries(jpp_core_train jpp_core ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(jpp_core_train_tests jpp_core_train)