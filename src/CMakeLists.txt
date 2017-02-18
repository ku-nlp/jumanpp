add_subdirectory(cdb)
add_subdirectory(rnnlm)

find_package(Boost "1.57" REQUIRED)
include_directories(${Boost_INCLUDE_DIRECTORIES})

add_definitions("-DHAVE_CONFIG_H=1")

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
        ${JPP_HEADERS}
)

add_executable(mkdarts ${JPP_SOURCES} mkdarts.cc)

add_executable(jumanpp ${JPP_SOURCES} morph.cc)

target_link_libraries(jumanpp cdb rnnlm pthread)

target_link_libraries(mkdarts cdb rnnlm pthread)