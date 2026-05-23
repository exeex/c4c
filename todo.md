Status: Active
Source Idea Path: ideas/open/386_aarch64_calls_mechanical_split.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Map Calls Dependencies

# Current Packet

## Just Finished

Completed Step 1 - Map Calls Dependencies for
`src/backend/mir/aarch64/codegen/calls.cpp` using AST-backed queries.

Selected first extraction cluster: `calls_common.cpp/hpp`.

Rationale: the common helper block is the least-risk first mechanical packet
because its representative callees are standard-library, ABI, prepared-call, or
same-cluster helpers, while later clusters depend on it. Moving it first creates
the shared calls implementation header needed by later extractions without
pulling move lowering, byval lane publication, preservation analysis, effects,
or printing logic across file boundaries yet.

Key AST query evidence from `test_after.log`:

- Signature inventory found `calls.cpp` at 7193 lines and mapped the source
  idea's proposed clusters: common helpers near lines 73-307, argument-source
  helpers near lines 326-1815, preservation near lines 1856-2607, byval and
  aggregate publication near lines 693-854 plus 990-1448 and 2750-3297, move
  lowering near lines 2548-5545, effects near lines 5890-6160, and printing
  near lines 6170-7179.
- Common cluster: `function-callers align_to` returned
  `fixed_frame_adjustment_bytes`, `incoming_stack_argument_size_bytes`,
  `named_incoming_stack_bytes`, `outgoing_stack_argument_bytes`, and
  `outgoing_stack_argument_size_bytes`; `function-callees
  outgoing_stack_argument_bytes` returned only `align_to` and
  `outgoing_stack_argument_size_bytes`.
- Common fanout: `function-callers append_call_diagnostic` returned
  `lower_after_call_move`, `lower_before_call_immediate_binding`,
  `lower_before_call_move`, `lower_prepared_call_instruction`,
  `make_f128_q_register_operand_from_carrier`,
  `make_indirect_callee_register`,
  `make_register_operand_from_prepared_authority`, and
  `require_prepared_call_plan`.
- Common-to-effects/arguments fanout: `function-callers
  prepared_clobber_expected_view` returned
  `effect_from_prepared_call_clobber` and
  `effect_from_prepared_call_preserved_value`; `function-callers
  register_class_from_bank` returned those two effects helpers plus
  `make_register_operand_from_prepared_authority`.
- Argument-source cluster: `function-callees
  make_frame_slot_call_argument_source` returned `find_frame_slot_by_id` and
  `find_prepared_addressing`, and `function-callers
  make_frame_slot_call_argument_source` returned `lower_before_call_move`.
  `make_aggregate_call_argument_source` has no direct callees in this
  translation unit and is called by `lower_before_call_move`.
- Byval/aggregate cluster: `function-callees
  make_fragmented_aggregate_register_lane_publication_instruction` returned
  `fragmented_aggregate_register_lane_publication_lines`,
  `make_call_boundary_machine_instruction`, `make_memory_operand`,
  `make_register_operand`, and `effect_from_operand`; its direct caller is
  `lower_before_call_move`.
- Preservation cluster: `function-callees
  find_prior_preserved_value_for_call_argument` returned
  `find_latest_prior_preserved_value_by_position` and
  `find_prepared_call_plans`; its direct caller is
  `make_prior_preserved_call_argument_source`.
- Move-lowering cluster: `function-callees lower_before_call_move` returned a
  broad cross-cluster dependency set including argument-source, byval,
  preservation, f128, common diagnostic, and instruction-construction helpers;
  its direct caller is `lower_before_call_moves`.
- Effects and printing clusters are not good first packets: `effect_from_prepared_call_clobber`
  depends on common register-class/view helpers, and
  `print_call_boundary_move` depends on printing helpers plus aggregate
  publication, frame-slot materialization, scalar view, ABI, and machine-printer
  helpers.

Awkward dependencies to preserve for the first code packet:

- `append_call_diagnostic` has high fanout and should be declared in the common
  header rather than duplicated or left private to `calls.cpp`.
- `register_class_from_bank`, `prepared_clobber_expected_view`, and
  `occupied_register_views` are common inputs for both argument-source and
  effects work; moving effects first would force common declarations anyway.
- `fixed_frame_adjustment_bytes` and `va_start_overflow_area_stack_offset` are
  common-sized helpers but are reached from `lower_prepared_call_instruction`;
  keep their behavior and ordering mechanical.
- There are overloaded/similarly named scalar helpers across argument and
  printing clusters, especially `scalar_fp_register_view`; the first extraction
  should avoid broad renames and declare only the moved common signatures.

## Suggested Next

