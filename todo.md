Status: Active
Source Idea Path: ideas/open/bir-agent-index-header-hierarchy.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Survey Current Header And Include Boundaries

# Current Packet

## Just Finished

Completed `plan.md` Step 1, `Survey Current Header And Include Boundaries`.
Surveyed `src/backend/bir/lir_to_bir.hpp`, all current
`src/backend/bir/lir_to_bir/*.cpp` includes, `src/backend/bir/lir_to_bir.cpp`,
and external includes/usages under `src/` and `tests/`.

Exported declarations that should remain in
`src/backend/bir/lir_to_bir.hpp`:

- `BirLoweringOptions`
- `BirLoweringNote`
- `BirLoweringResult`
- `try_lower_to_bir_with_options`
- `try_lower_to_bir`
- `lower_to_bir`

Private lowering declarations that should move to
`src/backend/bir/lir_to_bir/lowering.hpp`:

- `BirLoweringContext` and `make_lowering_context`
- `BirFunctionPreScan`, `BirModuleAnalysis`, `analyze_module`, and
  `lower_module`
- the `lir_to_bir_detail` namespace declarations: `ValueMap`,
  `GlobalAddress`, `GlobalInfo`, `GlobalTypes`, `TypeDeclMap`,
  `FunctionSymbolSet`, local slot/pointer aliases, `ParsedTypedOperand`,
  `AggregateField`, `AggregateTypeLayout`, and helper declarations including
  `build_type_decl_map`, `parse_i64`, `lower_integer_type`,
  `type_size_bytes`, `compute_call_arg_abi`,
  `compute_function_return_abi`, `split_top_level_initializer_items`,
  `parse_typed_operand`, `resolve_index_operand`,
  `compute_aggregate_type_layout`, `lower_minimal_global`,
  `lower_string_constant_global`, `resolve_pointer_initializer_offsets`,
  `resolve_known_global_address`, and `is_known_function_symbol`
- `BirFunctionLowerer`, including its nested public helper structs/aliases,
  private helper declarations, and lowering state fields

External callers/includes reviewed:

- Production public API users: `src/backend/backend.hpp`,
  `src/backend/backend.cpp`, `src/backend/mir/x86/api/api.cpp`, and
  `src/codegen/llvm/llvm_codegen.cpp` use the public lowering options/result
  and entry functions.
- Production private-helper leak: `src/backend/prealloc/regalloc.cpp` calls
  `c4c::backend::lir_to_bir_detail::compute_call_arg_abi`.
- AArch64 handoff include:
  `src/backend/mir/aarch64/codegen/emit.cpp` includes
  `../../../bir/lir_to_bir.hpp`; no private lowering declaration use was found
  in the surveyed opening section.
- Test public API users include the backend handoff, notes, liveness, and
  frontend lowering tests.
- Test private-helper leaks include x86 handoff tests that call
  `lir_to_bir_detail::compute_call_arg_abi` or
  `compute_function_return_abi`.
- `tests/frontend/frontend_hir_tests.cpp` directly uses
  `BirLoweringContext`, `make_lowering_context`, `BirModuleAnalysis`,
  `BirFunctionPreScan`, and `analyze_module`.
- Every current `src/backend/bir/lir_to_bir/*.cpp` file includes
  `../lir_to_bir.hpp`; `src/backend/bir/lir_to_bir.cpp` includes
  `lir_to_bir.hpp` and also uses private helpers/lowerer declarations.

## Suggested Next

Execute `plan.md` Step 2 by adding
`src/backend/bir/lir_to_bir/lowering.hpp`, moving the private declarations
listed above into it, and updating `src/backend/bir/lir_to_bir/*.cpp` plus
`src/backend/bir/lir_to_bir.cpp` to include the private index where they use
lowering internals. Keep `src/backend/bir/lir_to_bir.hpp` focused on the public
options, notes, result type, and entry functions, and handle the current
external `lir_to_bir_detail`/analysis leaks explicitly so the move is
behavior-preserving and compile-clean.

## Watchouts

- This initiative is structural; do not change lowering semantics or testcase
  expectations.
- Do not edit `ideas/open/bir-agent-index-header-hierarchy.md` for routine
  execution notes.
- Do not create one header per implementation file.
- `BirLoweringResult` currently exposes `BirModuleAnalysis`, so Step 2 must
  either keep the analysis result surface intentionally public or adjust the
  result/API shape as an explicit behavior-preserving public-boundary decision.
- Moving `lir_to_bir_detail::compute_call_arg_abi` and
  `compute_function_return_abi` without a replacement public ABI surface will
  break `src/backend/prealloc/regalloc.cpp` and several backend tests.
- Moving `BirLoweringContext`/analysis declarations will require updating or
  rethinking the direct analysis probe in `tests/frontend/frontend_hir_tests.cpp`.

## Proof

No build or test proof required by the survey-only packet. No test logs were
created or modified.
