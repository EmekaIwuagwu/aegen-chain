add_library(aegen_proofs
    fraud_proof.cpp
    zk_proof.cpp
)
target_include_directories(aegen_proofs PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(aegen_proofs PUBLIC aegen_core aegen_util)
