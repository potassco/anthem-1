set(target anthem)

file(GLOB core_sources "anthem/*.cpp")
file(GLOB core_headers "../include/anthem/*.h")

file(GLOB output_sources "anthem/output/*.cpp")
file(GLOB output_headers "../include/anthem/output/*.h")

set(sources
	${core_sources}
	${core_headers}

	${output_sources}
	${output_headers}
)

set(includes
    ${PROJECT_SOURCE_DIR}/include
	${Boost_INCLUDE_DIRS}
)

set(libraries
	${Boost_LIBRARIES}
	libclasp
	libclingo
	libgringo
	pthread
)

add_library(${target} ${sources})
target_include_directories(${target} PUBLIC ${includes})
target_link_libraries(${target} ${libraries})
