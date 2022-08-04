add_executable(wip
    driver.cpp
    Visitor.cpp
)
target_link_libraries(wip PRIVATE slangcompiler)
if(STATIC_BUILD)
  target_link_libraries(wip PRIVATE -static -Wl,--whole-archive -lpthread
                                       -Wl,--no-whole-archive)
endif()
set_target_properties(wip PROPERTIES OUTPUT_NAME "wip_slang")
