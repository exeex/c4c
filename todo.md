Status: Active
Source Idea Path: ideas/open/507_rv64_select_publication_stack_home_materialization.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect Final Admission Gate

# Current Packet

## Just Finished

Step 1 inspected the final RV64 select-publication admission gate and emission
path for the stack-home intent fields published by idea 505 without
implementation changes.

The exact final admission surfaces are:

- `prepared_select_publication_move_is_rv64_object_admitted` in
  `src/backend/mir/riscv/codegen/object_emission.cpp`: strict final predicate
  for ordinary select-publication moves. It currently admits only
  register/immediate-to-register intent and rejects `source_stack_offset_bytes`
  and `destination_stack_offset_bytes`.
- `prepared_select_publication_destination_is_stack_home`: existing stack-home
  helper that admits only the legacy 4-byte destination stack-home bypass.
- `rv64_select_publication_bundle_rejection_diagnostic` and
  `prepared_predecessor_select_publication_bundle_is_rv64_object_admitted`:
  the diagnostic and bundle-level gates used before move-bundle emission.
- the object traversal move-bundle branch around `object_emission.cpp:10079`,
  which runs select-publication diagnostic/admission, then the stack-join
  bypass, then `fragment_for_prepared_move_bundle`.

The emission helpers already exist: `append_rv64_load_stack_to_register` can
encode the pointer/XLEN stack-source load, and
`append_rv64_store_register_to_stack` can encode 1/2/4-byte GPR-to-stack
stores. `fragment_for_prepared_move_bundle` also uses these helpers, but it
reconstructs from raw move/value homes and should not be the authority source
for this idea.

Representative rows from idea 505 are ready for final consumer work:

- `src/builtin-constant.c`: available pointer stack-source to GPR intent with
  `source_stack_offset=8`, `source_stack_size=8`, `destination_register=a0`,
  `instruction_text="ld a0, 8(sp)"`; currently fail-closed because the strict
  predicate rejects source-stack intent.
- `src/pr58726.c`: available direct GPR source to i16 stack-destination intent
  with `source_register=t1`, `destination_stack_offset=4`,
  `destination_stack_size=2`, `instruction_text="sh t1, 4(sp)"`; currently
  fail-closed because strict admission rejects stack destinations and the
  stack-home helper only admits the 4-byte legacy route.

## Suggested Next

Execute Step 2 by implementing a consumer-only final RV64 materialization hook
for the supported pointer/XLEN concrete stack-source to GPR select-publication
intent. Consume `EdgePublicationMoveIntent` fields and existing
publication/bundle checks; do not reconstruct authority from raw BIR or generic
move-bundle shape. No new producer facts are needed.

## Watchouts

- Keep `src/pr37924.c` routed to idea 506; it is generic out-of-SSA immediate
  materialization, not select-publication stack-home work.
- Preserve fail-closed behavior for missing/non-available intent, publication
  or bundle mismatch, non-predecessor execution block, cycle/temp/non-move
  steps, missing concrete stack fields, non-GPR register identities, large
  offsets, unsupported widths, and stack-to-stack select-publication without a
  scratch/interleaving policy.
- Do not fold generic immediate materialization from `src/pr37924.c` into idea
  507.
- Do not infer homes or authority from testcase names, raw BIR shape, object
  output, final registers, target behavior, or absent diagnostic tokens.
- Keep stack-to-stack select-publication unsupported without a dedicated
  scratch/interleaving policy.
- Preserve existing untracked review artifacts and the rejected
  `test_baseline.new.log`.

## Proof

Delegated Step 1 proof:

```sh
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_before.log --allow-non-decreasing-passed > test_after.log 2>&1 && git diff --check >> test_after.log 2>&1
```

Result: passed. `test_after.log` reports `result: PASS` with
`passed=328 failed=0 total=328`, followed by a clean `git diff --check`.
