set(BUILD_ALL_EXAMPLES OFF CACHE BOOL "Determine if build all examples")
set(EXAMPLE_SOURCE_DIR ${PROJECT_SOURCE_DIR}/example)

function (GenExample exec_name)
  if (${BUILD_ALL_EXAMPLES})   
    add_executable(${exec_name} ${ARGN})
  else ()
    add_executable(${exec_name} EXCLUDE_FROM_ALL ${ARGN})
  endif (${BUILD_ALL_EXAMPLES})   
  
  target_link_libraries(${exec_name} kanon_net kanon_base ${FCGI_LIB})
  set_target_properties(${exec_name}
    PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/example
  )
endfunction ()

GenExample(echo_cgi echo_cgi.cc)
