Status: Active
Source Idea Path: ideas/open/314_rv64_aggregate_local_subobject_and_byval_flow.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Repair Or Classify Aggregate Copy And Byval Flow

# Current Packet

## Just Finished

Step 4 aggregate copy/byval classification for idea 314.

Classified `src/00140.c` rather than implementing a broad ABI repair.

Probe result:

- `src/00140.c` emits and links, but qemu exits `132` (`Illegal instruction`)
  because emitted assembly is truncated.

First bad mechanism:

- `f1` starts with byval aggregate-copy loads through `%p.f`.
- Prepared value locations put `%p.f` in a stack-slot home:
  `home %p.f value_id=0 kind=stack_slot slot_id=0 offset=0`.
- The current RV64 prepared local-memory emitter supports pointer-value loads
  when the pointer base is in a register, but not when the pointer base is this
  byval stack-slot home. Emission stops after the `f1` prologue.

Additional ABI evidence:

- `main`'s prepared call plan classifies byval argument 0 as
  `aggregate_address` with no ordinary GPR destination, and before-call moves
  include `destination_storage=stack_slot abi_index=0`.
- The current simple prepared call emitter only handles ordinary GPR
  register/immediate arguments and rejects aggregate transport / stack argument
  forms.
- The byval aggregate contains a `float` lane (`struct foo::v`), so repairing
  the full case would fold in floating aggregate lane behavior.

Classification:

- `separate_byval_aggregate_call_abi_residual`
- This packet does not implement the repair because it would require
  coordinated RV64 prepared byval aggregate call ABI support across callee
  byval stack-slot pointer handling, caller aggregate-address/byval payload
  transport, and floating aggregate lane handling.

Step 4 probe artifacts:

- `build/rv64_c_testsuite_probe_latest/triage_314_step4/probe_results.tsv`
- `build/rv64_c_testsuite_probe_latest/triage_314_step4/summary.md`

## Suggested Next

Hand Step 4 classification to the supervisor/plan owner. A future repair
should be an explicit RV64 prepared byval aggregate call ABI packet, not a
continuation of this narrow aggregate-local subobject emission slice.

## Watchouts

- Do not special-case filenames, fixed offsets, or struct/union names.
- Do not reopen generic local frame-slot address publication unless aggregate
  evidence proves it is still incomplete.
- Do not repair `src/00140.c` by only teaching the callee to read `%p.f` from a
  stack-slot home; caller-side aggregate-address/byval payload transport is
  also missing from the simple prepared call emitter.
- Do not fold vararg/floating aggregate ABI behavior into this Step 4 packet.
- The generated assembly can emit/link while truncated; keep qemu or emission
  completeness checks in any future byval packet.

## Proof

Ran:

- `cmake --build --preset default -j`
- emit/link/qemu probe for `src/00140.c` into `build/rv64_c_testsuite_probe_latest/triage_314_step4/`
- `ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`

Results:

- Build passed.
- No focused tests were added or modified in this classification-only packet.
- `src/00140.c` probe emitted and linked, then qemu exited `132`; classified
  as `separate_byval_aggregate_call_abi_residual`.
- Delegated backend proof wrote `test_after.log` and failed with the existing
  `backend_riscv_prepared_edge_publication` failure as the only failing backend
  test.
