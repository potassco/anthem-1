set(target tests)

file(GLOB core_sources "*.cpp")

set(includes
	${PROJECT_SOURCE_DIR}/lib/catch/single_include
)

add_executable(${target} ${core_sources})
target_include_directories(${target} PRIVATE ${includes})
target_link_libraries(${target} anthem)

add_custom_target(run-tests
	COMMAND ${CMAKE_BINARY_DIR}/bin/tests --use-colour=yes
	DEPENDS ${target}
	WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)
