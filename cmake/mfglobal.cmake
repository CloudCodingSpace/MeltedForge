function(EnableFlags target)
    if(MSVC)
        target_compile_options(${target} PRIVATE
            $<$<CONFIG:Debug>:/Zi>
            $<$<CONFIG:Release>:/O2>
        )
    elseif(CMAKE_COMPILER_IS_GNUCXX)
        target_compile_options(${target} PRIVATE
            $<$<CONFIG:Debug>:-g>
            $<$<CONFIG:Release>:-O3>
        )
    endif()

    if(WIN32)
        set(MF_PLATFORM "MF_PLATFORM_WINDOWS")
    elseif(APPLE)
        set(MF_PLATFORM "MF_PLATFORM_MAC")
    elseif(UNIX AND NOT APPLE)
        set(MF_PLATFORM "MF_PLATFORM_LINUX")
    endif()

    target_compile_definitions(${target} PUBLIC
        $<$<CONFIG:Debug>:_DEBUG;MF_DEBUG>
        $<$<CONFIG:Release>:NDEBUG;MF_NDEBUG>
        ${MF_PLATFORM}
    )
endfunction()
