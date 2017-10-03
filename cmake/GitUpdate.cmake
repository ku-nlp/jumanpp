add_custom_target(
    git-pull
    COMMAND git pull --rebase
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)