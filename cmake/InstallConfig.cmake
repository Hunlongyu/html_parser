include_guard(GLOBAL)

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

function(hps_install_project target_name)
	set(hps_package_directory "${CMAKE_INSTALL_LIBDIR}/cmake/hps")

	configure_package_config_file(
		"${PROJECT_SOURCE_DIR}/cmake/hpsConfig.cmake.in"
		"${PROJECT_BINARY_DIR}/hpsConfig.cmake"
		INSTALL_DESTINATION "${hps_package_directory}"
	)

	write_basic_package_version_file(
		"${PROJECT_BINARY_DIR}/hpsConfigVersion.cmake"
		VERSION "${PROJECT_VERSION}"
		COMPATIBILITY SameMajorVersion
	)

	install(
		TARGETS ${target_name}
		EXPORT hpsTargets
		RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
		LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
		ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
		FILE_SET public_headers DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
		FILE_SET generated_headers DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
	)

	install(
		EXPORT hpsTargets
		FILE hpsTargets.cmake
		NAMESPACE hps::
		DESTINATION "${hps_package_directory}"
	)

	install(
		FILES
			"${PROJECT_BINARY_DIR}/hpsConfig.cmake"
			"${PROJECT_BINARY_DIR}/hpsConfigVersion.cmake"
		DESTINATION "${hps_package_directory}"
	)
endfunction()
