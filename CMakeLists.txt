cmake_minimum_required(VERSION 3.20)
project(tiny_c2ll LANGUAGES CXX)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif()

option(ENABLE_HEAVY_TENTATIVE_SNAPSHOT
       "Use full parser-state snapshots in TentativeParseGuard speculative parses"
       ON)

find_program(CLANG_EXECUTABLE NAMES clang)
if(NOT CLANG_EXECUTABLE)
  message(WARNING "clang not found: clang execution tests will be skipped")
endif()

find_program(OBJDUMP_EXECUTABLE NAMES llvm-objdump objdump gobjdump)
if(NOT OBJDUMP_EXECUTABLE)
  message(WARNING "objdump not found: backend object contract tests will be skipped")
endif()

set(EXAMPLE_C "${PROJECT_SOURCE_DIR}/tests/c/internal/example/example.c")
set(EXAMPLE_LL "${CMAKE_BINARY_DIR}/example.ll")

set(FRONTEND_CXX_COMMON_SRCS
    "${PROJECT_SOURCE_DIR}/src/frontend/arena.cpp"
    "${PROJECT_SOURCE_DIR}/src/frontend/lexer/token.cpp"
    "${PROJECT_SOURCE_DIR}/src/frontend/lexer/lexer.cpp"
    "${PROJECT_SOURCE_DIR}/src/frontend/sema/type_utils.cpp"
    "${PROJECT_SOURCE_DIR}/src/frontend/sema/canonical_symbol.cpp"
    "${PROJECT_SOURCE_DIR}/src/frontend/sema/sema.cpp"
    "${PROJECT_SOURCE_DIR}/src/frontend/sema/validate.cpp"
    "${PROJECT_SOURCE_DIR}/src/frontend/sema/consteval.cpp"
    "${PROJECT_SOURCE_DIR}/src/frontend/preprocessor/preprocessor.cpp"
    "${PROJECT_SOURCE_DIR}/src/frontend/preprocessor/pp_text.cpp"
    "${PROJECT_SOURCE_DIR}/src/frontend/preprocessor/pp_cond.cpp"
    "${PROJECT_SOURCE_DIR}/src/frontend/preprocessor/pp_include.cpp"
    "${PROJECT_SOURCE_DIR}/src/frontend/preprocessor/pp_macro_expand.cpp"
    "${PROJECT_SOURCE_DIR}/src/frontend/preprocessor/pp_pragma.cpp"
    "${PROJECT_SOURCE_DIR}/src/frontend/preprocessor/pp_predefined.cpp"
    "${PROJECT_SOURCE_DIR}/src/frontend/parser/parser_core.cpp"
    "${PROJECT_SOURCE_DIR}/src/frontend/parser/parser_types_template.cpp"
    "${PROJECT_SOURCE_DIR}/src/frontend/parser/parser_types_declarator.cpp"
    "${PROJECT_SOURCE_DIR}/src/frontend/parser/parser_types_base.cpp"
    "${PROJECT_SOURCE_DIR}/src/frontend/parser/parser_types_struct.cpp"
    "${PROJECT_SOURCE_DIR}/src/frontend/parser/parser_expressions.cpp"
    "${PROJECT_SOURCE_DIR}/src/frontend/parser/parser_statements.cpp"
    "${PROJECT_SOURCE_DIR}/src/frontend/parser/parser_declarations.cpp"
    "${PROJECT_SOURCE_DIR}/src/frontend/parser/parser_support.cpp"
    "${PROJECT_SOURCE_DIR}/src/frontend/hir/hir_lowering_core.cpp"
    "${PROJECT_SOURCE_DIR}/src/frontend/hir/hir_build.cpp"
    "${PROJECT_SOURCE_DIR}/src/frontend/hir/hir.cpp"
    "${PROJECT_SOURCE_DIR}/src/frontend/hir/hir_expr.cpp"
    "${PROJECT_SOURCE_DIR}/src/frontend/hir/hir_expr_scalar_control.cpp"
    "${PROJECT_SOURCE_DIR}/src/frontend/hir/hir_expr_builtin.cpp"
    "${PROJECT_SOURCE_DIR}/src/frontend/hir/hir_expr_call.cpp"
    "${PROJECT_SOURCE_DIR}/src/frontend/hir/hir_expr_object.cpp"
    "${PROJECT_SOURCE_DIR}/src/frontend/hir/hir_expr_operator.cpp"
    "${PROJECT_SOURCE_DIR}/src/frontend/hir/hir_functions.cpp"
    "${PROJECT_SOURCE_DIR}/src/frontend/hir/compile_time_engine.cpp"
    "${PROJECT_SOURCE_DIR}/src/frontend/hir/hir_printer.cpp"
    "${PROJECT_SOURCE_DIR}/src/frontend/hir/hir_stmt.cpp"
    "${PROJECT_SOURCE_DIR}/src/frontend/hir/hir_stmt_decl.cpp"
    "${PROJECT_SOURCE_DIR}/src/frontend/hir/hir_stmt_control_flow.cpp"
    "${PROJECT_SOURCE_DIR}/src/frontend/hir/hir_stmt_range_for.cpp"
    "${PROJECT_SOURCE_DIR}/src/frontend/hir/hir_stmt_switch.cpp"
    "${PROJECT_SOURCE_DIR}/src/frontend/hir/hir_templates.cpp"
    "${PROJECT_SOURCE_DIR}/src/frontend/hir/hir_templates_deduction.cpp"
    "${PROJECT_SOURCE_DIR}/src/frontend/hir/hir_templates_deferred_nttp.cpp"
    "${PROJECT_SOURCE_DIR}/src/frontend/hir/hir_templates_global.cpp"
    "${PROJECT_SOURCE_DIR}/src/frontend/hir/hir_templates_materialization.cpp"
    "${PROJECT_SOURCE_DIR}/src/frontend/hir/hir_templates_member_typedef.cpp"
    "${PROJECT_SOURCE_DIR}/src/frontend/hir/hir_templates_struct_instantiation.cpp"
    "${PROJECT_SOURCE_DIR}/src/frontend/hir/hir_templates_type_resolution.cpp"
    "${PROJECT_SOURCE_DIR}/src/frontend/hir/hir_templates_value_args.cpp"
    "${PROJECT_SOURCE_DIR}/src/frontend/hir/hir_types.cpp"
    "${PROJECT_SOURCE_DIR}/src/frontend/hir/inline_expand.cpp"
    "${PROJECT_SOURCE_DIR}/src/codegen/llvm/calling_convention.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/target.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/state.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/traits.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/liveness.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/regalloc.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/generation.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/asm_expr.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/asm_preprocess.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/call_abi.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/cast.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/common.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/elf/archive.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/elf/mod.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/elf_writer_common.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/f128_softfloat.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/inline_asm.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/linker_common/archive.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/linker_common/args.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/linker_common/check.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/linker_common/dynamic.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/linker_common/dynstr.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/linker_common/eh_frame.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/linker_common/gc_sections.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/linker_common/hash.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/linker_common/merge.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/linker_common/parse_object.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/linker_common/parse_shared.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/linker_common/resolve_lib.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/linker_common/section_map.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/linker_common/symbols.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/linker_common/types.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/linker_common/write.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/linker_common/mod.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/mod.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/peephole_common.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/stack_layout/mod.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/stack_layout/alloca_coalescing.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/stack_layout/analysis.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/stack_layout/regalloc_helpers.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/stack_layout/slot_assignment.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/x86_common.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/bir.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/bir_printer.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/bir_validate.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/lowering/call_decode.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/lowering/lir_to_bir.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/lowering/lir_to_bir/aggregates.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/lowering/lir_to_bir/calls.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/lowering/lir_to_bir/cfg.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/lowering/lir_to_bir/memory.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/lowering/lir_to_bir/phi.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/lowering/lir_to_bir/types.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/x86/mod.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/x86/assembler/mod.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/x86/assembler/parser.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/x86/assembler/encoder/mod.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/x86/assembler/elf_writer.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/x86/codegen/mod.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/x86/codegen/shared_call_support.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/x86/codegen/emit.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/x86/codegen/calls.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/x86/codegen/alu.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/x86/codegen/cast_ops.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/x86/codegen/f128.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/x86/codegen/float_ops.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/x86/codegen/comparison.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/x86/codegen/i128_ops.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/x86/codegen/intrinsics.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/x86/codegen/inline_asm.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/x86/codegen/memory.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/x86/codegen/asm_emitter.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/x86/codegen/globals.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/x86/codegen/returns.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/x86/codegen/peephole/mod.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/x86/codegen/peephole/types.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/x86/codegen/peephole/passes/helpers.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/x86/codegen/peephole/passes/local_patterns.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/x86/codegen/peephole/passes/compare_branch.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/x86/codegen/peephole/passes/copy_propagation.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/x86/codegen/peephole/passes/dead_code.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/x86/codegen/peephole/passes/callee_saves.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/x86/codegen/peephole/passes/frame_compact.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/x86/codegen/peephole/passes/loop_trampoline.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/x86/codegen/peephole/passes/memory_fold.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/x86/codegen/peephole/passes/push_pop.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/x86/codegen/peephole/passes/store_forwarding.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/x86/codegen/peephole/passes/tail_call.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/x86/codegen/peephole/passes/mod.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/x86/linker/mod.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/x86/linker/elf.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/x86/linker/input.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/aarch64/mod.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/aarch64/assembler/mod.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/aarch64/assembler/encoder/mod.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/aarch64/assembler/encoder/bitfield.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/aarch64/assembler/encoder/compare_branch.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/aarch64/assembler/encoder/data_processing.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/aarch64/assembler/encoder/fp_scalar.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/aarch64/assembler/encoder/load_store.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/aarch64/assembler/encoder/neon.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/aarch64/assembler/encoder/system.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/aarch64/assembler/elf_writer.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/aarch64/assembler/parser.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/aarch64/codegen/mod.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/aarch64/codegen/alu.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/aarch64/codegen/asm_emitter.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/aarch64/codegen/atomics.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/aarch64/codegen/cast_ops.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/aarch64/codegen/calls.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/aarch64/codegen/comparison.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/aarch64/codegen/f128.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/aarch64/codegen/float_ops.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/aarch64/codegen/globals.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/aarch64/codegen/i128_ops.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/aarch64/codegen/inline_asm.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/aarch64/codegen/intrinsics.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/aarch64/codegen/memory.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/aarch64/codegen/peephole.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/aarch64/codegen/prologue.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/aarch64/codegen/returns.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/aarch64/codegen/variadic.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/aarch64/codegen/emit.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/aarch64/linker/mod.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/aarch64/linker/elf.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/aarch64/linker/types.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/aarch64/linker/input.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/aarch64/linker/reloc.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/aarch64/linker/plt_got.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/aarch64/linker/link.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/aarch64/linker/emit_dynamic.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/aarch64/linker/emit_shared.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/aarch64/linker/emit_static.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/riscv/mod.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/riscv/assembler/mod.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/riscv/assembler/parser.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/riscv/assembler/elf_writer.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/riscv/assembler/compress.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/riscv/codegen/mod.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/riscv/codegen/emit.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/riscv/codegen/atomics.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/riscv/codegen/alu.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/riscv/codegen/calls.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/riscv/codegen/cast_ops.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/riscv/codegen/comparison.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/riscv/codegen/f128.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/riscv/codegen/float_ops.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/riscv/codegen/globals.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/riscv/codegen/i128_ops.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/riscv/codegen/intrinsics.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/riscv/codegen/memory.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/riscv/codegen/returns.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/riscv/codegen/prologue.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/riscv/codegen/variadic.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/riscv/codegen/asm_emitter.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/riscv/codegen/peephole.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/backend.cpp"
    "${PROJECT_SOURCE_DIR}/src/codegen/llvm/llvm_codegen.cpp"
    "${PROJECT_SOURCE_DIR}/src/codegen/lir/stmt_emitter_core.cpp"
    "${PROJECT_SOURCE_DIR}/src/codegen/lir/stmt_emitter_types.cpp"
    "${PROJECT_SOURCE_DIR}/src/codegen/lir/stmt_emitter_lvalue.cpp"
    "${PROJECT_SOURCE_DIR}/src/codegen/lir/stmt_emitter_expr.cpp"
    "${PROJECT_SOURCE_DIR}/src/codegen/lir/stmt_emitter_expr_binary.cpp"
    "${PROJECT_SOURCE_DIR}/src/codegen/lir/stmt_emitter_expr_misc.cpp"
    "${PROJECT_SOURCE_DIR}/src/codegen/lir/stmt_emitter_vaarg.cpp"
    "${PROJECT_SOURCE_DIR}/src/codegen/lir/stmt_emitter_call_builtin.cpp"
    "${PROJECT_SOURCE_DIR}/src/codegen/lir/stmt_emitter_call_args.cpp"
    "${PROJECT_SOURCE_DIR}/src/codegen/lir/stmt_emitter_call_target.cpp"
    "${PROJECT_SOURCE_DIR}/src/codegen/lir/stmt_emitter_call.cpp"
    "${PROJECT_SOURCE_DIR}/src/codegen/lir/stmt_emitter_call_vaarg_amd64.cpp"
    "${PROJECT_SOURCE_DIR}/src/codegen/lir/stmt_emitter_call_vaarg_amd64_registers.cpp"
    "${PROJECT_SOURCE_DIR}/src/codegen/lir/stmt_emitter_stmt.cpp"
    "${PROJECT_SOURCE_DIR}/src/codegen/lir/const_init_emitter.cpp"
    "${PROJECT_SOURCE_DIR}/src/codegen/lir/hir_to_lir.cpp"
    "${PROJECT_SOURCE_DIR}/src/codegen/lir/lir_printer.cpp"
    "${PROJECT_SOURCE_DIR}/src/codegen/lir/verify.cpp"
)

