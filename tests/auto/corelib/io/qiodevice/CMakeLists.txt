# Generated from qiodevice.pro.

#####################################################################
## tst_qiodevice Test:
#####################################################################

# Collect test data
list(APPEND test_data "tst_qiodevice.cpp")

qt_internal_add_test(tst_qiodevice
    SOURCES
        tst_qiodevice.cpp
    PUBLIC_LIBRARIES
        Qt::Network
    TESTDATA ${test_data}
)

#### Keys ignored in scope 1:.:.:qiodevice.pro:<TRUE>:
# MOC_DIR = "tmp"

## Scopes:
#####################################################################

if(ANDROID)
    # Resources:
    set(android_testdata_resource_files
        "tst_qiodevice.cpp"
    )

    qt_internal_add_resource(tst_qiodevice "android_testdata"
        PREFIX
            "/"
        FILES
            ${android_testdata_resource_files}
    )
endif()
