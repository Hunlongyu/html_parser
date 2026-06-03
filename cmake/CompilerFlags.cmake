include_guard(GLOBAL)

include(CodeCoverage)

function(hps_set_common_warnings target_name)
	if(MSVC)
		target_compile_options(${target_name} PRIVATE /utf-8 /W4 /permissive- /Zc:__cplusplus)
	else()
		target_compile_options(${target_name} PRIVATE -Wall -Wextra -Wpedantic)
	endif()
endfunction()

function(hps_enable_clang_tidy target_name)
	if(NOT HPS_ENABLE_CLANG_TIDY)
		return()
	endif()

	if(NOT HPS_CLANG_TIDY_EXE)
		find_program(HPS_CLANG_TIDY_EXE NAMES clang-tidy REQUIRED)
	endif()

	set_target_properties(${target_name} PROPERTIES CXX_CLANG_TIDY "${HPS_CLANG_TIDY_EXE}")
endfunction()

function(hps_apply_target_defaults target_name)
	target_compile_features(${target_name} PUBLIC cxx_std_20)
	hps_set_common_warnings(${target_name})
	hps_enable_coverage(${target_name})
	hps_enable_clang_tidy(${target_name})
endfunction()
