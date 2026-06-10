Status: Active
Source Idea Path: ideas/open/167_route1_producer_constant_oracle_thinning.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Contract Only Proven-Private Surface

# Current Packet

## Just Finished

Completed Step 3: migrated the selected AArch64 GP scalar
value-publication materialization consumer in
`src/backend/mir/aarch64/codegen/dispatch_value_materialization.cpp` from
prepared same-block producer and named integer-constant discovery to the
Route 1-backed MIR query helpers.

The selected path now uses:
- `mir::find_same_block_scalar_producer(...)` for named same-block producer
  discovery.
- `mir::evaluate_same_block_integer_constant(...)` for named integer-constant
  discovery.

Immediate values remain a direct fast path. Unsupported, missing, and
non-materializable select-chain cases still fail closed into the existing home
fallback path rather than visiting unsupported BIR instructions.

## Suggested Next

Execute Step 4: re-scan direct consumers before contracting any prepared
surface. Limit the first contraction packet to evidence-backed surfaces made
private by this selected migration, or report that no contraction is safe yet.

## Watchouts

- `dispatch_value_materialization.cpp` no longer calls the prepared same-block
  producer or prepared integer-constant helpers directly.
- Do not move homes, registers, storage, emitted-register state, operand views,
  frame slots, final instruction order, spill/reload behavior, or publication
  hooks into BIR.
- Reject testcase-shaped shortcuts, expectation rewrites, or narrow named-case
  matching as progress.
- Residual prepared consumers in the selected file are still intentional:
  prepared homes, prepared memory accesses, local-load offsets, global-load
  emission policy, select-chain materialization, publication helpers, and a
  local prepared-shaped adapter for existing AArch64 target-policy APIs.
- The local adapter reshapes a MIR producer answer for the existing global-load
  access helper and ALU scratch-hazard callback type; it does not call prepared
  same-block producer discovery.
- Do not contract `PreparedFunctionLookups` or
  `edge_publication_source_producers` until a Step 4 consumer scan proves no
  remaining public consumers need them.

## Proof

Ran the supervisor-selected proof command:
`bash -lc "set -o pipefail; cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prepared_lookup_helper|backend_aarch64_instruction_dispatch)$'" |& tee test_after.log`

Result: passed. `backend_prepared_lookup_helper` and
`backend_aarch64_instruction_dispatch` both passed.

Supervisor-side broader backend validation:
`ctest --test-dir build -j --output-on-failure -R '^backend_'`

Result: passed 179/179.

Proof log: `test_after.log`.
