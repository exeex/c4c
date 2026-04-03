include(CMakeParseArguments)

function(c4c_collect_backend_labels out_var)
  set(labels internal backend)
  foreach(label IN LISTS ARGN)
    if(NOT "${label}" STREQUAL "")
      list(APPEND labels "${label}")
    endif()
  endforeach()
  list(JOIN labels ";" labels_value)
  set(${out_var} "${labels_value}" PARENT_SCOPE)
endfunction()

function(c4c_set_backend_test_labels test_name)
  c4c_collect_backend_labels(labels ${ARGN})
  set_tests_properties("${test_name}" PROPERTIES LABELS "${labels}")
endfunction()

function(c4c_add_backend_test test_name)
  add_test(NAME "${test_name}" ${ARGN})
  c4c_set_backend_test_labels("${test_name}")
endfunction()

function(c4c_add_backend_cmake_test test_name runner)
  add_test(
    NAME "${test_name}"
    COMMAND "${CMAKE_COMMAND}"
            ${ARGN}
            -P "${INTERNAL_C_TEST_CMAKE_ROOT}/${runner}"
  )
  c4c_set_backend_test_labels("${test_name}")
endfunction()

function(c4c_add_backend_codegen_route_test test_name)
  set(options)
  set(one_value_args SRC TARGET_TRIPLE OUT_TEXT REQUIRED_SNIPPETS FORBIDDEN_SNIPPETS)
  set(multi_value_args LABELS)
  cmake_parse_arguments(ARG "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

  if(ARG_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR
      "c4c_add_backend_codegen_route_test(${test_name}) got unexpected arguments: ${ARG_UNPARSED_ARGUMENTS}")
  endif()

  foreach(required_arg IN ITEMS SRC TARGET_TRIPLE OUT_TEXT)
    if(NOT DEFINED ARG_${required_arg} OR "${ARG_${required_arg}}" STREQUAL "")
      message(FATAL_ERROR
        "c4c_add_backend_codegen_route_test(${test_name}) missing ${required_arg}")
    endif()
  endforeach()

  add_test(
    NAME "${test_name}"
    COMMAND "${CMAKE_COMMAND}"
            -DCOMPILER=$<TARGET_FILE:c4cll>
            -DSRC=${ARG_SRC}
            -DTARGET_TRIPLE=${ARG_TARGET_TRIPLE}
            -DOUT_TEXT=${ARG_OUT_TEXT}
            "-DREQUIRED_SNIPPETS=${ARG_REQUIRED_SNIPPETS}"
            "-DFORBIDDEN_SNIPPETS=${ARG_FORBIDDEN_SNIPPETS}"
            -P "${INTERNAL_C_TEST_CMAKE_ROOT}/run_backend_codegen_route_case.cmake"
  )
  c4c_set_backend_test_labels("${test_name}" backend_route ${ARG_LABELS})
endfunction()
