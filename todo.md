Status: Active
Source Idea Path: ideas/open/238_aarch64_atomic_machine_nodes.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Select Ordered Loads, Stores, And Fences

# Current Packet

## Just Finished

Step 3 `Select Ordered Loads, Stores, And Fences` added structured AArch64
selection for simple atomic memory operations from complete prepared carriers.

Changed behavior:
- AArch64 dispatch now consumes complete `PreparedAtomicOperationCarrier`
  records for atomic load, atomic store, and non-relaxed fence selection.
- Selected atomic records preserve ordering, width/type, pointer/value/result
  identity, address space, register authority, and acquire/release/seq-cst
  side-effect semantics without printing final assembly.
- Incomplete carriers, unsupported load/store orderings, unsupported widths,
  relaxed fences, RMW, and compare-exchange remain fail-closed with explicit
  diagnostics instead of falling back to ordinary volatile memory records.

Added test coverage:
- `backend_aarch64_instruction_dispatch_test` now proves ordered atomic load,
  ordered atomic store, and seq-cst fence carriers select as atomic-specific
  machine records.
- The same test proves missing/unsupported atomic carriers, RMW, and
  compare-exchange are rejected before selection.

## Suggested Next

Begin Step 4 `Select Atomic RMW And Compare-Exchange Loops` by adding
structured exclusive-access selected records for complete RMW and
compare-exchange carriers while preserving old-value and boolean result modes.

## Watchouts

- Do not infer atomic semantics from volatile flags, rendered text, fixed
  scratch-register snippets, or named testcase shortcuts.
- Step 3 intentionally did not add final assembly printing; Step 5 owns
  printer emission after all selected atomic node families exist.
- RMW and compare-exchange currently diagnose as unsupported operation kinds in
  dispatch. Step 4 should replace only those fail-closed diagnostics with
  structured exclusive-loop records from complete carriers.
- Preserve ordinary volatile memory behavior separately from atomic behavior;
  atomic selection must continue to require carrier provenance.
- Compare-exchange must preserve success ordering, failure ordering, and
  boolean-vs-old-value result mode. RMW must preserve old-value result
  semantics.
- Do not fold intrinsic, inline-assembly, binary128, scalar FP, or i128 behavior
  into this route.

## Proof

Proof command:
`set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'; } 2>&1 | tee test_after.log`

Result: passed, `139/139` backend tests green.

Proof log: `test_after.log`.
