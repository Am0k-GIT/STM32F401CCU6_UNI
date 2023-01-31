add_library(spindle INTERFACE)

target_sources(spindle INTERFACE
 ${CMAKE_CURRENT_LIST_DIR}/modbus.c
 ${CMAKE_CURRENT_LIST_DIR}/select.c
 ${CMAKE_CURRENT_LIST_DIR}/vfd/spindle.c
 ${CMAKE_CURRENT_LIST_DIR}/vfd/huanyang.c
 ${CMAKE_CURRENT_LIST_DIR}/vfd/h100.c
 ${CMAKE_CURRENT_LIST_DIR}/vfd/modvfd.c
 ${CMAKE_CURRENT_LIST_DIR}/vfd/gs20.c
 ${CMAKE_CURRENT_LIST_DIR}/vfd/yl620.c
)

target_include_directories(spindle INTERFACE ${CMAKE_CURRENT_LIST_DIR})
