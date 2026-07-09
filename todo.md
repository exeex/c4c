Status: Active
Source Idea Path: ideas/open/643_rv64_scalar_call_boundary_freshness_after_call.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Validate Boundaries

# Current Packet

## Just Finished

Step 4 recorded the supervisor-run broader structural validation for active
idea 643 after the Step 3 commits `9b0ff362f` and `6c65143d3`.

Changed files:
- `todo.md`

Completed work:
- Accepted validation scope was the 22-test structural backend CTest subset
  covering prepare, prealloc, prepared, RV64 object emission, call-boundary
  effect planning, publication records, store-source publication plans, and
  object-model records.
- The focused RV64 torture probe for `src/ipa-sra-2.c` passed under the
  existing Step 1 allowlist.
- No implementation, expectation, allowlist, unsupported-marker, or pass/fail
  accounting changes were made in this packet.

## Suggested Next

Step 5 should be final lifecycle review and plan-owner close decision.

## Watchouts

- Reject named-case fixes for `src/ipa-sra-2.c`, `calloc`, `foo`, `argc`, or
  physical register `a0`; the repair family is generic scalar freshness across
  call boundaries.
- Do not treat a pre-call register home as fresh after a call without explicit
  preservation, republication, or rematerialization authority.
- A broader `ctest -L backend` attempt was excluded from canonical proof because
  it hit 17 existing RV64 route/runtime residual failures outside this accepted
  validation scope.

## Proof

Supervisor ran the accepted Step 4 proof command:

`rm -f test_after.log && (cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_(prepare|prealloc|prepared|riscv_object_emission|call_boundary_effect_plan|publication_plan_record|store_source_publication_plan|object_model_records|rv64_roundtrip_contract))' && ALLOWLIST=build/agent_state/643_step1_ipa_sra_2.allowlist BUILD_DIR=build scripts/check_progress_rv64_gcc_c_torture_backend.sh) > test_after.log 2>&1`

Result: pass. The default build was up to date, all 22 structural backend CTests
passed, and the one-row RV64 torture probe passed for `src/ipa-sra-2.c`.

Proof log: `test_after.log`.
