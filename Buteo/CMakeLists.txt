set(BUTEO_SYNC_QML_PLUGIN "buteo-syncfw-qml")

set(BUTEO_SYNC_QML_PLUGIN_SRCS
    buteo-sync-qml.cpp
    buteo-sync-qml.h
    plugin.h
    plugin.cpp
)

set(BUTEO_SYNC_QML_PLUGIN_FILES
    qmldir
)

add_library(${BUTEO_SYNC_QML_PLUGIN} MODULE
    ${BUTEO_SYNC_QML_PLUGIN_SRCS}
)

target_link_libraries(${BUTEO_SYNC_QML_PLUGIN}
    Qt5::Core
    Qt5::Qml
    Qt5::DBus
    Qt5::Xml
)

#copy qml files to build dir to make it possible to run without install
add_custom_target(buteo_components_QmlFiles ALL SOURCES ${BUTEO_SYNC_QML_PLUGIN_FILES})
if(NOT ${CMAKE_CURRENT_BINARY_DIR} STREQUAL ${CMAKE_CURRENT_SOURCE_DIR})
  add_custom_command(TARGET buteo_components_QmlFiles PRE_BUILD
                     COMMAND ${CMAKE_COMMAND} -E
                         copy ${CMAKE_CURRENT_SOURCE_DIR}/qmldir ${CMAKE_CURRENT_BINARY_DIR}/)
endif()

execute_process(
    COMMAND qmake -query QT_INSTALL_QML
    OUTPUT_VARIABLE QT_IMPORTS_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
install(TARGETS ${BUTEO_SYNC_QML_PLUGIN} DESTINATION ${QT_IMPORTS_DIR}/Buteo/)
install(FILES ${BUTEO_SYNC_QML_PLUGIN_FILES} DESTINATION ${QT_IMPORTS_DIR}/Buteo/)
