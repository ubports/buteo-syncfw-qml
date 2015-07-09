find_program(QMLTESTRUNNER_BIN
    NAMES qmltestrunner
    PATHS /usr/lib/*/qt5/bin
    NO_DEFAULT_PATH
)

find_program(DBUS_RUNNER_BIN dbus-test-runner)
macro(DECLARE_QML_TEST TST_NAME TST_QML_FILE)
    set(TEST_COMMAND "")
    add_test(NAME ${TST_NAME}
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMAND ${DBUS_RUNNER_BIN}
                --task ${CMAKE_CURRENT_SOURCE_DIR}/buteo-syncfw.py -r -n buteo-syncfw
                --task ${QMLTESTRUNNER_BIN}
                    -p -input -p ${CMAKE_CURRENT_SOURCE_DIR}/${TST_QML_FILE}
                    -p -import -p ${CMAKE_BINARY_DIR}
                    -p -platform -p offscreen
                --wait-for=com.meego.msyncd
                -n ${TST_NAME}
    )
endmacro()

if(QMLTESTRUNNER_BIN AND DBUS_RUNNER_BIN)
    declare_qml_test("buteo_syncfw_component" tst_ButeoSyncFW.qml)
else()
    if (NOT QMLTESTRUNNER_BIN)
        message(WARNING "Qml tests disabled: qmltestrunner not found")
    else()
        message(WARNING "Qml tests disabled: dbus-test-runner not found")
    endif()
endif()

set(QML_TST_FILES
    tst_ButeoSyncFW.qml
)
add_custom_target(tst_QmlFiles ALL SOURCES ${QML_TST_FILES})
