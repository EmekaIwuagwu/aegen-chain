add_library(aegen_tests_lib INTERFACE)

add_executable(unit_token_test unit/token_test.cpp)
target_link_libraries(unit_token_test PRIVATE aegen_tokens aegen_core)

add_executable(unit_wallet_test unit/wallet_test.cpp)
target_link_libraries(unit_wallet_test PRIVATE aegen_wallet aegen_core)

add_executable(unit_execution_test unit/execution_test.cpp)
target_link_libraries(unit_execution_test PRIVATE aegen_exec aegen_core aegen_db)

add_executable(unit_block_test unit/block_test.cpp)
target_link_libraries(unit_block_test PRIVATE aegen_consensus aegen_core aegen_db aegen_exec aegen_wallet)

add_executable(unit_consensus_test unit/consensus_test.cpp)
target_link_libraries(unit_consensus_test PRIVATE aegen_consensus aegen_core aegen_db aegen_exec aegen_wallet)

add_executable(unit_vm_test unit/vm_test.cpp)
target_link_libraries(unit_vm_test PRIVATE aegen_exec aegen_core aegen_proofs)
