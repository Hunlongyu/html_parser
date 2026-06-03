include_guard(GLOBAL)

function(hps_enable_coverage target_name)
	if(NOT HPS_ENABLE_COVERAGE)
		return()
	endif()

	if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
		target_compile_options(${target_name} PRIVATE -O0 -g --coverage)
		target_link_options(${target_name} PRIVATE --coverage)
		return()
	endif()

	message(WARNING "HPS_ENABLE_COVERAGE is currently supported only with GCC or Clang.")
endfunction()
