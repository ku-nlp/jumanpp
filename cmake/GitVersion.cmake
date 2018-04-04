if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/.git)
    execute_process(
            COMMAND git log --oneline --date=format:%Y%m%d --format=%ad-%h --max-count=1 HEAD
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            RESULT_VARIABLE GIT_VERSION_OK
            OUTPUT_VARIABLE GIT_VERSION_STRING
            ERROR_VARIABLE GIT_OUTPUT_ERROR
    )
    if (GIT_VERSION_OK EQUAL 0)
        string(STRIP ${GIT_VERSION_STRING} GIT_VERSION_STRING)
        set(GIT_VERSION_STRING ".${GIT_VERSION_STRING}")
    else()
        message("Failed to get a version string from git:\n ${GIT_OUTPUT_ERROR} ${GIT_VERSION_STRING}")
        set(GIT_VERSION_STRING "-git")
    endif()
else()
    set(GIT_VERSION_STRING "")
endif()