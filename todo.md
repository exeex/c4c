Status: Active
Source Idea Path: ideas/open/aarch64-codegen-03-alu-fallback-operand-phase-extraction.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect Fallback Operand Ownership

# Current Packet

## Just Finished

Step 1 - Inspect Fallback Operand Ownership completed the no-code boundary
inspection around `make_scalar_fallback_operand`.

Selected fallback helper group for extraction:

- `make_scalar_fallback_operand` is the exact phase-local selection boundary.
- Its extracted helper should preserve the current selection order: immediate
  value, already-emitted scalar register, unpublished same-block load-local
  prepared memory source, resolved named scalar operand, prepared scalar load
  source, then final named scalar fallback.
- Keep the helper callable only from the scalar ALU fallback path in
  `lower_scalar_instruction`; it should not become an `alu.hpp` concept for
  this packet series.

Shared dependencies that the fallback selector may call but should not own:

- `make_immediate_scalar_operand`, `find_named_value_home`,
  `find_emitted_scalar_register`, `make_unpublished_load_local_source_operand`,
  `make_named_scalar_operand`, and `make_prepared_scalar_load_source`.
- Generic scalar operand conversion and prepared-memory helpers underneath
  those calls, including `make_resolved_scalar_operand`,
  `make_prepared_scalar_register_operand`, `make_prepared_scalar_load_source`,
  and frame-slot load-source construction.

Helpers that must stay outside extraction:

- `make_control_publication_operand`.
- Control-publication materialization and move helpers:
  `materialize_control_register_source`, `materialize_control_compare_rhs`,
  `materialize_control_binary_result_source`,
  `append_move_control_value_to_register`,
  `append_control_binary_to_register`, `append_control_cast_to_register`, and
  `append_control_value_to_register`.
- Result/publication ownership helpers such as
  `control_prepared_scalar_result_operand`,
  `make_control_publication_assembler`,
  `lower_scalar_compare_publication`, and
  `lower_scalar_select_publication`.

Step 2 can remain inside `alu.cpp`: extract the fallback selector into a
local anonymous-namespace helper boundary in the same implementation unit,
without changing `alu.hpp` or moving shared scalar/control-publication
infrastructure to a new phase-local implementation unit.

## Suggested Next

Delegate Step 2 to do a behavior-preserving in-file extraction in `alu.cpp`:
introduce a small local helper wrapper/group for scalar ALU fallback operand
selection around `make_scalar_fallback_operand`, keep the existing dependency
calls in place, and update only the fallback call sites in
`lower_scalar_instruction` as needed.

## Watchouts

- Keep the slice behavior-preserving and preserve fallback operand selection
  order exactly.
- Do not widen `alu.hpp` with private fallback concepts; Step 2 does not need
  a separate implementation unit.
- Do not fold control-publication operand selection or materialization into the
  fallback extraction. In particular, leave
  `make_control_publication_operand` and all `materialize_control_*` /
  `append_control_*` helpers outside the extracted group.
- Treat any need to touch control-publication materialization as a plan-review
  stop, not as part of Step 2.

## Proof

`git diff --check` passed. No build, tests, logs, or `test_after.log` were
required by the delegated proof for this no-code inspection packet.
