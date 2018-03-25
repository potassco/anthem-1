set(target anthem-app)

file(GLOB core_sources "*.cpp")
file(GLOB core_headers "*.h")

set(sources
	${core_sources}
	${core_headers}
)

set(includes
	${PROJECT_SOURCE_DIR}/lib/cxxopts/include
)

set(libraries
	anthem
)

add_executable(${target} ${sources})
target_include_directories(${target} PRIVATE ${includes})
target_link_libraries(${target} PRIVATE ${libraries})
set_target_properties(${target} PROPERTIES OUTPUT_NAME anthem)
