file(GLOB RNNLM_SOURCES *.cpp *.h)

add_library(rnnlm ${RNNLM_SOURCES})

link_libraries(rnnln)