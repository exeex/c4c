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
  backend_codegen_route_x86_64_pointer_return_direct_observe_semantic_bir
  SRC "${BACKEND_CASE_ROOT}/pointer_return_direct.c"
  TARGET_TRIPLE x86_64-unknown-linux-gnu
  OUT_TEXT "${CMAKE_BINARY_DIR}/backend_route/pointer_return_direct_semantic_bir_x86_64.ll"
  EXTRA_COMPILER_ARGS "--backend-bir-stage|semantic"
  REQUIRED_SNIPPETS "bir.func @id_ptr(ptr %p.p) -> ptr {|bir.func @forward(ptr %p.p) -> ptr {|%t0 = bir.call ptr id_ptr(ptr %p.p)|bir.ret ptr %t0"
  FORBIDDEN_SNIPPETS "define ptr @forward(ptr %p.p)|call ptr (ptr) @id_ptr(ptr %p.p)"
)

add_test(
  NAME backend_codegen_route_x86_64_builtin_abs_observe_semantic_bir
  COMMAND "${CMAKE_COMMAND}"
          -DCOMPILER=$<TARGET_FILE:c4cll>
          -DTARGET_TRIPLE=x86_64-unknown-linux-gnu
          -DSRC_A=${BACKEND_CASE_ROOT}/builtin_abs_i32.c
          -DOUT_A=${CMAKE_BINARY_DIR}/backend_route/builtin_abs_i32_semantic_bir_x86_64.ll
          "-DREQUIRED_SNIPPETS_A=bir.func @take_abs_int(i32 %p.x) -> i32 {|%t0.neg = bir.sub i32 0, %p.x|%t0 = bir.select slt i32 %p.x, 0, i32 %t0.neg, %p.x|bir.ret i32 %t0"
          "-DFORBIDDEN_SNIPPETS_A=@llvm.abs.i32|bir.call i32 @llvm.abs.i32"
          -DSRC_B=${BACKEND_CASE_ROOT}/builtin_abs_i64.c
          -DOUT_B=${CMAKE_BINARY_DIR}/backend_route/builtin_abs_i64_semantic_bir_x86_64.ll
          "-DREQUIRED_SNIPPETS_B=bir.func @take_abs_long(i64 %p.x) -> i64 {|%t0.neg = bir.sub i64 0, %p.x|%t0 = bir.select slt i64 %p.x, 0, i64 %t0.neg, %p.x|bir.ret i64 %t0"
          "-DFORBIDDEN_SNIPPETS_B=@llvm.abs.i64|bir.call i64 @llvm.abs.i64"
          -P "${BACKEND_TEST_CMAKE_ROOT}/run_backend_dual_codegen_route_case.cmake"
)
c4c_set_backend_test_labels(
  "backend_codegen_route_x86_64_builtin_abs_observe_semantic_bir"
  backend_route
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_x86_64_builtin_memcpy_local_pair_observe_semantic_bir
  SRC "${BACKEND_CASE_ROOT}/builtin_memcpy_local_pair.c"
  TARGET_TRIPLE x86_64-unknown-linux-gnu
  OUT_TEXT "${CMAKE_BINARY_DIR}/backend_route/builtin_memcpy_local_pair_semantic_bir_x86_64.ll"
  EXTRA_COMPILER_ARGS "--backend-bir-stage|semantic"
  REQUIRED_SNIPPETS "bir.func @main() -> i32 {|%lv.dst.memcpy.copy.0 = bir.load_local i32 %lv.src.0|bir.store_local %lv.dst.0, i32 %lv.dst.memcpy.copy.0|%lv.dst.memcpy.copy.4 = bir.load_local i32 %lv.src.4|bir.store_local %lv.dst.4, i32 %lv.dst.memcpy.copy.4|bir.ret i32 %t7"
  FORBIDDEN_SNIPPETS "define i32 @main()|call ptr @memcpy|bir.call ptr memcpy"
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_x86_64_builtin_memcpy_nested_pair_field_observe_semantic_bir
  SRC "${BACKEND_CASE_ROOT}/builtin_memcpy_nested_pair_field.c"
  TARGET_TRIPLE x86_64-unknown-linux-gnu
  OUT_TEXT "${CMAKE_BINARY_DIR}/backend_route/builtin_memcpy_nested_pair_field_semantic_bir_x86_64.ll"
  EXTRA_COMPILER_ARGS "--backend-bir-stage|semantic"
  REQUIRED_SNIPPETS "bir.func @main() -> i32 {|%t10.memcpy.copy.0 = bir.load_local i32 %lv.src.4|bir.store_local %lv.dst.4, i32 %t10.memcpy.copy.0|%t10.memcpy.copy.4 = bir.load_local i32 %lv.src.8|bir.store_local %lv.dst.8, i32 %t10.memcpy.copy.4|bir.ret i32 %t22"
  FORBIDDEN_SNIPPETS "define i32 @main()|call ptr @memcpy|bir.call ptr memcpy"
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_x86_64_builtin_memcpy_local_i32_array_observe_semantic_bir
  SRC "${BACKEND_CASE_ROOT}/builtin_memcpy_local_i32_array.c"
  TARGET_TRIPLE x86_64-unknown-linux-gnu
  OUT_TEXT "${CMAKE_BINARY_DIR}/backend_route/builtin_memcpy_local_i32_array_semantic_bir_x86_64.ll"
  EXTRA_COMPILER_ARGS "--backend-bir-stage|semantic"
  REQUIRED_SNIPPETS "bir.func @main() -> i32 {|%t36.memcpy.copy.0 = bir.load_local i32 %lv.src.0|bir.store_local %lv.dst.0, i32 %t36.memcpy.copy.0|%t36.memcpy.copy.4 = bir.load_local i32 %lv.src.1|bir.store_local %lv.dst.1, i32 %t36.memcpy.copy.4|%t36.memcpy.copy.8 = bir.load_local i32 %lv.src.2|bir.store_local %lv.dst.2, i32 %t36.memcpy.copy.8|bir.ret i32 %t52"
  FORBIDDEN_SNIPPETS "define i32 @main()|call ptr @memcpy|bir.call ptr memcpy"
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_x86_64_builtin_memcpy_nested_i32_array_field_observe_semantic_bir
  SRC "${BACKEND_CASE_ROOT}/builtin_memcpy_nested_i32_array_field.c"
  TARGET_TRIPLE x86_64-unknown-linux-gnu
  OUT_TEXT "${CMAKE_BINARY_DIR}/backend_route/builtin_memcpy_nested_i32_array_field_semantic_bir_x86_64.ll"
  EXTRA_COMPILER_ARGS "--backend-bir-stage|semantic"
  REQUIRED_SNIPPETS "bir.func @main() -> i32 {|%t26.memcpy.copy.0 = bir.load_local i32 %lv.w.4|bir.store_local %lv.w.16, i32 %t26.memcpy.copy.0|%t26.memcpy.copy.4 = bir.load_local i32 %lv.w.8|bir.store_local %lv.w.20, i32 %t26.memcpy.copy.4|%t26.memcpy.copy.8 = bir.load_local i32 %lv.w.12|bir.store_local %lv.w.24, i32 %t26.memcpy.copy.8|bir.ret i32 %t49"
  FORBIDDEN_SNIPPETS "define i32 @main()|call ptr @memcpy|bir.call ptr memcpy"
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_x86_64_builtin_memcpy_local_i32_array_to_pair_observe_semantic_bir
  SRC "${BACKEND_CASE_ROOT}/builtin_memcpy_local_i32_array_to_pair.c"
  TARGET_TRIPLE x86_64-unknown-linux-gnu
  OUT_TEXT "${CMAKE_BINARY_DIR}/backend_route/builtin_memcpy_local_i32_array_to_pair_semantic_bir_x86_64.ll"
  EXTRA_COMPILER_ARGS "--backend-bir-stage|semantic"
  REQUIRED_SNIPPETS "bir.func @main() -> i32 {|%lv.dst.memcpy.copy.0 = bir.load_local i32 %lv.src.0|bir.store_local %lv.dst.0, i32 %lv.dst.memcpy.copy.0|%lv.dst.memcpy.copy.4 = bir.load_local i32 %lv.src.1|bir.store_local %lv.dst.4, i32 %lv.dst.memcpy.copy.4|bir.ret i32 %t20"
  FORBIDDEN_SNIPPETS "define i32 @main()|call ptr @memcpy|bir.call ptr memcpy"
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_x86_64_builtin_memcpy_local_pair_to_i32_array_observe_semantic_bir
  SRC "${BACKEND_CASE_ROOT}/builtin_memcpy_local_pair_to_i32_array.c"
  TARGET_TRIPLE x86_64-unknown-linux-gnu
  OUT_TEXT "${CMAKE_BINARY_DIR}/backend_route/builtin_memcpy_local_pair_to_i32_array_semantic_bir_x86_64.ll"
  EXTRA_COMPILER_ARGS "--backend-bir-stage|semantic"
  REQUIRED_SNIPPETS "bir.func @main() -> i32 {|memcpy.copy.0 = bir.load_local i32 %lv.src.0|bir.store_local %lv.dst.0, i32 %t14.memcpy.copy.0|memcpy.copy.4 = bir.load_local i32 %lv.src.4|bir.store_local %lv.dst.1, i32 %t14.memcpy.copy.4|bir.ret i32 %t24"
  FORBIDDEN_SNIPPETS "define i32 @main()|call ptr @memcpy|bir.call ptr memcpy"
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_x86_64_builtin_memcpy_prefix_local_i32_array_to_pair_observe_semantic_bir
  SRC "${BACKEND_CASE_ROOT}/builtin_memcpy_prefix_local_i32_array_to_pair.c"
  TARGET_TRIPLE x86_64-unknown-linux-gnu
  OUT_TEXT "${CMAKE_BINARY_DIR}/backend_route/builtin_memcpy_prefix_local_i32_array_to_pair_semantic_bir_x86_64.ll"
  EXTRA_COMPILER_ARGS "--backend-bir-stage|semantic"
  REQUIRED_SNIPPETS "bir.func @main() -> i32 {|%lv.dst.memcpy.copy.0 = bir.load_local i32 %lv.src.4|bir.store_local %lv.dst.0, i32 %lv.dst.memcpy.copy.0|%lv.dst.memcpy.copy.4 = bir.load_local i32 %lv.src.8|bir.store_local %lv.dst.4, i32 %lv.dst.memcpy.copy.4|bir.ret i32 %t31"
  FORBIDDEN_SNIPPETS "define i32 @main()|call ptr @memcpy|bir.call ptr memcpy"
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_x86_64_builtin_memcpy_prefix_local_pair_to_i32_array_observe_semantic_bir
  SRC "${BACKEND_CASE_ROOT}/builtin_memcpy_prefix_local_pair_to_i32_array.c"
  TARGET_TRIPLE x86_64-unknown-linux-gnu
  OUT_TEXT "${CMAKE_BINARY_DIR}/backend_route/builtin_memcpy_prefix_local_pair_to_i32_array_semantic_bir_x86_64.ll"
  EXTRA_COMPILER_ARGS "--backend-bir-stage|semantic"
  REQUIRED_SNIPPETS "bir.func @main() -> i32 {|%t16.memcpy.copy.0 = bir.load_local i32 %lv.src.0|bir.store_local %lv.dst.4, i32 %t16.memcpy.copy.0|%t16.memcpy.copy.4 = bir.load_local i32 %lv.src.4|bir.store_local %lv.dst.8, i32 %t16.memcpy.copy.4|bir.ret i32 %t37"
  FORBIDDEN_SNIPPETS "define i32 @main()|call ptr @memcpy|bir.call ptr memcpy"
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_x86_64_builtin_memcpy_prefix_local_i32_subarray_to_nested_pair_field_observe_semantic_bir
  SRC "${BACKEND_CASE_ROOT}/builtin_memcpy_prefix_local_i32_subarray_to_nested_pair_field.c"
  TARGET_TRIPLE x86_64-unknown-linux-gnu
  OUT_TEXT "${CMAKE_BINARY_DIR}/backend_route/builtin_memcpy_prefix_local_i32_subarray_to_nested_pair_field_semantic_bir_x86_64.ll"
  EXTRA_COMPILER_ARGS "--backend-bir-stage|semantic"
  REQUIRED_SNIPPETS "bir.func @main() -> i32 {|%t30.memcpy.copy.0 = bir.load_local i32 %lv.src.8|bir.store_local %lv.dst.8, i32 %t30.memcpy.copy.0|%t30.memcpy.copy.4 = bir.load_local i32 %lv.src.12|bir.store_local %lv.dst.12, i32 %t30.memcpy.copy.4|bir.ret i32 %t60"
  FORBIDDEN_SNIPPETS "define i32 @main()|call ptr @memcpy|bir.call ptr memcpy"
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_x86_64_builtin_memset_local_pair_observe_semantic_bir
  SRC "${BACKEND_CASE_ROOT}/builtin_memset_local_pair.c"
  TARGET_TRIPLE x86_64-unknown-linux-gnu
  OUT_TEXT "${CMAKE_BINARY_DIR}/backend_route/builtin_memset_local_pair_semantic_bir_x86_64.ll"
  EXTRA_COMPILER_ARGS "--backend-bir-stage|semantic"
  REQUIRED_SNIPPETS "bir.func @main() -> i32 {|bir.store_local %lv.dst.0, i32 1|bir.store_local %lv.dst.4, i32 2|bir.store_local %lv.dst.0, i32 0|bir.store_local %lv.dst.4, i32 0|bir.ret i32 %t7"
  FORBIDDEN_SNIPPETS "define i32 @main()|call ptr @memset|bir.call ptr memset"
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_x86_64_builtin_memset_local_i32_array_observe_semantic_bir
  SRC "${BACKEND_CASE_ROOT}/builtin_memset_local_i32_array.c"
  TARGET_TRIPLE x86_64-unknown-linux-gnu
  OUT_TEXT "${CMAKE_BINARY_DIR}/backend_route/builtin_memset_local_i32_array_semantic_bir_x86_64.ll"
  EXTRA_COMPILER_ARGS "--backend-bir-stage|semantic"
  REQUIRED_SNIPPETS "bir.func @main() -> i32 {|bir.store_local %lv.dst.0, i32 1|bir.store_local %lv.dst.1, i32 2|bir.store_local %lv.dst.2, i32 3|bir.store_local %lv.dst.0, i32 0|bir.store_local %lv.dst.1, i32 0|bir.store_local %lv.dst.2, i32 0|bir.ret i32 %t33"
  FORBIDDEN_SNIPPETS "define i32 @main()|call ptr @memset|bir.call ptr memset"
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_x86_64_builtin_memset_nested_i32_array_field_observe_semantic_bir
  SRC "${BACKEND_CASE_ROOT}/builtin_memset_nested_i32_array_field.c"
  TARGET_TRIPLE x86_64-unknown-linux-gnu
  OUT_TEXT "${CMAKE_BINARY_DIR}/backend_route/builtin_memset_nested_i32_array_field_semantic_bir_x86_64.ll"
  EXTRA_COMPILER_ARGS "--backend-bir-stage|semantic"
  REQUIRED_SNIPPETS "bir.func @main() -> i32 {|bir.store_local %lv.dst.0, i32 7|bir.store_local %lv.dst.4, i32 1|bir.store_local %lv.dst.8, i32 2|bir.store_local %lv.dst.12, i32 3|bir.store_local %lv.dst.4, i32 0|bir.store_local %lv.dst.8, i32 0|bir.store_local %lv.dst.12, i32 0|bir.ret i32 %t35"
  FORBIDDEN_SNIPPETS "define i32 @main()|call ptr @memset|bir.call ptr memset"
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_x86_64_pointer_param_direct_global_arg_observe_semantic_bir
  SRC "${BACKEND_CASE_ROOT}/pointer_param_direct_global_arg.c"
  TARGET_TRIPLE x86_64-unknown-linux-gnu
  OUT_TEXT "${CMAKE_BINARY_DIR}/backend_route/pointer_param_direct_global_arg_semantic_bir_x86_64.ll"
  EXTRA_COMPILER_ARGS "--backend-bir-stage|semantic"
  REQUIRED_SNIPPETS "bir.func @consume(ptr %p.p) -> i32 {|bir.call i32 consume(ptr @g)|bir.ret i32 %t0"
  FORBIDDEN_SNIPPETS "define i32 @main()|call i32 (ptr) @consume(ptr @g)"
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_x86_64_pointer_param_global_pointer_value_arg_observe_semantic_bir
  SRC "${BACKEND_CASE_ROOT}/pointer_param_global_pointer_value_arg.c"
  TARGET_TRIPLE x86_64-unknown-linux-gnu
  OUT_TEXT "${CMAKE_BINARY_DIR}/backend_route/pointer_param_global_pointer_value_arg_semantic_bir_x86_64.ll"
  EXTRA_COMPILER_ARGS "--backend-bir-stage|semantic"
  REQUIRED_SNIPPETS "bir.func @consume(ptr %p.p) -> i32 {|%t0 = bir.load_global ptr @gp|%t1 = bir.load_local ptr %lv.p|bir.call i32 consume(ptr %t1)|bir.ret i32 %t2"
  FORBIDDEN_SNIPPETS "define i32 @main()|call i32 (ptr) @consume("
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_x86_64_pointer_return_fn_param_observe_semantic_bir
  SRC "${BACKEND_CASE_ROOT}/pointer_return_fn_param.c"
  TARGET_TRIPLE x86_64-unknown-linux-gnu
  OUT_TEXT "${CMAKE_BINARY_DIR}/backend_route/pointer_return_fn_param_semantic_bir_x86_64.ll"
  EXTRA_COMPILER_ARGS "--backend-bir-stage|semantic"
  REQUIRED_SNIPPETS "bir.func @id_ptr(ptr %p.p) -> ptr {|bir.func @use(ptr %p.fn, ptr %p.p) -> ptr {|%t0 = bir.call ptr %p.fn(ptr %p.p)|bir.ret ptr %t0"
  FORBIDDEN_SNIPPETS "define ptr @use(ptr %p.fn, ptr %p.p)|call ptr (ptr) %p.fn(ptr %p.p)"
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_x86_64_pointer_return_global_pointer_value_observe_semantic_bir
  SRC "${BACKEND_CASE_ROOT}/pointer_return_global_pointer_value.c"
  TARGET_TRIPLE x86_64-unknown-linux-gnu
  OUT_TEXT "${CMAKE_BINARY_DIR}/backend_route/pointer_return_global_pointer_value_semantic_bir_x86_64.ll"
  EXTRA_COMPILER_ARGS "--backend-bir-stage|semantic"
  REQUIRED_SNIPPETS "bir.func @id_ptr(ptr %p.p) -> ptr {|bir.func @use() -> ptr {|%t0 = bir.load_global ptr @gp|%t1 = bir.load_local ptr %lv.p|bir.call ptr id_ptr(ptr %t1)|bir.ret ptr %t2"
  FORBIDDEN_SNIPPETS "define ptr @use()|call ptr (ptr) @id_ptr("
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

add_test(
  NAME backend_codegen_route_x86_64_nested_member_pointer_array_observe_semantic_bir
  COMMAND "${CMAKE_COMMAND}"
          -DCOMPILER=$<TARGET_FILE:c4cll>
          -DTARGET_TRIPLE=x86_64-unknown-linux-gnu
          -DSRC_A=${BACKEND_CASE_ROOT}/nested_member_pointer_array.c
          -DOUT_A=${CMAKE_BINARY_DIR}/backend_route/nested_member_pointer_array_semantic_bir_x86_64.ll
          "-DREQUIRED_SNIPPETS_A=bir.func @get_second(ptr %p.o) -> i32 {|%t1 = bir.load_local ptr %t1.addr, addr %p.o|%t6 = bir.load_local i32 %t6.addr, addr %t1+4|bir.store_local %lv.outer.0, ptr %lv.inner|bir.call i32 get_second(ptr %lv.outer)|bir.ret i32 %t9"
          -DSRC_B=${BACKEND_CASE_ROOT}/local_array.c
          -DOUT_B=${CMAKE_BINARY_DIR}/backend_route/nested_member_pointer_array_local_array_semantic_bir_x86_64.ll
          "-DREQUIRED_SNIPPETS_B=bir.func @main() -> i32 {|bir.store_local %lv.xs.0, i32 4|bir.store_local %lv.xs.1, i32 3|%t14 = bir.add i32 %t9, %t13|bir.ret i32 %t14"
          -P "${BACKEND_TEST_CMAKE_ROOT}/run_backend_dual_codegen_route_case.cmake"
)
c4c_set_backend_test_labels(
  "backend_codegen_route_x86_64_nested_member_pointer_array_observe_semantic_bir"
  backend_route
)

add_test(
  NAME backend_codegen_route_x86_64_local_dynamic_member_array_observe_semantic_bir
  COMMAND "${CMAKE_COMMAND}"
          -DCOMPILER=$<TARGET_FILE:c4cll>
          -DTARGET_TRIPLE=x86_64-unknown-linux-gnu
          -DSRC_A=${BACKEND_CASE_ROOT}/local_dynamic_member_array.c
          -DOUT_A=${CMAKE_BINARY_DIR}/backend_route/local_dynamic_member_array_semantic_bir_x86_64.ll
          "-DREQUIRED_SNIPPETS_A=bir.func @get_at(ptr %p.p, i32 %p.i) -> i32 {|%t4.elt0 = bir.load_local i32 %t4.elt0.addr, addr %p.p|%t4.elt1 = bir.load_local i32 %t4.elt1.addr, addr %p.p+4|%t4.elt2 = bir.load_local i32 %t4.elt2.addr, addr %p.p+8|%t4 = bir.select eq i64 %t2, 0, i32 %t4.elt0, %t4.sel1|bir.call i32 get_at(ptr %lv.p, i32 2)|bir.ret i32 %t12"
          "-DFORBIDDEN_SNIPPETS_A=define i32 @get_at|getelementptr [3 x i32], ptr"
          -DSRC_B=${BACKEND_CASE_ROOT}/local_array.c
          -DOUT_B=${CMAKE_BINARY_DIR}/backend_route/local_dynamic_member_array_local_array_semantic_bir_x86_64.ll
          "-DREQUIRED_SNIPPETS_B=bir.func @main() -> i32 {|bir.store_local %lv.xs.0, i32 4|bir.store_local %lv.xs.1, i32 3|%t14 = bir.add i32 %t9, %t13|bir.ret i32 %t14"
          -P "${BACKEND_TEST_CMAKE_ROOT}/run_backend_dual_codegen_route_case.cmake"
)
c4c_set_backend_test_labels(
  "backend_codegen_route_x86_64_local_dynamic_member_array_observe_semantic_bir"
  backend_route
)

add_test(
  NAME backend_codegen_route_x86_64_local_dynamic_member_array_store_observe_semantic_bir
  COMMAND "${CMAKE_COMMAND}"
          -DCOMPILER=$<TARGET_FILE:c4cll>
          -DTARGET_TRIPLE=x86_64-unknown-linux-gnu
          -DSRC_A=${BACKEND_CASE_ROOT}/local_dynamic_member_array_store.c
          -DOUT_A=${CMAKE_BINARY_DIR}/backend_route/local_dynamic_member_array_store_semantic_bir_x86_64.ll
          "-DREQUIRED_SNIPPETS_A=bir.func @set_at(ptr %p.p, i32 %p.i, i32 %p.v) -> void {|%t2 = bir.sext i32 %p.i to i64|%t3.elt0 = bir.load_local i32 %t3.elt0.load.addr, addr %p.p|%t3.store0 = bir.select eq i64 %t2, 0, i32 %p.v, %t3.elt0|bir.store_local %t3.elt0.store.addr, i32 %t3.store0, addr %p.p|%t3.elt1 = bir.load_local i32 %t3.elt1.load.addr, addr %p.p+4|%t3.store1 = bir.select eq i64 %t2, 1, i32 %p.v, %t3.elt1|bir.store_local %t3.elt1.store.addr, i32 %t3.store1, addr %p.p+4|%t3.elt2 = bir.load_local i32 %t3.elt2.load.addr, addr %p.p+8|%t3.store2 = bir.select eq i64 %t2, 2, i32 %p.v, %t3.elt2|bir.store_local %t3.elt2.store.addr, i32 %t3.store2, addr %p.p+8|bir.ret"
          "-DFORBIDDEN_SNIPPETS_A=define void @set_at|store i32 %p.v, ptr|getelementptr [3 x i32], ptr"
          -DSRC_B=${BACKEND_CASE_ROOT}/local_dynamic_member_array.c
          -DOUT_B=${CMAKE_BINARY_DIR}/backend_route/local_dynamic_member_array_store_neighbor_semantic_bir_x86_64.ll
          "-DREQUIRED_SNIPPETS_B=bir.func @get_at(ptr %p.p, i32 %p.i) -> i32 {|%t4.elt0 = bir.load_local i32 %t4.elt0.addr, addr %p.p|%t4.elt1 = bir.load_local i32 %t4.elt1.addr, addr %p.p+4|%t4.elt2 = bir.load_local i32 %t4.elt2.addr, addr %p.p+8|%t4 = bir.select eq i64 %t2, 0, i32 %t4.elt0, %t4.sel1|bir.call i32 get_at(ptr %lv.p, i32 2)|bir.ret i32 %t12"
          -P "${BACKEND_TEST_CMAKE_ROOT}/run_backend_dual_codegen_route_case.cmake"
)
c4c_set_backend_test_labels(
  "backend_codegen_route_x86_64_local_dynamic_member_array_store_observe_semantic_bir"
  backend_route
)

add_test(
  NAME backend_codegen_route_x86_64_local_direct_dynamic_member_array_store_observe_semantic_bir
  COMMAND "${CMAKE_COMMAND}"
          -DCOMPILER=$<TARGET_FILE:c4cll>
          -DTARGET_TRIPLE=x86_64-unknown-linux-gnu
          -DSRC_A=${BACKEND_CASE_ROOT}/local_direct_dynamic_member_array_store.c
          -DOUT_A=${CMAKE_BINARY_DIR}/backend_route/local_direct_dynamic_member_array_store_semantic_bir_x86_64.ll
          "-DREQUIRED_SNIPPETS_A=bir.func @main() -> i32 {|bir.store_local %lv.p.0, i32 4|bir.store_local %lv.p.4, i32 8|bir.store_local %lv.p.8, i32 11|bir.store_local %lv.i, i32 1|%t16.elt0 = bir.load_local i32 %lv.p.0|%t16.store1 = bir.select eq i64 %t15, 1, i32 13, %t16.elt1|bir.store_local %lv.p.8, i32 %t16.store2|%t21 = bir.load_local i32 %lv.p.4|bir.ret i32 %t21"
          "-DFORBIDDEN_SNIPPETS_A=define i32 @main()|store i32 13, ptr|getelementptr [3 x i32], ptr"
          -DSRC_B=${BACKEND_CASE_ROOT}/local_array.c
          -DOUT_B=${CMAKE_BINARY_DIR}/backend_route/local_direct_dynamic_member_array_store_local_array_semantic_bir_x86_64.ll
          "-DREQUIRED_SNIPPETS_B=bir.func @main() -> i32 {|bir.store_local %lv.xs.0, i32 4|bir.store_local %lv.xs.1, i32 3|%t14 = bir.add i32 %t9, %t13|bir.ret i32 %t14"
          -P "${BACKEND_TEST_CMAKE_ROOT}/run_backend_dual_codegen_route_case.cmake"
)
c4c_set_backend_test_labels(
  "backend_codegen_route_x86_64_local_direct_dynamic_member_array_store_observe_semantic_bir"
  backend_route
)

add_test(
  NAME backend_codegen_route_x86_64_local_direct_dynamic_member_array_load_observe_semantic_bir
  COMMAND "${CMAKE_COMMAND}"
          -DCOMPILER=$<TARGET_FILE:c4cll>
          -DTARGET_TRIPLE=x86_64-unknown-linux-gnu
          -DSRC_A=${BACKEND_CASE_ROOT}/local_direct_dynamic_member_array_load.c
          -DOUT_A=${CMAKE_BINARY_DIR}/backend_route/local_direct_dynamic_member_array_load_semantic_bir_x86_64.ll
          "-DREQUIRED_SNIPPETS_A=bir.func @main() -> i32 {|bir.store_local %lv.p.0, i32 4|bir.store_local %lv.p.4, i32 8|bir.store_local %lv.p.8, i32 11|bir.store_local %lv.i, i32 1|%t14 = bir.load_local i32 %lv.i|%t15 = bir.sext i32 %t14 to i64|%t17.elt0 = bir.load_local i32 %lv.p.0|%t17.elt1 = bir.load_local i32 %lv.p.4|%t17.elt2 = bir.load_local i32 %lv.p.8|%t17 = bir.select eq i64 %t15, 0, i32 %t17.elt0, %t17.sel1|bir.ret i32 %t17"
          "-DFORBIDDEN_SNIPPETS_A=define i32 @main()|load i32, ptr|getelementptr [3 x i32], ptr"
          -DSRC_B=${BACKEND_CASE_ROOT}/local_array.c
          -DOUT_B=${CMAKE_BINARY_DIR}/backend_route/local_direct_dynamic_member_array_load_local_array_semantic_bir_x86_64.ll
          "-DREQUIRED_SNIPPETS_B=bir.func @main() -> i32 {|bir.store_local %lv.xs.0, i32 4|bir.store_local %lv.xs.1, i32 3|%t14 = bir.add i32 %t9, %t13|bir.ret i32 %t14"
          -P "${BACKEND_TEST_CMAKE_ROOT}/run_backend_dual_codegen_route_case.cmake"
)
c4c_set_backend_test_labels(
  "backend_codegen_route_x86_64_local_direct_dynamic_member_array_load_observe_semantic_bir"
  backend_route
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_x86_64_local_direct_dynamic_struct_array_call_observe_semantic_bir
  SRC "${BACKEND_CASE_ROOT}/local_direct_dynamic_struct_array_call.c"
  TARGET_TRIPLE x86_64-unknown-linux-gnu
  OUT_TEXT "${CMAKE_BINARY_DIR}/backend_route/local_direct_dynamic_struct_array_call_semantic_bir_x86_64.ll"
  EXTRA_COMPILER_ARGS "--backend-bir-stage|semantic"
  REQUIRED_SNIPPETS "bir.func @main() -> i32 {|bir.store_local %lv.pairs.0, i32 1|bir.store_local %lv.pairs.12, i32 4|%t17 = bir.load_local i32 %lv.i|%t18 = bir.sext i32 %t17 to i64|%t19 = bir.select eq i64 %t18, 0, ptr %t19.elt0, %t19.elt1|bir.call i32 get_y(ptr %t19)|bir.ret i32 %t20"
  FORBIDDEN_SNIPPETS "define i32 @main()|call i32 (ptr) @get_y|getelementptr [2 x %struct.Pair], ptr"
)
