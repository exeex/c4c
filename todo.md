Status: Active
Source Idea Path: ideas/open/252_aarch64_calls_markdown_shard_implementation_redistribution.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Reconcile `calls.md` And Close Shard Ownership

# Current Packet

## Just Finished

Completed Step 5 `calls.md` reconciliation. The compiled calls owner already
accounts for the valid durable call shard content through prepared call-plan
requirements, direct and indirect `CallInstructionRecord` lowering and
printing, call-boundary move and ABI-binding records, memory-return snapshots,
structured F128 register/constant carriers, preserved-value and clobber
effects, and variadic entry helper records. Deleted
`src/backend/mir/aarch64/codegen/calls.md` after reconciling the legacy entry
points, ABI/stack/register/call/result notes, dependencies, hidden
assumptions, known failure risks, and rebuild guidance.

## Suggested Next

Proceed to Step 6 behavior-preservation and ownership-boundary proof for the
completed redistribution.

## Watchouts

- The former `calls.md` historical ABI configuration, stack-area computation,
  stack-argument copy sequencing, local reclassification, and final cleanup
  arithmetic are not current compiled calls ownership. The durable boundary is
  that later lowering must consume prepared call plans and fail closed on
  missing carriers instead of recovering policy from legacy text.
- Remaining carrier/runtime-helper boundaries from the shard are explicitly
  deferred outside this packet: complete outgoing call-area and call-time
  stack-alignment carriers, indirect-callee spill-area ownership, target ABI
  classification carriers for HFA/F128/i128/aggregate-splitting edge cases
  where prepared plans are insufficient, explicit special-register role records
  for `x8`/`x16`/`x17` when ABI helpers and prepared facts are insufficient,
  and runtime-helper call resource carriers for F128 or soft-float helper paths.
- Runtime-helper printers that emit helper `bl` lines remain
  runtime-helper-family behavior, not calls-owned direct/indirect call
  spelling, until a separate helper-call resource carrier makes that boundary
  machine-verifiable.
- `calls.cpp` now remains the compiled owner for call-boundary records and
  call spelling; `calls.md` has no remaining durable ownership role.

## Proof

Proof passed with log at `test_after.log`:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`.
The final run passed 139/139 `backend_` tests.
