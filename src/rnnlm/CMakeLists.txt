file(GLOB RNNLM_SOURCES *.cpp *.h)

add_library(rnnlm ${RNNLM_SOURCES})

target_include_directories(rnnlm PRIVATE ${Boost_INCLUDE_DIRS})