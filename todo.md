Status: Active
Source Idea Path: ideas/open/292_reopen_286_288_match_load_local_memory_admission.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Reproduce and classify the reopened blocker

# Current Packet

## Just Finished

Step 1 complete: reproduced and classified the reopened 286/288 CLI blocker
from the supervisor-provided `test_after.log`.

Exact failing command:
`cmake --build --preset default && ctest --test-dir build -R '^(backend_cli_dump_bir_00204_stdarg_movi_zext_immediate_fold|backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication)$' --output-on-failure`

Failing tests:
- `backend_cli_dump_bir_00204_stdarg_movi_zext_immediate_fold`
- `backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication`

Classification:
- Failure family: semantic BIR `load local-memory semantic family`.
- Latest function: `match`.
- Reported source fixture: `tests/c/external/c-testsuite/src/00204.c`.
- First implementation target: trace `LirLoadOp` through
  `src/backend/bir/lir_to_bir/memory/coordinator.cpp::lower_scalar_or_local_memory_inst`
  into `src/backend/bir/lir_to_bir/memory/local_slots.cpp::lower_memory_load_inst`,
  then classify the rejected local-memory load shape before changing admission.
- Boundary conclusion: blocker is external to idea 291's rendered call-argument
  ABI suffix-parser boundary. The reproduced failure is in semantic local-memory
  load admission, not call-argument suffix parsing or `alignstack(...)`
  authority.

## Suggested Next

Execute Step 2 from `plan.md`: trace the `match` semantic local-memory load
route and identify whether the missing/ignored fact is type, slot, pointer,
aggregate-layout, or prepared metadata.

## Watchouts

- Closed idea 286 originally failed in `myprintf`; the reopened failure now
  reports latest function `match`. Avoid replaying a named `myprintf` fix.
- Do not use named-case logic for `00204.c`, `match`, `myprintf`, `movi`, or
  HFA struct spellings.
- Do not reintroduce rendered call-argument `alignstack(...)` parsing as
  semantic authority; idea 291's structured metadata precedence must remain
  fenced.
- Do not close idea 291 until this blocker is resolved and its close-time proof
  can pass.

## Proof

Supervisor-provided proof log used without rerun per packet:
`test_after.log`.

Command captured there:
`cmake --build --preset default && ctest --test-dir build -R '^(backend_cli_dump_bir_00204_stdarg_movi_zext_immediate_fold|backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication)$' --output-on-failure`

Result: build was up to date, both selected CTest cases failed with
`latest function failure: semantic lir_to_bir function 'match' failed in load local-memory semantic family`.
