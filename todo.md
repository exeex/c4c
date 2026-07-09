Status: Active
Source Idea Path: ideas/open/627_pointer_stack_result_call_policy.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Reclassify Representative Rows And Close Readiness

# Current Packet

## Just Finished

Completed Step 5 by refreshing the six idea-627 representative rows through
prepared dumps and direct RV64 object-route diagnostics after pointer
stack-result admission.

Evidence directory: `/tmp/c4c627_step5_evidence` (temporary, non-canonical).

Row reclassification:

- `src/20030715-1.c`: moved past the original pointer stack-result authority
  blocker. Prepared facts for `server_type` -> `ap_check_cmd_context` publish
  `source_placement=gpr:call_result#0/w1`, `source_reg=a0`, and
  `destination_storage=stack_slot` at `destination_slot=#8`,
  `dest_stack_offset=56`. Current object-route rejection is
  `unsupported_terminator_fragment`, an out-of-scope terminator/branch
  lowering bucket.
- `src/20011113-1.c`: moved past the original pointer stack-result authority
  blocker. Prepared facts for the `foo` and `baz` calls to `bar` publish
  pointer result stack homes at `slot#29+stack112` and `slot#38+stack112`
  with `source_placement=gpr:call_result#0/w1` from `a0`. Current object-route
  rejection is a before-return `return_stack_to_register` pointer move with
  `destination_kind=function_return_abi`, an out-of-scope return stack-to-GPR
  owner bucket; the row still also contains adjacent outgoing aggregate/byval
  stack transport facts, but that is not the current first object-route
  rejection.
- `src/20041218-1.c`: moved past the original pointer stack-result authority
  blocker. Prepared facts publish several pointer result stack homes, including
  `dummy1` result `slot#29+stack104`, `check` results `slot#55+stack176` and
  `slot#59+stack200`, `bar` result `slot#66+stack64`, and `dummy2` result
  `slot#68+stack80`, all with `source_placement=gpr:call_result#0/w1` from
  `a0`. Current object-route rejection is `unsupported_local_memory_access`,
  an out-of-scope local-memory/addressing owner bucket.
- `src/pr20601-1.c`: moved past the original pointer stack-result authority
  blocker. Prepared facts for `setup1` -> `baz` and `main` -> `setup1`
  publish pointer result stack homes at `slot#22+stack64` and `slot#71+stack4`
  with `source_placement=gpr:call_result#0/w1` from `a0`. Current object-route
  rejection is `unsupported_terminator_fragment`, an out-of-scope
  terminator/branch lowering bucket.
- `src/pr34176.c`: moved past the original pointer stack-result authority
  blocker and direct RV64 object emission now produces an ELF object. Prepared
  facts for `main` -> `foo` publish the pointer result stack home
  `slot#12+stack88` with `source_placement=gpr:call_result#0/w1` from `a0`.
- `src/pr58209.c`: moved past the original pointer stack-result authority
  blocker. Prepared facts for `bar` -> `foo` publish the pointer result stack
  home `slot#3+stack8` with `source_placement=gpr:call_result#0/w1` from `a0`.
  Current object-route rejection is
  `ambiguous_non_parallel_multi_source_stack_destination` /
  `producer_authority_missing_for_register_fan_in_stack_destination`, an
  out-of-scope move-bundle/branch-publication owner bucket.

Close readiness: idea 627 is close-ready. The shared in-scope ordinary
same-module pointer stack-result authority blocker is gone from the
representative rows; remaining failures belong to separate owner buckets and
should not be folded into this idea.

## Suggested Next

Ask the plan owner to close idea 627. If the supervisor wants follow-up work,
split it into separate initiatives by owner bucket rather than extending this
runbook.

## Watchouts

- Do not stretch idea 627 into the remaining owner buckets:
  terminator/branch lowering (`20030715-1.c`, `pr20601-1.c`),
  return stack-to-GPR pointer moves (`20011113-1.c`), local-memory/addressing
  (`20041218-1.c`), or ambiguous register fan-in to one stack destination
  (`pr58209.c`).
- `build/rv64_gcc_c_torture_backend/*/case.log` contained stale
  `unsupported_call_abi` diagnostics before this packet's direct refresh;
  use `/tmp/c4c627_step5_evidence` for this packet's row classification.
- This packet made no implementation, test, expectation, unsupported-marker,
  allowlist, source-idea, or `plan.md` changes.

## Proof

Command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`

Result: passed, 347/347 backend tests. Proof log: `test_after.log`.
