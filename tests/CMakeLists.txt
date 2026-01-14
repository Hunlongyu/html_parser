include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://gitee.com/mirrors/googletest.git
  GIT_TAG v1.17.0)
set(gtest_force_shared_crt
    ON
    CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

function(add_config_test name source)
  add_executable(${name} ${source})
  target_link_libraries(${name} PRIVATE $<IF:$<TARGET_EXISTS:hps_static>,hps_static,hps> GTest::gtest_main)
  add_test(NAME ${name} COMMAND ${name})
endfunction()

add_config_test(hps_tests hps_tests.cpp)
