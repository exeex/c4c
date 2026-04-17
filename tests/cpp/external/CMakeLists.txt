# ── clang C++ external ────────────────────────────────────────────────────────

if(ENABLE_CPP_CLANG_EXTERNAL_TESTS AND
   CPP_CLANG_TEST_SUITE_ROOT AND EXISTS "${CPP_CLANG_TEST_SUITE_ROOT}")
  set(CPP_CLANG_TEST_ALLOWLIST "${CPP_CLANG_TEST_SUITE_ROOT}/allowlist.txt")
  if(EXISTS "${CPP_CLANG_TEST_ALLOWLIST}")
    file(STRINGS "${CPP_CLANG_TEST_ALLOWLIST}" CPP_CLANG_TEST_ALLOWLIST_RAW)
    foreach(entry IN LISTS CPP_CLANG_TEST_ALLOWLIST_RAW)
      string(STRIP "${entry}" entry)
      if(entry STREQUAL "" OR entry MATCHES "^#")
        continue()
      endif()

      set(fields "${entry}")
      string(REPLACE "|" ";" fields "${fields}")
      list(LENGTH fields field_count)
      if(field_count LESS 1)
        continue()
      endif()

      list(GET fields 0 rel_src)
      set(expect_mode "pass")
      if(field_count GREATER 1)
        list(GET fields 1 expect_mode)
      endif()

      set(src "${CPP_CLANG_TEST_SUITE_ROOT}/${rel_src}")
      if(NOT EXISTS "${src}")
        message(WARNING "clang cpp allowlist entry not found: ${rel_src}")
        continue()
      endif()

      string(REGEX REPLACE "[^A-Za-z0-9_]" "_" test_id "${rel_src}")
      set(test_name "clang_cpp_external_${test_id}")

      add_test(
        NAME "${test_name}"
        COMMAND "${CMAKE_COMMAND}"
                -DCOMPILER=$<TARGET_FILE:c4cll>
                -DPYTHON3_EXECUTABLE=${PYTHON3_EXECUTABLE}
                -DRUNNER=${PROJECT_SOURCE_DIR}/tests/verify_external_clang_case.py
                -DSRC=${src}
                -DEXPECT_MODE=${expect_mode}
                -DOUT_LL=${CMAKE_BINARY_DIR}/clang_cpp_external/${rel_src}.ll
                -P "${CPP_CLANG_TEST_SUITE_ROOT}/RunCase.cmake"
      )
      set_tests_properties("${test_name}" PROPERTIES LABELS "clang_cpp_external;cpp")
    endforeach()
  endif()
endif()

# ── EASTL C++ external ────────────────────────────────────────────────────────

if(ENABLE_CPP_EASTL_EXTERNAL_TESTS AND
   CPP_EASTL_EXTERNAL_TEST_SUITE_ROOT AND EXISTS "${CPP_EASTL_EXTERNAL_TEST_SUITE_ROOT}")
  set(CPP_EASTL_EXTERNAL_ALLOWLIST "${CPP_EASTL_EXTERNAL_TEST_SUITE_ROOT}/allowlist.txt")
  if(EXISTS "${CPP_EASTL_EXTERNAL_ALLOWLIST}")
    file(STRINGS "${CPP_EASTL_EXTERNAL_ALLOWLIST}" CPP_EASTL_EXTERNAL_ALLOWLIST_RAW)
    foreach(entry IN LISTS CPP_EASTL_EXTERNAL_ALLOWLIST_RAW)
      string(STRIP "${entry}" entry)
      if(entry STREQUAL "" OR entry MATCHES "^#")
        continue()
      endif()

      set(fields "${entry}")
      string(REPLACE "|" ";" fields "${fields}")
      list(LENGTH fields field_count)
      if(field_count LESS 1)
        continue()
      endif()

      list(GET fields 0 rel_src)
      set(test_mode "runtime")
      if(field_count GREATER 1)
        list(GET fields 1 test_mode)
      endif()

      set(src "${CPP_EASTL_EXTERNAL_TEST_SUITE_ROOT}/${rel_src}")
      if(NOT EXISTS "${src}")
        message(WARNING "eastl cpp allowlist entry not found: ${rel_src}")
        continue()
      endif()

      string(REGEX REPLACE "[^A-Za-z0-9_]" "_" test_id "${rel_src}")
      set(test_name "eastl_cpp_external_${test_id}")

      add_test(
        NAME "${test_name}"
        COMMAND "${CMAKE_COMMAND}"
                -DCOMPILER=$<TARGET_FILE:c4cll>
                -DCLANG=${CLANG_EXECUTABLE}
                -DCXX_COMPILER=${CMAKE_CXX_COMPILER}
                -DROOT=${PROJECT_SOURCE_DIR}
                -DSRC=${src}
                -DTEST_MODE=${test_mode}
                -DOUT_LL=${CMAKE_BINARY_DIR}/eastl_cpp_external/${rel_src}.ll
                -DOUT_CLANG_BIN=${CMAKE_BINARY_DIR}/eastl_cpp_external/${rel_src}.host.bin
                -DOUT_C4C_BIN=${CMAKE_BINARY_DIR}/eastl_cpp_external/${rel_src}.c4c.bin
                -P "${CPP_EASTL_EXTERNAL_TEST_SUITE_ROOT}/RunCase.cmake"
      )
      set_tests_properties("${test_name}" PROPERTIES LABELS "eastl_cpp_external;cpp")
    endforeach()
  endif()
endif()
