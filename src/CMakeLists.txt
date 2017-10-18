find_package(Boost "1.57" REQUIRED)

add_subdirectory(cdb)
add_subdirectory(rnnlm)

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


add_library(libjumanv1 ${JPP_SOURCES})

add_executable(mkdarts mkdarts.cc)
add_executable(jumanpp morph.cc)

target_include_directories(libjumanv1 PUBLIC ${Boost_INCLUDE_DIRS})

target_link_libraries(jumanpp cdb rnnlm pthread libjumanv1)

target_link_libraries(mkdarts cdb rnnlm pthread libjumanv1)