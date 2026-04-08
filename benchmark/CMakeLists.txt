function(hps_add_benchmark target_name source_file)
	add_executable(${target_name} ${source_file})
	target_link_libraries(${target_name} PRIVATE hps_static)
	target_compile_definitions(${target_name} PRIVATE HPS_SOURCE_DIR="${PROJECT_SOURCE_DIR}")
endfunction()

hps_add_benchmark(tokenizer_bench tokenizer_bench.cpp)
hps_add_benchmark(css_selector_bench css_selector_bench.cpp)
hps_add_benchmark(parser_bench parser_bench.cpp)
