Status: Active
Source Idea Path: ideas/open/643_rv64_scalar_call_boundary_freshness_after_call.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement One Complete-Authority Freshness Path

# Current Packet

## Just Finished

Step 3 implemented a complete-authority RV64 scalar freshness consumer path for
prepared scalar values used after an intervening call.

Changed files:
- `src/backend/prealloc/call_plans.cpp`
- `src/backend/mir/riscv/codegen/prepared_emit_context.hpp`
- `src/backend/mir/riscv/codegen/prepared_function_emit.cpp`
- `src/backend/mir/riscv/codegen/prepared_scalar_emit.hpp`
- `src/backend/mir/riscv/codegen/prepared_scalar_emit.cpp`
- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `tests/backend/mir/backend_riscv_object_emission_test.cpp`

Completed work:
- Prepared call plans now publish call-result source registers as clobbered
  registers, so a call returning through a GPR invalidates stale direct homes in
  that physical result register.
- RV64 prepared text emission and object emission now carry block/instruction
  position into scalar value materialization.
- RV64 scalar move, compare, compare/trunc publication, and generic encoded
  binary fallback paths now reject a direct register home after a same-block
  clobbering call unless a prior preservation fact exists.
- The positive authority path consumes prior scalar preservation facts: a
  preserved callee-saved register is moved from that register, and a preserved
  stack-slot scalar is loaded from the preserved stack slot.
- Fail-closed behavior is preserved when no preservation, republication, or
  rematerialization authority exists.
- Added focused Step 3 tests:
  `builds_prepared_post_call_scalar_compare_from_prior_preserved_register`
  proves a post-call scalar compare consumes explicit prior callee-saved
  preservation authority, and
  `rejects_post_call_scalar_compare_without_freshness_authority` proves the
  same shape fails closed without that authority.

## Suggested Next

Next packet should add the producer-side preservation/republication authority
for register-homed scalar formals or live scalar values that are used after a
call, then rerun the same focused proof to move `src/ipa-sra-2.c` past the
current fail-closed diagnostic.

## Watchouts

- Reject named-case fixes for `src/ipa-sra-2.c`, `calloc`, `foo`, `argc`, or
  physical register `a0`; the repair family is generic scalar freshness across
  call boundaries.
- Do not treat a pre-call register home as fresh after a call without explicit
  preservation, republication, or rematerialization authority.
- `src/ipa-sra-2.c` now fails before object creation with
  `unsupported_scalar_compare_publication`; there is no refreshed `c4c.bin`
  because the stale object emission is rejected.
- Prepared dump still shows `%p.argc` value id `6` homed in `register a0` and
  no `preserve value=%p.argc` fact, so the remaining owner is producer-side
  preservation/republication/rematerialization authority for that scalar.

## Proof

Ran the delegated proof command exactly:

`rm -f test_after.log && (cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_riscv_object_emission|backend_prepare_frame_stack_call_contract|backend_prepared_lookup_helper|backend_prealloc_call_boundary_classification|backend_prepared_object_consumer_contract)$' && ALLOWLIST=build/agent_state/643_step1_ipa_sra_2.allowlist BUILD_DIR=build scripts/check_progress_rv64_gcc_c_torture_backend.sh) > test_after.log 2>&1`

Result after adding focused tests: nonzero due only to the focused torture row.
All five focused CTests passed, including the new positive and negative
`backend_riscv_object_emission` Step 3 scalar freshness rows.
`src/ipa-sra-2.c` now fails with `[RV64_C4C_OBJ_COMPILE_FAIL]` and
`unsupported_scalar_compare_publication: RV64 object route requires prepared
scalar compare result homes and materializable operands`, instead of producing
an object that reads post-call `a0`.

Proof log: `test_after.log`.
