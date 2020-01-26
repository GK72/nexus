add_executable (main
	Console.cpp
	datatable_test.cpp
)


target_link_libraries (main PRIVATE utility)
target_link_libraries (main PRIVATE io)
target_link_libraries (main PRIVATE arg)
target_link_libraries (main PRIVATE data)
target_include_directories(main PRIVATE ../glib)



add_executable (measure measuring.cpp)
target_link_libraries (measure PRIVATE utility)
target_include_directories(measure PRIVATE ../glib)
