Status: Active
Source Idea Path: ideas/open/652_prepared_incoming_stack_formal_authority.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Validate And Review Boundaries

# Current Packet

## Just Finished

Step 4 tightened RV64 consumer-boundary validation for prepared incoming stack
formal authority.

Focused coverage:
`tests/backend/mir/backend_riscv_object_emission_test.cpp` now includes a
negative duplicate-formal branch case. The branch consumer rejects ambiguous
matching prepared formal-publication plans instead of selecting the first
matching formal. Positive explicit-authority branch and `LoadLocal` coverage
remain intact.

## Suggested Next

Supervisor review/acceptance for Step 4. If accepted, compare
`test_before.log` and `test_after.log`, roll the focused proof forward, and
decide whether idea 652 is ready for lifecycle close review.

## Watchouts

- Do not reintroduce RV64 helpers that compute incoming offsets by applying
  ABI size/alignment policy in the consumer. The current RV64 loops over
  formals only to find one matching prepared formal-publication plan and now
  fail closed on duplicate matches.
- Do not use `PreparedValueHome::offset_bytes` as incoming authority; it is
  still the callee local home offset.
- Incoming stack formal loads in functions that save `ra` must use the
  established call-frame size as the incoming-stack base; local home validation
  still uses local frame-slot offsets.
- The duplicate branch-formal case rejects through the existing RV64
  terminator fallback diagnostic; missing explicit authority remains covered by
  the dedicated `unsupported_param_home` admission diagnostic.

## Proof

Validation command written to `test_after.log`:
`rm -f test_after.log && (cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_riscv_object_emission|backend_prepare_frame_stack_call_contract|backend_prepared_lookup_helper|backend_prealloc_formal_publications|backend_prealloc_call_boundary_classification|backend_prepared_object_consumer_contract|backend_call_boundary_effect_plan|backend_x86_prepared_decoded_home_storage)$' && ALLOWLIST=build/agent_state/644_step1_20001017_1.allowlist BUILD_DIR=build scripts/check_progress_rv64_gcc_c_torture_backend.sh) > test_after.log 2>&1`

Result: PASS. The 8/8 focused CTests passed, and
`src/20001017-1.c` passed the allowlisted RV64 backend object progress check.
