add_executable(sqlpp.demo)

target_sources(sqlpp.demo
	PRIVATE
		./main.cpp
)

target_link_libraries(sqlpp.demo
	PRIVATE
		sqlpp23
		sqlite3
)

target_compile_features(sqlpp.demo
	PUBLIC
		c_std_23
		cxx_std_26
)

target_compile_options(sqlpp.demo
	PRIVATE
)

target_link_options(sqlpp.demo
	PRIVATE
)

set_target_properties(sqlpp.demo PROPERTIES
	C_EXTENSIONS OFF
	CXX_EXTENSIONS OFF
	LINKER_LANGUAGE CXX
	VERIFY_INTERFACE_HEADER_SETS ON
)
