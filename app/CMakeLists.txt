set(target anthem-app)

file(GLOB core_sources "*.cpp")
file(GLOB core_headers "*.h")

set(sources
	${core_sources}
	${core_headers}
)

add_executable(${target} ${sources})
target_link_libraries(${target} anthem)
set_target_properties(${target} PROPERTIES OUTPUT_NAME anthem)
