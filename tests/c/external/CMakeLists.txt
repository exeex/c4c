# ── c-testsuite ───────────────────────────────────────────────────────────────

set(C_TESTSUITE_ALLOWLIST "${C_TESTSUITE_ROOT}/allowlist.txt")
set(C_TESTSUITE_RUN_CASE "${CMAKE_CURRENT_SOURCE_DIR}/c-testsuite/RunCase.cmake")

if(CLANG_EXECUTABLE AND C_TESTSUITE_ROOT AND EXISTS "${C_TESTSUITE_ROOT}")
  if(NOT EXISTS "${C_TESTSUITE_ALLOWLIST}" OR NOT EXISTS "${C_TESTSUITE_RUN_CASE}")
    message(STATUS
      "Skipping c-testsuite external tests: missing submodule contents under "
      "${C_TESTSUITE_ROOT} (expected allowlist.txt and RunCase.cmake)")
  else()
    string(TOLOWER "${CMAKE_HOST_SYSTEM_PROCESSOR}" C_TESTSUITE_HOST_CPU)
    if(C_TESTSUITE_HOST_CPU STREQUAL "x86_64" OR C_TESTSUITE_HOST_CPU STREQUAL "amd64")
      set(C_TESTSUITE_BACKEND_MODE "backend-x86_64")
      set(C_TESTSUITE_BACKEND_TRIPLE "x86_64-unknown-linux-gnu")
      set(C_TESTSUITE_BACKEND_ID "x86")
      set(C_TESTSUITE_BACKEND_LABEL "x86_backend")
      set(C_TESTSUITE_BACKEND_DIR "c_testsuite_x86_backend")
    elseif(C_TESTSUITE_HOST_CPU STREQUAL "aarch64" OR C_TESTSUITE_HOST_CPU STREQUAL "arm64")
      set(C_TESTSUITE_BACKEND_MODE "backend-aarch64")
      set(C_TESTSUITE_BACKEND_TRIPLE "aarch64-unknown-linux-gnu")
      set(C_TESTSUITE_BACKEND_ID "aarch64")
      set(C_TESTSUITE_BACKEND_LABEL "aarch64_backend")
      set(C_TESTSUITE_BACKEND_DIR "c_testsuite_aarch64_backend")
    else()
      set(C_TESTSUITE_BACKEND_MODE "")
      set(C_TESTSUITE_BACKEND_TRIPLE "")
      set(C_TESTSUITE_BACKEND_ID "")
      set(C_TESTSUITE_BACKEND_LABEL "")
      set(C_TESTSUITE_BACKEND_DIR "")
    endif()

    file(STRINGS "${C_TESTSUITE_ALLOWLIST}" C_TESTSUITE_ALLOWLIST_RAW)
    foreach(entry_raw IN LISTS C_TESTSUITE_ALLOWLIST_RAW)
      string(STRIP "${entry_raw}" entry)
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
      set(backend_asm_source "stdout")

      set(src "${C_TESTSUITE_ROOT}/${rel_src}")
      if(NOT EXISTS "${src}")
        message(WARNING "c-testsuite allowlist entry not found: ${rel_src}")
        continue()
      endif()

      string(REGEX REPLACE "[^A-Za-z0-9_]" "_" test_id "${rel_src}")
      set(test_name "c_testsuite_${test_id}")

      add_test(
        NAME "${test_name}"
        COMMAND "${CMAKE_COMMAND}"
                -DCOMPILER=$<TARGET_FILE:c4cll>
                -DCLANG=${CLANG_EXECUTABLE}
                -DSRC=${src}
                -DROOT=${C_TESTSUITE_ROOT}
                -DOUT_LL=${CMAKE_BINARY_DIR}/c_testsuite/${rel_src}.ll
                -DOUT_BIN=${CMAKE_BINARY_DIR}/c_testsuite/${rel_src}.bin
                -P "${C_TESTSUITE_RUN_CASE}"
      )
      set_tests_properties("${test_name}" PROPERTIES LABELS "c_testsuite")

      if(C_TESTSUITE_BACKEND_MODE AND C_TESTSUITE_BACKEND_LABEL AND C_TESTSUITE_BACKEND_TRIPLE)
        set(backend_test_name "c_testsuite_${C_TESTSUITE_BACKEND_ID}_backend_${test_id}")
        add_test(
          NAME "${backend_test_name}"
          COMMAND "${CMAKE_COMMAND}"
                  -DCODEGEN_MODE=${C_TESTSUITE_BACKEND_MODE}
                  -DCOMPILER=$<TARGET_FILE:c4cll>
                  -DCLANG=${CLANG_EXECUTABLE}
                  -DSRC=${src}
                  -DROOT=${C_TESTSUITE_ROOT}
                  -DTARGET_TRIPLE=${C_TESTSUITE_BACKEND_TRIPLE}
                  -DBACKEND_ASM_SOURCE=${backend_asm_source}
                  -DOUT_LL=${CMAKE_BINARY_DIR}/${C_TESTSUITE_BACKEND_DIR}/${rel_src}.s
                  -DOUT_BIN=${CMAKE_BINARY_DIR}/${C_TESTSUITE_BACKEND_DIR}/${rel_src}.bin
                  -P "${C_TESTSUITE_RUN_CASE}"
        )
        set_tests_properties("${backend_test_name}" PROPERTIES LABELS "c_testsuite;${C_TESTSUITE_BACKEND_LABEL}")
      endif()
    endforeach()
  endif()
