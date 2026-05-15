Status: Active
Source Idea Path: ideas/open/238_aarch64_atomic_machine_nodes.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Define Structured Atomic Operation Carriers

# Current Packet

## Just Finished

Step 2 `Define Structured Atomic Operation Carriers` added the first
target-neutral/prepared atomic operation carrier boundary.

Changed behavior:
- BIR functions can now carry structured `AtomicOperation` facts before AArch64
  selection. The facts explicitly model kind
  `load/store/fence/rmw/compare_exchange`, success ordering, failure ordering
  for compare-exchange, width/type, pointer/value operands, compare
  expected/desired operands, RMW opcode, result mode, and address space.
- Prepared state now publishes `PreparedAtomicOperationCarrier` records from
  those BIR facts. Complete carriers are marked `complete`; partial carriers
  stay `missing` and accumulate specific missing-fact diagnostics instead of
  falling back to volatile memory or target-local reconstruction.
- Prepared printer debug output now exposes complete atomic carrier facts under
  `--- prepared-atomic-operations ---`; incomplete carriers are not printed as
  usable operation records and only surface missing-fact diagnostics.

Added test coverage:
- `backend_prepared_printer_test` now proves complete load, store, fence, RMW,
  and compare-exchange carrier facts preserve their fields and print only as
  structured prepared atomic records.
- The same test proves incomplete RMW and compare-exchange facts fail closed
  with missing width/value/expected/failure-ordering diagnostics and do not
  print as usable atomic operation records.

## Suggested Next

Begin Step 3 `Select Ordered Loads, Stores, And Fences` by consuming only
complete `PreparedAtomicOperationCarrier` records for ordered load, ordered
store, and fence selection. Keep RMW and compare-exchange selection for Step 4.

## Watchouts

- Do not infer atomic semantics from volatile flags, rendered text, fixed
  scratch-register snippets, or named testcase shortcuts.
- Selection must reject `PreparedAtomicOperationCarrierKind::Missing` and any
  missing function-level atomic carrier table rather than reconstructing facts
  from volatile memory or printer text.
- Step 3 should leave RMW and compare-exchange carriers unselected/fail-closed;
  this packet only established their facts for later Step 4 consumption.
- Preserve ordinary volatile memory behavior separately from atomic behavior.
- RMW and compare-exchange must preserve old-value result semantics when Step 4
  consumes them; compare exchange also carries explicit boolean-vs-old-value
  result mode and failure ordering.
- Do not fold intrinsic, inline-assembly, binary128, scalar FP, or i128 behavior
  into this route.

## Proof

Proof command:
`set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'; } 2>&1 | tee test_after.log`

Result: passed, `139/139` backend tests green.

Proof log: `test_after.log`.
