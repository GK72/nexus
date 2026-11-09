add_executable(@name@
    main.cpp
)

# <@name@/gen/build_info.hpp>: the version, the commit and the branch.
nova_target_build_info(@name@ NAMESPACE @namespace@::build)

target_link_libraries(@name@ PRIVATE
    @name@-core
    Boost::program_options
)

install(TARGETS @name@ RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
