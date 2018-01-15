set(VERSION "3.3.3")

include(ExternalProject)
ExternalProject_Add(
    eigen
    URL "http://bitbucket.org/eigen/eigen/get/${VERSION}.tar.bz2"
    URL_HASH SHA256=a4143fc45e4454b4b98fcea3516b3a79b8cdb3bc7fadf996d088c6a0d805fea1
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
)

ExternalProject_Get_Property(eigen install_dir)

add_library (Eigen3 INTERFACE)
add_dependencies(Eigen3 eigen)
target_sources(Eigen3 INTERFACE "${install_dir}/src/eigen")
target_include_directories(Eigen3 INTERFACE "${install_dir}/src/eigen")
