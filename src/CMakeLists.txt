set(target anthem)

file(GLOB core_sources "anthem/*.cpp")
file(GLOB core_headers "../include/anthem/*.h")

file(GLOB examine_semantics_sources "anthem/examine-semantics/*.cpp")
file(GLOB examine_semantics_headers "../include/anthem/examine-semantics/*.h")

file(GLOB output_sources "anthem/output/*.cpp")
file(GLOB output_headers "../include/anthem/output/*.h")

file(GLOB translation_common_sources "anthem/translation-common/*.cpp")
file(GLOB translation_common_headers "../include/anthem/translation-common/*.h")

file(GLOB verify_properties_sources "anthem/verify-properties/*.cpp")
file(GLOB verify_properties_headers "../include/anthem/verify-properties/*.h")

file(GLOB verify_strong_equivalence_sources "anthem/verify-strong-equivalence/*.cpp")
file(GLOB verify_strong_equivalence_headers "../include/anthem/verify-strong-equivalence/*.h")

set(sources
	${core_sources}
	${core_headers}

	${examine_semantics_sources}
	${examine_semantics_headers}

	${output_sources}
	${output_headers}

	${translation_common_sources}
	${translation_common_headers}

	${verify_properties_sources}
	${verify_properties_headers}

	${verify_strong_equivalence_sources}
	${verify_strong_equivalence_headers}
)

set(includes
	${PROJECT_SOURCE_DIR}/include
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