set(BACKEND_LOWERING_BOOTSTRAP_SRCS
    "${PROJECT_SOURCE_DIR}/src/backend/target.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/backend.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/bir.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/bir_printer.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/bir_validate.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/lowering/call_decode.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/lowering/lir_to_bir.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/lowering/lir_to_bir_context.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/lowering/lir_to_bir_analysis.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/lowering/lir_to_bir_module.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/prepare/prepare.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/prepare/stack_layout.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/prepare/liveness.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/prepare/regalloc.cpp"
)

# Backend reboot bootstrap:
# keep only the new top-level backend driver, lir_to_bir skeleton, and prepare
# skeleton in buildable source sets while the broader backend/codegen tree is
# being redesigned.
list(FILTER FRONTEND_CXX_COMMON_SRCS EXCLUDE REGEX "/src/backend/")
list(APPEND FRONTEND_CXX_COMMON_SRCS
     ${BACKEND_LOWERING_BOOTSTRAP_SRCS})

set(FRONTEND_CXX_INCLUDE_DIRS
    "${PROJECT_SOURCE_DIR}/src/frontend"
    "${PROJECT_SOURCE_DIR}/src/frontend/lexer"
    "${PROJECT_SOURCE_DIR}/src/frontend/parser"
    "${PROJECT_SOURCE_DIR}/src/frontend/sema"
    "${PROJECT_SOURCE_DIR}/src/frontend/hir"
    "${PROJECT_SOURCE_DIR}/src/frontend/preprocessor"
    "${PROJECT_SOURCE_DIR}/src/backend"
    "${PROJECT_SOURCE_DIR}/src/backend/aarch64"
    "${PROJECT_SOURCE_DIR}/src/codegen/llvm"
    "${PROJECT_SOURCE_DIR}/src/codegen/lir"
)

