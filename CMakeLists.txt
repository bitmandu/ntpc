# CMakeLists.txt

idf_component_register(SRCS "ntpc.c"
  INCLUDE_DIRS "include"
  REQUIRES "myfi")