endif()

# ── clang C external ──────────────────────────────────────────────────────────

if(ENABLE_C_CLANG_EXTERNAL_TESTS AND
   C_CLANG_TEST_SUITE_ROOT AND EXISTS "${C_CLANG_TEST_SUITE_ROOT}")
  set(C_CLANG_TEST_ALLOWLIST "${C_CLANG_TEST_SUITE_ROOT}/allowlist.txt")
  if(EXISTS "${C_CLANG_TEST_ALLOWLIST}")
    file(STRINGS "${C_CLANG_TEST_ALLOWLIST}" C_CLANG_TEST_ALLOWLIST_RAW)
    foreach(entry IN LISTS C_CLANG_TEST_ALLOWLIST_RAW)
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

      set(src "${C_CLANG_TEST_SUITE_ROOT}/${rel_src}")
      if(NOT EXISTS "${src}")
        message(WARNING "clang c allowlist entry not found: ${rel_src}")
        continue()
      endif()

      string(REGEX REPLACE "[^A-Za-z0-9_]" "_" test_id "${rel_src}")
      set(test_name "clang_c_external_${test_id}")

      add_test(
        NAME "${test_name}"
        COMMAND "${CMAKE_COMMAND}"
                -DCOMPILER=$<TARGET_FILE:c4cll>
                -DPYTHON3_EXECUTABLE=${PYTHON3_EXECUTABLE}
                -DRUNNER=${PROJECT_SOURCE_DIR}/tests/verify_external_clang_case.py
                -DSRC=${src}
                -DEXPECT_MODE=${expect_mode}
                -DOUT_LL=${CMAKE_BINARY_DIR}/clang_c_external/${rel_src}.ll
                -P "${C_CLANG_TEST_SUITE_ROOT}/RunCase.cmake"
      )
      set_tests_properties("${test_name}" PROPERTIES LABELS "clang_c_external;c")
    endforeach()
  endif()
endif()

# ── llvm gcc-c-torture ────────────────────────────────────────────────────────

