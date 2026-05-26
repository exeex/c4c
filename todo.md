Status: Active
Source Idea Path: ideas/open/09_calls_preservation_republication_retirement.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Prepare Preservation Source And Destination Facts

# Current Packet

## Just Finished

Step 2 - Prepare Preservation Source And Destination Facts completed the AArch64
endpoint-consumption packet.

`src/backend/mir/aarch64/codegen/calls_moves.cpp` now prefers prepared
preservation endpoints when lowering:

- callee-saved preservation home population source registers or stack sources
  from `PreparedCallBoundaryEffectPlan::source`
- preservation home population destination registers from
  `PreparedCallBoundaryEffectPlan::destination`
- prior stack-preserved reload memory sources from
  `PreparedCallPreservedValue::preservation_destination`

The legacy `PreparedValueHome` lookup and preserved-value stack fields remain
as compatibility fallbacks for older/manual fixtures that do not populate the
new endpoint facts.

`backend_aarch64_instruction_dispatch_test` now proves preservation home
population consumes explicit source/destination endpoints instead of the
legacy source home or preserved register fields, and proves a later
before-instruction reload consumes the stack destination endpoint offset
instead of the preserved-value legacy stack offset.

Changed files:

- `src/backend/mir/aarch64/codegen/calls_moves.cpp`
- `tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp`
- `todo.md`

## Suggested Next

Next Step 2 or Step 3 packet: retire the remaining compatibility fallbacks only
after all manual fixtures and non-AArch64 consumers publish complete prepared
preservation endpoints, or move on to prepared post-call republication facts if
the supervisor wants endpoint consumption landed first.

## Watchouts

AArch64 still has compatibility fallback paths through `find_value_home` and
legacy `PreparedCallPreservedValue` stack fields. This packet intentionally did
not remove them. Stack-slot preservation home population into memory still
flows through existing explicit stack move lowering because
`CallBoundaryMoveInstructionRecord` has no destination-memory field.

## Proof

Ran exactly:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_instruction_dispatch|backend_call_boundary_effect_plan|backend_aarch64_target_instruction_records)$'`

Result: passed. Proof log: `test_after.log`.
