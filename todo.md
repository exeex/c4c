Status: Active
Source Idea Path: ideas/open/643_rv64_scalar_call_boundary_freshness_after_call.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement One Complete-Authority Freshness Path

# Current Packet

## Just Finished

Step 3 added producer-side complete-authority preservation for RV64
register-homed scalar formals or scalar values consumed after an intervening
call.

Changed files:
- `src/backend/prealloc/call_plans.cpp`
- `tests/backend/bir/backend_prepare_frame_stack_call_contract_test.cpp`
- `todo.md`

Completed work:
- Prepared call plans now synthesize a preservation fact for a clobbered
  direct-register scalar home when a same-block post-call consumer move proves
  the scalar is needed after the call and a complete saved callee register is
  available.
- The route rejects incomplete authority: call-result values are not treated as
  pre-call preservation candidates, and preservation destinations must not
  overlap the current call result destination.
- When the consumer move's preferred register would be overwritten by the call
  result, the producer selects another saved callee register with complete frame
  save authority.
- Added focused positive/negative BIR contract coverage for RV64 formal
  consumer-move preservation and for the no-post-call-consumer case.
- `src/ipa-sra-2.c` now publishes `%p.argc` preservation from `a0` into `s2`
  at the `calloc` boundary and passes the focused RV64 object torture row.

## Suggested Next

Next packet should let the supervisor decide whether Step 3 is acceptance-ready
for broader regression guard validation or whether another adjacent scalar
freshness boundary needs review.

## Watchouts

- Reject named-case fixes for `src/ipa-sra-2.c`, `calloc`, `foo`, `argc`, or
  physical register `a0`; the repair family is generic scalar freshness across
  call boundaries.
- Do not treat a pre-call register home as fresh after a call without explicit
  preservation, republication, or rematerialization authority.
- The producer path is currently same-block consumer-move preservation with a
  complete saved callee-register destination; broader cross-block or stack-slot
  producer paths remain separate route choices.
- The focused torture row passed, so this packet did not leave a new
  `src/ipa-sra-2.c` residual.

## Proof

Ran the delegated proof command exactly:

`rm -f test_after.log && (cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_riscv_object_emission|backend_prepare_frame_stack_call_contract|backend_prepared_lookup_helper|backend_prealloc_call_boundary_classification|backend_prepared_object_consumer_contract)$' && ALLOWLIST=build/agent_state/643_step1_ipa_sra_2.allowlist BUILD_DIR=build scripts/check_progress_rv64_gcc_c_torture_backend.sh) > test_after.log 2>&1`

Result: pass. The default build was up to date, all five focused CTests passed,
and the one-row RV64 torture probe passed for `src/ipa-sra-2.c`.

Proof log: `test_after.log`.
