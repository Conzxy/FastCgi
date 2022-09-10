file(GLOB_RECURSE FCGI_SRC
  *.cc
)

GenLib(${FCGI_LIB} ${FCGI_SRC})
