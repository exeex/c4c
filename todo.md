Status: Active
Source Idea Path: ideas/open/557_bir_local_memory_semantic_producer_admission.md
Source Plan Path: plan.md
Current Step ID: 8
Current Step Title: Prove One Representative Backend-Object Row

# Current Packet

## Just Finished

Step 8 - Prove One Representative Backend-Object Row completed as an
RV64 proof/log-inspection packet for `src/20001026-1.c`.

The delegated backend-object proof rebuilt successfully, then failed the
single-row RV64 scan. The refreshed representative log
`build/rv64_gcc_c_torture_backend/src_20001026-1.c/case.log` still reports
semantic `lir_to_bir` failure in function `build_real_from_int_cst_1` in the
`store local-memory` semantic family.

Result classification: the row remained blocked at the prior store
local-memory admission point. It did not advance to runtime comparison,
downstream prepared-object behavior, or a clearly new in-scope local-memory
boundary.

## Suggested Next

Proceed with a narrow diagnostic packet for the remaining `store local-memory`
admission failure in `build_real_from_int_cst_1`, starting from the semantic
`lir_to_bir` failure path rather than RV64 or prepared-object inference.

## Watchouts

Reject target exclusions, testcase/helper-name shaped rules, expectation
rewrites, unsupported-marker changes, allowlist changes, and RV64/MIR inference.
Do not infer prepared/route compatibility from the RV64 row alone.

The `src/20001026-1.c` representative is still failing at the same admitted
family label as before Step 8, so the next packet needs a local semantic
admission diagnosis before any expectation, allowlist, runtime comparison, or
prepared-object route work.

Remaining neighboring representative families are still intentionally left for
later packets: load
`src/20000314-1.c`, GEP `src/20000717-4.c`, scalar/local-memory
`src/20000519-1.c`, and alloca `src/20050604-1.c`.

## Proof

`cmake --build --preset default && ALLOWLIST=build/agent_state/557_step8_20001026.allowlist VERBOSE_FAILURES=1 scripts/check_progress_rv64_gcc_c_torture_backend.sh`

Result: failed as an expected-useful Step 8 proof result, `0/1` RV64
backend-object rows passed. Proof log: `test_after.log`. Case log inspected:
`build/rv64_gcc_c_torture_backend/src_20001026-1.c/case.log`.
