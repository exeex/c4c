# AArch64 Memory Frame-Slot Address Materialization Owner

## Goal

Split the AArch64 memory-local frame-slot/address materialization helpers into
a narrow owner while preserving memory lowering's authority over selected
memory operands, AArch64 address spelling, base policy, offset folding, and
scratch materialization.

## Why This Exists

The memory owner subresponsibility audit found a concrete local boundary around
frame-slot lookup/address formatting, fixed-slot base policy, offset folding,
and materialization lines. That boundary survived comparison with idea 70 only
because it remains AArch64-memory-local and consumes prepared
address/value-home/storage authority instead of recreating it.

## In Scope

- Create a focused AArch64 memory-local owner for frame-slot address
  materialization helpers currently clustered in `memory.cpp` and `memory.hpp`.
- Keep the owner responsible for frame-slot lookup consumption, base selection
  policy, encodable-offset folding, textual address formatting, and emitted
  address-materialization lines.
- Preserve existing prepared authority inputs such as prepared stack layout,
  prepared memory access, value-home/storage facts, and frame-slot offsets.
- Keep the public surface no wider than needed by existing memory lowering and
  memory-backed transport consumers.
- Prove equivalent lowering for local frame-slot load/store forms, prepared
  frame-slot memory operands, materialized frame-slot addresses, and fixed-slot
  frame-pointer policy.

## Out Of Scope

- Moving AArch64 offset legality, scratch selection, register spelling, or
  memory opcode choice into shared BIR or prealloc authority.
- Recreating local source-authority, value-home, storage-plan, stack-source, or
  frame-offset decisions that prepared owners already provide.
- Moving `va_list` field addressing or cursor update logic into this owner.
- Creating a broad mechanical `memory.cpp` split whose proof is only line-count
  reduction.
- Reopening dispatch publication, edge-copy, or prepared-wrapper contraction
  decisions from ideas 80, 81, or 84.

## Acceptance And Proof Expectations

- The implementation introduces a narrow AArch64 memory-local owner with a
  bounded header/source surface and updates memory consumers without changing
  selected memory semantics.
- Focused proof covers frame-slot local loads/stores, prepared frame-slot
  operand construction, materialized address emission, fixed-slot base policy,
  and any memory-backed f128/variadic consumer touched by the split.
- Build proof includes at least the backend targets affected by AArch64 memory
  lowering.
- Acceptance-sized slices should use matching before/after regression logs when
  the change crosses more than the frame-slot/address helper boundary.

## Closure Note

Closed on 2026-06-02 after the active runbook completed Steps 1-5. The final
audit confirmed the owner surface is narrow, `memory.hpp` no longer exposes the
frame-slot helper cluster, f128 and variadic consumers remain consumers of
prepared facts rather than owners of ABI or cursor policy, and generated
records/diagnostics proof passed for the acceptance subset.

Close guard:

```sh
cmake --build build --target backend_aarch64_memory_operand_records_test backend_aarch64_prepared_memory_operand_records_test backend_aarch64_memory_operand_contract_test backend_aarch64_machine_printer_test backend_aarch64_instruction_dispatch_test backend_prepared_lookup_helper_test c4cll && ctest --test-dir build -R '^(backend_aarch64_(memory_operand_records|prepared_memory_operand_records|memory_operand_contract|machine_printer|instruction_dispatch)|backend_codegen_route_aarch64_(variadic_aggregate_overflow_byte_copy|dynamic_stack_fixed_slot_uses_fp_anchor)|backend_prepared_lookup_helper)$' --output-on-failure > test_after.log
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed
```

Result: 8/8 before, 8/8 after, no new failures, close guard passed.

## Reviewer Reject Signals

- The route re-derives prepared value homes, storage decisions, stack-source
  authority, or frame offsets under a new frame-slot helper name.
- AArch64 offset legality, scratch choice, register spelling, or address text
  formatting is moved into shared prealloc/BIR without a target-neutral fact
  boundary.
- `va_list` field addressing is swept into this owner without separate ABI
  proof.
- The slice claims capability progress through helper renames, file movement,
  or weaker expectations while generated memory records and diagnostics are not
  proven equivalent.
- Tests are narrowed to one named frame-slot case while nearby immediate-offset,
  materialized-offset, fixed-slot, and prepared-address forms remain
  unexamined.
