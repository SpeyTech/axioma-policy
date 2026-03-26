cmake_minimum_required(VERSION 3.16)

project(axioma-policy
    VERSION 0.1.0
    DESCRIPTION "Axioma L4 Policy Evaluation Layer"
    LANGUAGES C
)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_compile_options(-Wall -Wextra -Wpedantic)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(axioma-policy STATIC
    src/policy.c
)

target_include_directories(axioma-policy PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

enable_testing()

add_executable(test_policy_totality
    tests/identity/test_policy_totality.c
)

target_link_libraries(test_policy_totality PRIVATE
    axioma-policy
)

add_test(
    NAME policy_totality
    COMMAND test_policy_totality
)

message(STATUS "")
message(STATUS "axioma-policy v${PROJECT_VERSION}")
message(STATUS "Layer: L4 — Policy Evaluation")
message(STATUS "Determinism: D2")
message(STATUS "DVEC: v1.3")
message(STATUS "")
