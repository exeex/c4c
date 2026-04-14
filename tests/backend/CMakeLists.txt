set(BACKEND_TEST_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")
set(BACKEND_TEST_CMAKE_ROOT "${BACKEND_TEST_ROOT}/cmake")
set(BACKEND_CASE_ROOT "${BACKEND_TEST_ROOT}/case")

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

function(c4c_add_backend_codegen_route_test test_name)
  set(options)
  set(one_value_args SRC TARGET_TRIPLE OUT_TEXT REQUIRED_SNIPPETS FORBIDDEN_SNIPPETS
                     EXTRA_COMPILER_ARGS)
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
            "-DEXTRA_COMPILER_ARGS=${ARG_EXTRA_COMPILER_ARGS}"
            -P "${BACKEND_TEST_CMAKE_ROOT}/run_backend_codegen_route_case.cmake"
  )
  c4c_set_backend_test_labels("${test_name}" backend_route ${ARG_LABELS})
endfunction()

add_executable(backend_prepare_phi_materialize_test
    "${BACKEND_TEST_ROOT}/backend_prepare_phi_materialize_test.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/bir.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/prepare/legalize.cpp"
)
target_include_directories(backend_prepare_phi_materialize_test PRIVATE
    "${PROJECT_SOURCE_DIR}"
)
target_link_libraries(backend_prepare_phi_materialize_test PRIVATE
    c4c_codegen
)
set_target_properties(backend_prepare_phi_materialize_test PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

add_test(
  NAME backend_prepare_phi_materialize
  COMMAND $<TARGET_FILE:backend_prepare_phi_materialize_test>
)
set_tests_properties(backend_prepare_phi_materialize PROPERTIES
  LABELS "backend;internal;cpp"
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_riscv64_two_param_select_eq_split_predecessor_deeper_then_mixed_affine_post_add_sub_observes_semantic_phi
  SRC "${BACKEND_CASE_ROOT}/three_way_phi_merge_post_add_sub.c"
  TARGET_TRIPLE riscv64-unknown-linux-gnu
  OUT_TEXT "${CMAKE_BINARY_DIR}/backend_route/three_way_phi_merge_post_add_sub_semantic_phi_riscv64.ll"
  EXTRA_COMPILER_ARGS "--backend-bir-stage|semantic"
  REQUIRED_SNIPPETS "bir.func @choose3_phi_post_chain(i32 %p.x, i32 %p.y, i32 %p.z) -> i32 {|%t19 = bir.phi i32 [tern.then.end.13, %t17] [tern.else.end.15, %t18]|%t20 = bir.phi i32 [tern.then.end.4, %t8] [tern.else.end.6, %t19]|%t21 = bir.add i32 %t20, 6|%t22 = bir.sub i32 %t21, 2|bir.ret i32 %t22"
  FORBIDDEN_SNIPPETS "define i32 @choose3_phi_post_chain(i32 %p.x, i32 %p.y, i32 %p.z)|bir.store_local|semantic_phi|bir.select"
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_riscv64_two_param_select_eq_split_predecessor_deeper_then_mixed_affine_post_add_sub_observes_semantic_bir
  SRC "${BACKEND_CASE_ROOT}/three_way_phi_merge_post_add_sub.c"
  TARGET_TRIPLE riscv64-unknown-linux-gnu
  OUT_TEXT "${CMAKE_BINARY_DIR}/backend_route/three_way_phi_merge_post_add_sub_semantic_bir_riscv64.ll"
  EXTRA_COMPILER_ARGS "--backend-bir-stage|semantic"
  REQUIRED_SNIPPETS "bir.func @choose3_phi_post_chain(i32 %p.x, i32 %p.y, i32 %p.z) -> i32 {|bir.phi i32|bir.ret i32"
  FORBIDDEN_SNIPPETS "define i32 @choose3_phi_post_chain(i32 %p.x, i32 %p.y, i32 %p.z)|bir.store_local|semantic_phi|bir.select"
)

add_test(
  NAME backend_codegen_route_x86_64_byval_member_array_params_observe_semantic_bir
  COMMAND "${CMAKE_COMMAND}"
          -DCOMPILER=$<TARGET_FILE:c4cll>
          -DTARGET_TRIPLE=x86_64-unknown-linux-gnu
          -DSRC_A=${BACKEND_CASE_ROOT}/param_member_array.c
          -DOUT_A=${CMAKE_BINARY_DIR}/backend_route/param_member_array_semantic_bir_x86_64.ll
          "-DREQUIRED_SNIPPETS_A=bir.func @get_second(ptr byval(size=8, align=4) %p.p) -> i32 {|bir.call i32 get_second(ptr byval(size=8, align=4) %lv.p)|bir.ret i32 %t9"
          -DSRC_B=${BACKEND_CASE_ROOT}/nested_param_member_array.c
          -DOUT_B=${CMAKE_BINARY_DIR}/backend_route/nested_param_member_array_semantic_bir_x86_64.ll
          "-DREQUIRED_SNIPPETS_B=bir.func @get_second(ptr byval(size=8, align=4) %p.o) -> i32 {|bir.call i32 get_second(ptr byval(size=8, align=4) %lv.outer)|bir.ret i32 %t11"
          -P "${BACKEND_TEST_CMAKE_ROOT}/run_backend_dual_codegen_route_case.cmake"
)
c4c_set_backend_test_labels(
  "backend_codegen_route_x86_64_byval_member_array_params_observe_semantic_bir"
  backend_route
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_x86_64_aggregate_return_pair_observe_semantic_bir
  SRC "${BACKEND_CASE_ROOT}/aggregate_return_pair.c"
  TARGET_TRIPLE x86_64-unknown-linux-gnu
  OUT_TEXT "${CMAKE_BINARY_DIR}/backend_route/aggregate_return_pair_semantic_bir_x86_64.ll"
  EXTRA_COMPILER_ARGS "--backend-bir-stage|semantic"
  REQUIRED_SNIPPETS "bir.func @make_pair(ptr sret(size=8, align=4) %ret.sret) -> void {|bir.store_local %lv.p.0, i32 3|bir.store_local %lv.p.0, i32 make_pair.ret.sret.copy.0, addr %ret.sret|bir.call void make_pair(ptr sret(size=8, align=4) %t0)|bir.ret i32 %t2"
  FORBIDDEN_SNIPPETS "define %struct.Pair @make_pair()|call %struct.Pair () @make_pair()"
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_x86_64_aggregate_param_return_pair_observe_semantic_bir
  SRC "${BACKEND_CASE_ROOT}/aggregate_param_return_pair.c"
  TARGET_TRIPLE x86_64-unknown-linux-gnu
  OUT_TEXT "${CMAKE_BINARY_DIR}/backend_route/aggregate_param_return_pair_semantic_bir_x86_64.ll"
  EXTRA_COMPILER_ARGS "--backend-bir-stage|semantic"
  REQUIRED_SNIPPETS "bir.func @id_pair(ptr sret(size=8, align=4) %ret.sret, ptr byval(size=8, align=4) %p.p) -> void {|%lv.param.p.p.aggregate.param.copy.0 = bir.load_local i32 %lv.param.p.p.0, addr %p.p|bir.store_local %lv.param.p.p.0, i32 id_pair.ret.sret.copy.0, addr %ret.sret|bir.call void id_pair(ptr sret(size=8, align=4) %t3, ptr byval(size=8, align=4) %lv.p)|bir.ret i32 %t5"
  FORBIDDEN_SNIPPETS "define %struct.Pair @id_pair(%struct.Pair %p.p)|call %struct.Pair (%struct.Pair) @id_pair(%struct.Pair %t2)"
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_x86_64_indirect_aggregate_param_return_pair_observe_semantic_bir
  SRC "${BACKEND_CASE_ROOT}/indirect_aggregate_param_return_pair.c"
  TARGET_TRIPLE x86_64-unknown-linux-gnu
  OUT_TEXT "${CMAKE_BINARY_DIR}/backend_route/indirect_aggregate_param_return_pair_semantic_bir_x86_64.ll"
  EXTRA_COMPILER_ARGS "--backend-bir-stage|semantic"
  REQUIRED_SNIPPETS "bir.func @id_pair(ptr sret(size=8, align=4) %ret.sret, ptr byval(size=8, align=4) %p.p) -> void {|bir.store_local %lv.p.0, i32 4|bir.call void @id_pair(ptr sret(size=8, align=4) %t4, ptr byval(size=8, align=4) %lv.p)|bir.ret i32 %t6"
  FORBIDDEN_SNIPPETS "define %struct.Pair @id_pair(%struct.Pair %p.p)|call %struct.Pair (%struct.Pair) %t2(%struct.Pair %t3)"
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_x86_64_function_pointer_param_direct_arg_observe_semantic_bir
  SRC "${BACKEND_CASE_ROOT}/function_pointer_param_direct_arg.c"
  TARGET_TRIPLE x86_64-unknown-linux-gnu
  OUT_TEXT "${CMAKE_BINARY_DIR}/backend_route/function_pointer_param_direct_arg_semantic_bir_x86_64.ll"
  EXTRA_COMPILER_ARGS "--backend-bir-stage|semantic"
  REQUIRED_SNIPPETS "bir.func @apply(ptr %p.fn, i32 %p.x) -> i32 {|%t0 = bir.call i32 %p.fn(i32 %p.x)|bir.call i32 apply(ptr @inc, i32 4)|bir.ret i32 %t0"
  FORBIDDEN_SNIPPETS "define i32 @main()|call i32 (ptr, i32) @apply(ptr @inc, i32 4)"
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_x86_64_aggregate_param_return_pair_fn_param_observe_semantic_bir
  SRC "${BACKEND_CASE_ROOT}/aggregate_param_return_pair_fn_param.c"
  TARGET_TRIPLE x86_64-unknown-linux-gnu
  OUT_TEXT "${CMAKE_BINARY_DIR}/backend_route/aggregate_param_return_pair_fn_param_semantic_bir_x86_64.ll"
  EXTRA_COMPILER_ARGS "--backend-bir-stage|semantic"
  REQUIRED_SNIPPETS "bir.func @use(ptr %p.fn, ptr byval(size=8, align=4) %p.p) -> i32 {|bir.call void %p.fn(ptr sret(size=8, align=4) %t0, ptr byval(size=8, align=4) %lv.param.p.p)|%t3 = bir.call i32 use(ptr @id_pair, ptr byval(size=8, align=4) %lv.p)|bir.ret i32 %t3"
  FORBIDDEN_SNIPPETS "define i32 @main()|call i32 (ptr, %struct.Pair) @use(ptr @id_pair, %struct.Pair %t2)|call %struct.Pair (%struct.Pair) %p.fn(%struct.Pair %p.p)"
)
