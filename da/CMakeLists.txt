add_library(aegen_da STATIC
    data_availability.cpp
)

target_include_directories(aegen_da PUBLIC ${CMAKE_SOURCE_DIR})