add_executable(c4cll
    "${PROJECT_SOURCE_DIR}/src/apps/c4cll.cpp"
    ${FRONTEND_CXX_COMMON_SRCS}
)
target_include_directories(c4cll PRIVATE
    ${FRONTEND_CXX_INCLUDE_DIRS}
)
target_compile_definitions(c4cll PRIVATE
    ENABLE_HEAVY_TENTATIVE_SNAPSHOT=$<IF:$<BOOL:${ENABLE_HEAVY_TENTATIVE_SNAPSHOT}>,1,0>
)
set_target_properties(c4cll PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

add_executable(frontend_cxx_preprocessor_tests
    "${PROJECT_SOURCE_DIR}/tests/preprocessor/frontend_cxx_preprocessor_tests.cpp"
    "${PROJECT_SOURCE_DIR}/src/frontend/preprocessor/preprocessor.cpp"
    "${PROJECT_SOURCE_DIR}/src/frontend/preprocessor/pp_text.cpp"
    "${PROJECT_SOURCE_DIR}/src/frontend/preprocessor/pp_cond.cpp"
    "${PROJECT_SOURCE_DIR}/src/frontend/preprocessor/pp_include.cpp"
    "${PROJECT_SOURCE_DIR}/src/frontend/preprocessor/pp_macro_expand.cpp"
    "${PROJECT_SOURCE_DIR}/src/frontend/preprocessor/pp_pragma.cpp"
    "${PROJECT_SOURCE_DIR}/src/frontend/preprocessor/pp_predefined.cpp"
)
target_include_directories(frontend_cxx_preprocessor_tests PRIVATE
    "${PROJECT_SOURCE_DIR}/src/frontend"
    "${PROJECT_SOURCE_DIR}/src/frontend/preprocessor"
)
set_target_properties(frontend_cxx_preprocessor_tests PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

add_executable(frontend_hir_tests
    "${PROJECT_SOURCE_DIR}/tests/frontend/frontend_hir_tests.cpp"
)
target_include_directories(frontend_hir_tests PRIVATE
    ${FRONTEND_CXX_INCLUDE_DIRS}
)
set_target_properties(frontend_hir_tests PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

add_custom_command(
  OUTPUT "${EXAMPLE_LL}"
  COMMAND $<TARGET_FILE:c4cll> "${EXAMPLE_C}" -o "${EXAMPLE_LL}"
  DEPENDS c4cll "${EXAMPLE_C}"
  COMMENT "Generating example LLVM IR"
  VERBATIM
)

add_custom_target(build_example_ir ALL DEPENDS "${EXAMPLE_LL}")

if(CLANG_EXECUTABLE)
  set(EXAMPLE_BIN "${CMAKE_BINARY_DIR}/example_bin")
  add_custom_command(
    OUTPUT "${EXAMPLE_BIN}"
    COMMAND "${CLANG_EXECUTABLE}" "${EXAMPLE_LL}" -o "${EXAMPLE_BIN}"
    DEPENDS "${EXAMPLE_LL}"
    COMMENT "Building example executable from LLVM IR"
    VERBATIM
  )
  add_custom_target(build_example_bin DEPENDS "${EXAMPLE_BIN}")

  add_custom_target(eastl_type_traits_simple_workflow
    COMMAND "${CMAKE_COMMAND}"
            -DCOMPILER=$<TARGET_FILE:c4cll>
            -DCLANG=${CLANG_EXECUTABLE}
            -DCXX_COMPILER=${CMAKE_CXX_COMPILER}
            -DSRC=${PROJECT_SOURCE_DIR}/tests/cpp/eastl/eastl_type_traits_simple.cpp
            -DROOT=${PROJECT_SOURCE_DIR}
            -DBINARY_DIR=${CMAKE_BINARY_DIR}
            -P "${PROJECT_SOURCE_DIR}/tests/cpp/eastl/run_eastl_type_traits_simple_workflow.cmake"
    DEPENDS c4cll
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
    COMMENT "Run standalone EASTL type_traits workflow"
    VERBATIM
  )

  add_custom_target(eastl_vector_simple_parse_recipe
    COMMAND "${CMAKE_COMMAND}"
            -DCOMPILER=$<TARGET_FILE:c4cll>
            -DROOT=${PROJECT_SOURCE_DIR}
            -DSRC=${PROJECT_SOURCE_DIR}/tests/cpp/eastl/eastl_vector_simple.cpp
            "-DEXPECT_FAIL_LOC:STRING=ref/EASTL/include/EASTL/internal/type_properties.h:35:52"
            "-DEXPECT_ERROR_SUBSTRING:STRING=parse_fn=try_parse_record_typedef_member phase=committed expected=SEMI got='('"
            -P "${PROJECT_SOURCE_DIR}/tests/cpp/eastl/run_eastl_vector_parse_recipe.cmake"
    DEPENDS c4cll
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
    COMMENT "Run standalone EASTL vector parse recipe"
    VERBATIM
  )

  add_custom_target(std_vector_simple_parse_baseline
    COMMAND "${CMAKE_COMMAND}"
            -DCOMPILER=$<TARGET_FILE:c4cll>
            -DSRC=${PROJECT_SOURCE_DIR}/tests/cpp/std/std_vector_simple.cpp
            -DEXPECT_TIMEOUT_SEC=60
            -P "${PROJECT_SOURCE_DIR}/tests/cpp/std/run_std_vector_parse_baseline.cmake"
    DEPENDS c4cll
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
    COMMENT "Run standalone std::vector parse baseline"
    VERBATIM
  )
endif()

include("${PROJECT_SOURCE_DIR}/tests/TestEntry.cmake")
