set(target anthem)

file(GLOB core_sources "anthem/*.cpp")
file(GLOB core_headers "../include/anthem/*.h")

file(GLOB input_sources "anthem/input/*.cpp")
file(GLOB input_headers "../include/anthem/input/*.h")

file(GLOB output_sources "anthem/output/*.cpp")
file(GLOB output_headers "../include/anthem/output/*.h")

set(sources
	${core_sources}
	${core_headers}

	${input_sources}
	${input_headers}

	${output_sources}
	${output_headers}
)

set(includes
	${PROJECT_SOURCE_DIR}/include
	${PROJECT_SOURCE_DIR}/lib/variant/include
)

set(libraries
	libclasp
	libclingo
	libgringo
)

if(ANTHEM_BUILD_STATIC)
	add_library(${target} STATIC ${sources})
else()
	add_library(${target} ${sources})
endif()

target_include_directories(${target} PUBLIC ${includes})
target_link_libraries(${target} ${libraries})
