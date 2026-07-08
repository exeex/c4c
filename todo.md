Status: Active
Source Idea Path: ideas/open/620_prepared_mixed_object_data_slots.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory mixed object-data fact gap

# Current Packet

## Just Finished

Lifecycle split from `608`: Step 6 evidence showed current prepared object-data
facts cannot safely represent mixed ordinary bytes plus relocation slots. The
incomplete executor code experiment was not accepted as implementation
progress.

## Suggested Next

Execute Step 1 evidence-first. Capture prepared facts and diagnostics for
`src/20010924-1.c` plus at least two neighboring selected object-data rows,
then identify the first missing prepared fact needed before implementation:
schema, producer population, verifier checks, or a smaller prerequisite.

## Watchouts

- Do not route RV64 relocation-record emission, byte emission, symbol
  materialization, or access-width policy into this plan.
- Do not mark mixed aggregate object data coherent without prepared emitted
  byte spans plus relocation slot offsets and target identity.
- Preserve `608` parked evidence for prepared global memory facts and direct
  global-symbol base-plus-offset authority; do not repeat helper-only
  `ByteStorageAggregate` publication as progress.
- Preserve relocation-only object-data progress from `608`, including the
  `src/921110-1.c` move to the RV64 relocation-record consumer stop.
- Treat `src/20010924-1.c` as a representative, not a named-case shortcut.

## Proof

Lifecycle-only transition. No code proof required for this packet. The previous
Step 6 proof command was:
`{ cmake --build --preset default && ALLOWLIST=build/agent_state/608_step6_mixed_object_data.allowlist scripts/check_progress_rv64_gcc_c_torture_backend.sh; } > test_after.log 2>&1`

That proof did not reach the allowlist because the build process was killed
while compiling `tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp`;
observed case logs during the attempt remained at the prepared selected
object-data contract stop.
