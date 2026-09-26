file(TO_CMAKE_PATH "${CMAKE_SOURCE_DIR}" OPENTS_SOURCE_DIR_CMAKE)

opents_add_test(IOSProductIdentityContract
    NAME ios-product-identity-contract
    SOURCES iosproductcontract.cpp
    DEFINITIONS OPENTS_SOURCE_DIR="${OPENTS_SOURCE_DIR_CMAKE}"
)
