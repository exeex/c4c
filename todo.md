Status: Active
Source Idea Path: ideas/open/652_prepared_incoming_stack_formal_authority.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Wire RV64 To Consume The Explicit Authority

# Current Packet

## Just Finished

Step 3 wired RV64 object-route stack-passed scalar formal consumers to read
`PreparedFormalPublicationPlan::incoming_stack_offset_bytes` from the shared
formal-publication planner.

Consumer path:
RV64 admission now accepts stack-passed scalar formal homes only when the
`IncomingStackToHome` publication is available, while preserving local
home/frame-slot coherence checks as validation. Branch stack-load operands and
`LoadLocal` formal loads use the explicit incoming offset plus the established
callee incoming-stack base; they do not derive that offset from formal order or
`PreparedValueHome::offset_bytes`.

Focused coverage:
`tests/backend/mir/backend_riscv_object_emission_test.cpp` now proves both a
stack-passed formal pointer branch operand and a local-memory formal load use
explicit incoming authority rather than the local home offset, while
local-home-only authority remains fail-closed.

## Suggested Next

Execute Step 4: validate and review the RV64 consumer boundary. Compare the
fresh `test_after.log` against the accepted before log, inspect that the RV64
diff consumes formal-publication authority rather than rebuilding ABI offsets,
and decide whether broader stack-passed formal coverage is needed before
lifecycle review.

## Watchouts

- Do not reintroduce RV64 helpers that compute incoming offsets by applying
  ABI size/alignment policy in the consumer. The current RV64 loops over
  formals only to find the matching prepared formal-publication plan.
- Do not use `PreparedValueHome::offset_bytes` as incoming authority; it is
  still the callee local home offset.
- Incoming stack formal loads in functions that save `ra` must use the
  established call-frame size as the incoming-stack base; local home validation
  still uses local frame-slot offsets.
- `src/20001017-1.c` now passes the allowlisted RV64 backend object progress
  check.

## Proof

Validation command written to `test_after.log`:
`rm -f test_after.log && (cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_riscv_object_emission|backend_prepare_frame_stack_call_contract|backend_prepared_lookup_helper|backend_prealloc_formal_publications|backend_prealloc_call_boundary_classification|backend_prepared_object_consumer_contract|backend_call_boundary_effect_plan|backend_x86_prepared_decoded_home_storage)$' && ALLOWLIST=build/agent_state/644_step1_20001017_1.allowlist BUILD_DIR=build scripts/check_progress_rv64_gcc_c_torture_backend.sh) > test_after.log 2>&1`

Result: PASS. The 8/8 focused CTests passed, and
`src/20001017-1.c` passed the allowlisted RV64 backend object progress check.
