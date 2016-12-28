add_subdirectory(cdb)
add_subdirectory(rnnlm)

file(GLOB JPP_HEADERS *.h)
set(JPP_SOURCES
        cdb_juman.cc
        charlattice.cc
        dic.cc
        feature.cc
        feature_vector.cc
        node.cc
        pos.cc
        scw.cc
        sentence.cc
        tagger.cc
        tools.cc
        u8string.cc
        wvector.cc
)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DHAVE_CONFIG_H=1")

add_executable(mkdarts ${JPP_SOURCES} mkdarts.cc)

add_executable(jumanpp ${JPP_SOURCES} morph.cc)

target_link_libraries(jumanpp cdb rnnlm)

target_link_libraries(mkdarts cdb rnnlm)