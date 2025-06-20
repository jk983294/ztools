file( GLOB EXAMPLE_TOOL_SOURCES "*.cpp" )
foreach( sourcefile ${EXAMPLE_TOOL_SOURCES} )
    # executable name cut off .cpp
    string( REGEX REPLACE ".+/" "demo_" executablename1 ${sourcefile} )
    string( REPLACE ".cpp" "" executablename ${executablename1} )

    message ("source files: " ${sourcefile} " executable: " ${executablename})
    add_executable( ${executablename} ${sourcefile} )
    target_link_libraries( ${executablename} zerg zergio pthread rt dl)
endforeach( sourcefile ${EXAMPLE_TOOL_SOURCES} )
