opents_add_test(SaveIdentityContract
    NAME save-identity-contract
    SOURCES saveidentitycontract.cpp
)
target_compile_definitions(SaveIdentityContract PRIVATE
	"OPENTS_SOURCE_DIR=\"${CMAKE_SOURCE_DIR}\""
)
