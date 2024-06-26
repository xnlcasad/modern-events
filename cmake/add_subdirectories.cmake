MACRO(ADD_ALL_SUBDIRECTORIES directory)

  file(GLOB children RELATIVE ${directory} ${directory}/*)
  set(dirlist "")
  foreach(child ${children})
    if(IS_DIRECTORY ${directory}/${child})
        list(APPEND dirlist ${child})
    endif()
  endforeach()

  foreach(SUBDIR ${dirlist})
    if(EXISTS "${directory}/${SUBDIR}/CMakeLists.txt")
      message(STATUS "Adding ${directory}/${SUBDIR}")
      add_subdirectory(${SUBDIR})
    else()
      message(WARNING "Skipping ${directory}/${SUBDIR} because no CMakeLists.txt file was found!")
    endif()
  endforeach()

ENDMACRO()
