opents_add_test(ModalInputContract
    NAME modal-input-contract
    SOURCES modalinputcontract.cpp
    INCLUDES "${CMAKE_SOURCE_DIR}/thirdparty/RmlUi/Include"
    LIBRARIES rmlui
)
