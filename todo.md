# Execution State

Status: Active
Source Idea Path: ideas/open/81_convert_reviewed_x86_codegen_drafts_to_implementation_for_phoenix_rebuild.md
Source Plan Path: plan.md
Current Step ID: 3.3.3
Current Step Title: Finish The Remaining Stack And Local-Slot Observational Holdouts
Plan Review Counter: 1 / 6
# Current Packet

## Just Finished

Completed another step 3.3.3 declaration cleanup packet by moving the
authoritative fixed/home stack-offset query surface and
`build_prepared_module_local_slot_layout(...)` wrappers out of
`x86_codegen.hpp` and into `lowering/frame_lowering.*`, so the transitional
header no longer owns those stack/home query declarations.

## Suggested Next

Continue step 3.3.3 by moving the last remaining local-slot declaration
holdout behind a narrower owner: the inline bounded multi-defined helper still
uses `render_prepared_local_slot_memory_operand_if_supported(...)` from
`x86_codegen.hpp`, so the next packet should re-seam or relocate that inline
path until the transitional header no longer needs the local-slot renderer
declaration.

## Watchouts

- `x86_codegen.hpp` still declares
  `render_prepared_local_slot_memory_operand_if_supported(...)` because
  `render_prepared_bounded_multi_defined_call_lane_body_if_supported(...)`
  remains inline there and still issues local-slot loads/stores directly.
- The generic stack render helpers
  `render_prepared_stack_memory_operand(...)` and
  `render_prepared_stack_address_expr(...)` still remain on the transitional
  header because backend tests and compatibility consumers include that
  surface directly.
- `prepared_fast_path_operands.cpp` still includes `x86_codegen.hpp` for
  non-stack helpers such as register narrowing and the canonical call-bundle
  handoff message.

## Proof

Step 3.3.3 stack/home declaration cleanup packet on
2026-04-23:
`cmake --build --preset default`
`ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`
Backend subset passed (`106/106`). Canonical log paths: `test_before.log`,
`test_after.log`
