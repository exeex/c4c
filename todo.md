Status: Active
Source Idea Path: ideas/open/623_rv64_cast_instruction_fragment_consumers.md
Source Plan Path: plan.md
Current Step ID: 7
Current Step Title: Implement only a classified multi-row cast-consumer cause

# Current Packet

## Just Finished

Step 7 attempted the selected
`rv64-consumer:width-preserving-zext-i32-to-i32` compile-time `CastInst`
consumer packet. The missing path was not another direct CastInst GPR-copy
fragment: the existing GPR-copy helper already handles register-to-register
same-width i32 `ZExt`/`Trunc`. The 18-row zext family was instead blocked when
prepared before-instruction move bundles had already materialized the same-width
zext operand/result transfer across supported GPR register/stack homes, after
which traversal still visited the no-op `CastInst` and required a second
fragment.

Implemented a fail-closed same-width i32 `ZExt` suppression for that exact
prepared move-bundle shape in
`src/backend/mir/riscv/codegen/object_emission.cpp`, plus focused RV64
object-emission coverage in
`tests/backend/mir/backend_riscv_object_emission_test.cpp`. The existing
fail-closed tests still reject unsupported opcode/type/home shapes when the
matching materializing move bundle is absent.

The supervisor-corrected 18-row proof no longer reports any
`instruction_kind=CastInst` first failures. The corrected tag summary is:

- `PASS=2`
- `RV64_BACKEND_RUNTIME_MISMATCH=6`
- `RV64_C4C_LINK_FAIL=2`
- `RV64_C4C_OBJ_COMPILE_FAIL=8`

The remaining instruction-kind split is no longer on the selected cast
consumer: `instruction_kind=CastInst` remaining is 0, and 4 remaining object
compile rows are at `instruction_kind=CallInst`.

## Suggested Next

Supervisor should treat Step 7's zext CastInst consumer slice as implemented
and decide the next route from the corrected proof split. The remaining rows
are now runtime mismatches, C4C link failures, and non-cast object compile
failures, including the 4 `CallInst` object compile rows.

## Watchouts

- The zext suppression is intentionally narrower than the older GPR-copy helper:
  it only accepts same-width i32 `ZExt` with an exact before-instruction move
  bundle from the cast operand value id to the cast result value id, using
  ordinary consumer register/stack move reasons and no ABI, parallel-copy,
  immediate, cycle-temp, authority, or width complications.
- Trunc rows were not widened in this packet.
- Runtime mismatch rows are proof-visible but outside this cast-consumer
  packet.
- The remaining object compile rows are non-cast route boundaries; do not fold
  `CallInst` or other non-cast lowering work into Step 7.
- Do not use expectation, unsupported-marker, allowlist, timeout, accounting,
  trunc/runtime, non-cast guard, or named-case-only changes as progress.

## Proof

Focused local validation:

- `cmake --build build --target backend_riscv_object_emission_test`
- `./build/tests/backend/mir/backend_riscv_object_emission_test`

Ran the exact delegated Step 7 proof command with absolute `SRC` paths and
preserved output in `test_after.log`. The build step completed and relinked
`c4cll`; all 18 rows were rerun into
`build/agent_state/623_step7_zext_probe/`.

Summary from `test_after.log` and
`build/agent_state/623_step7_zext_probe_summary.tsv`:

- `PASS=2`
- `RV64_BACKEND_RUNTIME_MISMATCH=6`
- `RV64_C4C_LINK_FAIL=2`
- `RV64_C4C_OBJ_COMPILE_FAIL=8`
- `instruction_kind=CastInst` rows remaining: 0
- `instruction_kind=CallInst` object compile rows remaining: 4
