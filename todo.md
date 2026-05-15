Status: Active
Source Idea Path: ideas/open/238_aarch64_atomic_machine_nodes.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Select Atomic RMW And Compare-Exchange Loops

# Current Packet

## Just Finished

Step 4 `Select Atomic RMW And Compare-Exchange Loops` added structured AArch64
selection for atomic RMW and compare-exchange operations from complete prepared
carriers.

Changed behavior:
- AArch64 dispatch now consumes complete `PreparedAtomicOperationCarrier`
  records for RMW and compare-exchange selection in the same structured atomic
  record family as load/store/fence.
- RMW selected records preserve exclusive retry-loop authority, old-value
  result semantics, RMW opcode, ordering, width/type, pointer/value/result
  identities, register authority, and atomic read/write side effects.
- Compare-exchange selected records preserve expected and desired operands,
  success ordering, failure ordering, boolean and old-value result modes,
  pointer/result identities, register authority, exclusive retry-loop facts, and
  monitor-clear-on-compare-failure facts.
- Incomplete carriers, unsupported widths/orderings, unsupported RMW opcodes,
  unsupported failure orderings, and unsupported result modes remain
  fail-closed with explicit diagnostics.
- Final assembly printing remains out of scope for this packet.

Added test coverage:
- `backend_aarch64_instruction_dispatch_test` now proves selected RMW records
  carry old-value result semantics and retry-loop facts.
- The same test proves compare-exchange records carry expected/desired
  operands, success/failure orderings, monitor-clear facts, and both boolean and
  old-value result modes.
- The fail-closed coverage now rejects unsupported RMW opcode and unsupported
  compare-exchange failure ordering rather than accepting partial loops.

## Suggested Next

Begin Step 5 `Print AArch64 Atomic Machine Nodes` by emitting final AArch64
assembly only from structured selected atomic load/store/fence/RMW and
compare-exchange records.

## Watchouts

- Do not infer atomic semantics from volatile flags, rendered text, fixed
  scratch-register snippets, or named testcase shortcuts.
- Step 4 intentionally did not add final assembly printing; Step 5 owns printer
  emission now that all selected atomic node families exist.
- Preserve ordinary volatile memory behavior separately from atomic behavior;
  atomic selection must continue to require carrier provenance.
- RMW and compare-exchange records currently model retry-loop and monitor-clear
  facts structurally; Step 5 should consume those fields directly rather than
  reconstructing from rendered text.
- Do not fold intrinsic, inline-assembly, binary128, scalar FP, or i128 behavior
  into this route.

## Proof

Proof command:
`set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'; } 2>&1 | tee test_after.log`

Result: passed, `139/139` backend tests green.

Proof log: `test_after.log`.