Execute Step 2 by mechanically extracting the common cluster into
`calls_common.cpp/hpp`: `align_to`,
`incoming_stack_argument_size_bytes`,
`incoming_stack_argument_alignment_bytes`,
`outgoing_stack_argument_size_bytes`, `outgoing_stack_argument_bytes`,
`outgoing_stack_argument_base_register`, `entry_param_uses_incoming_stack`,
`named_incoming_stack_bytes`, `function_has_call`,
`fixed_frame_adjustment_bytes`, `va_start_overflow_area_stack_offset`,
`register_class_from_bank`, `register_display_name`,
`occupied_register_views`, `prepared_clobber_expected_view`, and
`append_call_diagnostic`.

## Watchouts

- Unrelated dirty implementation/test files were already present when this
  lifecycle state was activated; do not revert or sweep them into a lifecycle
  commit.
- Keep this idea behavior-preserving and reject testcase-shaped shortcuts,
  expectation downgrades, or backend capability changes.
- `calls.cpp` was already dirty during this mapping packet; the AST evidence
  reflects the current worktree content.
- The first code extraction will need a normal implementation header for shared
  common helpers; do not solve linkage by duplicating helper bodies in later
  clusters.

## Proof

Mapping proof recorded in `test_after.log`:

```bash
command -v c4c-clang-tool c4c-clang-tool-ccdb
c4c-clang-tool-ccdb function-signatures /workspaces/c4c/src/backend/mir/aarch64/codegen/calls.cpp build/compile_commands.json
c4c-clang-tool-ccdb function-callers /workspaces/c4c/src/backend/mir/aarch64/codegen/calls.cpp align_to build/compile_commands.json
c4c-clang-tool-ccdb function-callees /workspaces/c4c/src/backend/mir/aarch64/codegen/calls.cpp outgoing_stack_argument_bytes build/compile_commands.json
c4c-clang-tool-ccdb function-callers /workspaces/c4c/src/backend/mir/aarch64/codegen/calls.cpp append_call_diagnostic build/compile_commands.json
c4c-clang-tool-ccdb function-callees /workspaces/c4c/src/backend/mir/aarch64/codegen/calls.cpp make_frame_slot_call_argument_source build/compile_commands.json
c4c-clang-tool-ccdb function-callers /workspaces/c4c/src/backend/mir/aarch64/codegen/calls.cpp make_frame_slot_call_argument_source build/compile_commands.json
c4c-clang-tool-ccdb function-callees /workspaces/c4c/src/backend/mir/aarch64/codegen/calls.cpp make_aggregate_call_argument_source build/compile_commands.json
c4c-clang-tool-ccdb function-callers /workspaces/c4c/src/backend/mir/aarch64/codegen/calls.cpp make_aggregate_call_argument_source build/compile_commands.json
c4c-clang-tool-ccdb function-callees /workspaces/c4c/src/backend/mir/aarch64/codegen/calls.cpp make_fragmented_aggregate_register_lane_publication_instruction build/compile_commands.json
c4c-clang-tool-ccdb function-callers /workspaces/c4c/src/backend/mir/aarch64/codegen/calls.cpp make_fragmented_aggregate_register_lane_publication_instruction build/compile_commands.json
c4c-clang-tool-ccdb function-callees /workspaces/c4c/src/backend/mir/aarch64/codegen/calls.cpp find_prior_preserved_value_for_call_argument build/compile_commands.json
c4c-clang-tool-ccdb function-callers /workspaces/c4c/src/backend/mir/aarch64/codegen/calls.cpp find_prior_preserved_value_for_call_argument build/compile_commands.json
c4c-clang-tool-ccdb function-callees /workspaces/c4c/src/backend/mir/aarch64/codegen/calls.cpp lower_before_call_move build/compile_commands.json
c4c-clang-tool-ccdb function-callers /workspaces/c4c/src/backend/mir/aarch64/codegen/calls.cpp lower_before_call_move build/compile_commands.json
c4c-clang-tool-ccdb function-callees /workspaces/c4c/src/backend/mir/aarch64/codegen/calls.cpp effect_from_prepared_call_clobber build/compile_commands.json
c4c-clang-tool-ccdb function-callers /workspaces/c4c/src/backend/mir/aarch64/codegen/calls.cpp effect_from_prepared_call_clobber build/compile_commands.json
c4c-clang-tool-ccdb function-callees /workspaces/c4c/src/backend/mir/aarch64/codegen/calls.cpp print_call_boundary_move build/compile_commands.json
```

One extra exploratory `function-callers print_call_boundary_move` query in
`test_after.log` returned `target not called in this translation unit`; that is
dependency evidence for printing dispatch shape, not a blocker for Step 1.

Future code extraction proof command:

```bash
cmake --build build --target c4cll backend_aarch64_instruction_dispatch_test -j10
ctest --test-dir build -R '^(backend_aarch64_instruction_dispatch|backend_lir_to_bir_notes|c_testsuite_aarch64_backend_src_00204_c)$' --output-on-failure
```
