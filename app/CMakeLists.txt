set(target anthem-app)

find_package(Boost 1.55.0 COMPONENTS program_options system filesystem REQUIRED)

file(GLOB core_sources "*.cpp")
file(GLOB core_headers "*.h")

set(sources
	${core_sources}
	${core_headers}
)

set(includes
	${Boost_INCLUDE_DIRS}
)

set(libraries
	${Boost_LIBRARIES}
	anthem
)

add_executable(${target} ${sources})
target_include_directories(${target} PRIVATE ${includes})
target_link_libraries(${target} PRIVATE ${libraries})
set_target_properties(${target} PROPERTIES OUTPUT_NAME anthem)
