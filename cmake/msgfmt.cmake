find_package(Gettext REQUIRED)
if(Gettext_FOUND)
    set(LOCALES_OUTPUT_DIR ${CMAKE_BINARY_DIR}/locales)
    set(LANGUAGES zh_CN)
    set(DOMAINS ui)
    set(MO_FILES "")

    foreach(LANG ${LANGUAGES})
        set(MO_DIR ${LOCALES_OUTPUT_DIR}/${LANG}/LC_MESSAGES)
        file(MAKE_DIRECTORY ${MO_DIR})
        foreach(DOM ${DOMAINS})
            set(PO_FILE ${CMAKE_CURRENT_SOURCE_DIR}/locales/${LANG}/${DOM}.po)
            set(MO_FILE ${MO_DIR}/${DOM}.mo)
            if(EXISTS ${PO_FILE})
                add_custom_command(
                    OUTPUT ${MO_FILE}
                    COMMAND ${GETTEXT_MSGFMT_EXECUTABLE} -o ${MO_FILE} ${PO_FILE}
                    DEPENDS ${PO_FILE}
                    COMMENT "Compiling ${PO_FILE} -> ${MO_FILE}"
                )
                list(APPEND MO_FILES ${MO_FILE})
            endif()
        endforeach()
    endforeach()

    add_custom_target(translations ALL DEPENDS ${MO_FILES})
    if(TARGET ${PROJECT_NAME})
        add_dependencies(${PROJECT_NAME} translations)
    endif()
endif()