if(ENABLE_LLVM_GCC_C_TORTURE_TESTS AND
   CLANG_EXECUTABLE AND LLVM_TEST_SUITE_ROOT AND EXISTS "${LLVM_TEST_SUITE_ROOT}")
  set(LLVM_GCC_C_TORTURE_ROOT "${LLVM_TEST_SUITE_ROOT}")
  set(LLVM_GCC_C_TORTURE_ALLOWLIST "${LLVM_TEST_SUITE_ROOT}/allowlist.txt")

  if(EXISTS "${LLVM_GCC_C_TORTURE_ROOT}" AND
     EXISTS "${LLVM_GCC_C_TORTURE_ALLOWLIST}")
    file(STRINGS "${LLVM_GCC_C_TORTURE_ALLOWLIST}"
         LLVM_GCC_C_TORTURE_ALLOWLIST_RAW)
    set(LLVM_GCC_C_TORTURE_TEST_MAP
        "${CMAKE_BINARY_DIR}/llvm_gcc_c_torture_test_map.tsv")
    file(WRITE "${LLVM_GCC_C_TORTURE_TEST_MAP}"
         "# test_name\tallowlist_entry\n")
    foreach(entry IN LISTS LLVM_GCC_C_TORTURE_ALLOWLIST_RAW)
      string(STRIP "${entry}" entry)
      if(entry STREQUAL "" OR entry MATCHES "^#")
        continue()
      endif()

      set(src "${LLVM_GCC_C_TORTURE_ROOT}/${entry}")
      if(NOT EXISTS "${src}")
        message(WARNING "llvm gcc-c-torture allowlist entry not found: ${entry}")
        continue()
      endif()

      string(REGEX REPLACE "[^A-Za-z0-9_]" "_" test_id "${entry}")
      set(test_name "llvm_gcc_c_torture_${test_id}")

      add_test(
        NAME "${test_name}"
        COMMAND "${CMAKE_COMMAND}"
                -DCOMPILER=$<TARGET_FILE:c4cll>
                -DCLANG=${CLANG_EXECUTABLE}
                -DSRC=${src}
                -DROOT=${LLVM_GCC_C_TORTURE_ROOT}
                -DOUT_LL=${CMAKE_BINARY_DIR}/llvm_gcc_c_torture/${entry}.ll
                -DOUT_CLANG_BIN=${CMAKE_BINARY_DIR}/llvm_gcc_c_torture/${entry}.clang.bin
                -DOUT_C2LL_BIN=${CMAKE_BINARY_DIR}/llvm_gcc_c_torture/${entry}.c2ll.bin
                -DCASE_TIMEOUT_SEC=${LLVM_GCC_C_TORTURE_STEP_TIMEOUT_SEC}
                -DRUN_MEM_MB=${LLVM_GCC_C_TORTURE_RUN_MEM_MB}
                -DRUN_CPU_SEC=${LLVM_GCC_C_TORTURE_RUN_CPU_SEC}
                -P "${CMAKE_CURRENT_SOURCE_DIR}/gcc_torture/RunCase.cmake"
      )
      set_tests_properties("${test_name}" PROPERTIES
        LABELS "llvm_gcc_c_torture"
        TIMEOUT "${LLVM_GCC_C_TORTURE_TEST_TIMEOUT_SEC}")
      file(APPEND "${LLVM_GCC_C_TORTURE_TEST_MAP}" "${test_name}\t${entry}\n")
    endforeach()

    add_custom_target(prune_llvm_gcc_c_torture_allowlist_to_failed
      COMMAND "${CMAKE_COMMAND}"
              -DTEST_MAP=${LLVM_GCC_C_TORTURE_TEST_MAP}
              -DLAST_FAILED=${CMAKE_BINARY_DIR}/Testing/Temporary/LastTestsFailed.log
              -DALLOWLIST=${LLVM_GCC_C_TORTURE_ALLOWLIST}
              -P "${CMAKE_CURRENT_SOURCE_DIR}/gcc_torture/PruneAllowlistToFailed.cmake"
      COMMENT "Rewrite tests/c/external/gcc_torture/allowlist.txt to keep only failed cases from last ctest run"
      VERBATIM
    )
  endif()
endif()
