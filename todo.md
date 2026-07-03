Status: Active
Source Idea Path: ideas/open/569_prepared_move_bundle_ambiguous_multi_source_stack_destination.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Representative RV64 Rerun And Owner Disposition

# Current Packet

## Just Finished

Step 4 reran representative
`tests/c/external/gcc_torture/src/20001026-1.c` through focused RV64 dump and
object-route commands after the Step 3 prepared classifier repair.

Old-vs-new representative result:

- Old Step 1 object-route result:
  `prepared_consumer_category=ambiguous_non_parallel_multi_source_stack_destination`
  with the message `prepared move-bundle classifier rejected ambiguous
  non-parallel multi-source stack-destination authority`.
- New Step 4 object-route result: the old prepared consumer category and
  `ambiguous_non_parallel_multi_source_stack_destination` text are gone.
- New object-route exit mode: exit 1 with
  `prepared module shape: unsupported_instruction_fragment: BIR instruction
  requires unsupported RV64 object lowering`.

Next-owner classification:

- The representative has crossed the prepared move-bundle classifier boundary.
- The remaining failure is a separate RV64 object-lowering owner reported only
  as generic `unsupported_instruction_fragment`; it is not the 569 prepared
  classifier owner and should remain separate from the generic diagnostics work
  tracked by idea 570.
- This rerun does not prove integer div/rem lowering, runtime behavior, or
  broader RV64 object-route progress.

Acceptance evaluation:

- Step 3 focused tests prove the intended prepared contract: the
  select-materialization multi-source stack-destination shape is accepted with
  no diagnostic while unrelated simpler ambiguous bundles still fail closed.
- Step 4 representative evidence proves the old prepared classifier diagnostic
  no longer blocks `src/20001026-1.c`.
- No runtime expectation, allowlist, unsupported marker, implementation file,
  test file, idea, or plan change was made in this Step 4 packet.

Evidence:

- `build/agent_state/569_step4_representative_disposition/disposition_summary.md`
- `build/agent_state/569_step4_representative_disposition/commands.sh.txt`
- `build/agent_state/569_step4_representative_disposition/dump-bir.txt`
- `build/agent_state/569_step4_representative_disposition/dump-prepared-bir.txt`
- `build/agent_state/569_step4_representative_disposition/dump-mir.txt`
- `build/agent_state/569_step4_representative_disposition/object-route.log`

## Suggested Next

Ask the plan owner to decide whether active idea 569 is complete under the
plan lifecycle. The prepared classifier acceptance criteria appear satisfied;
the remaining representative blocker is a separate generic RV64
`unsupported_instruction_fragment` owner and should not be folded into 569.

## Watchouts

- Do not treat this as integer div/rem lowering until the prepared classifier boundary is crossed.
- The old prepared classifier diagnostic is gone, but the representative still
  does not compile through the RV64 object route.
- The current next failure is generic `unsupported_instruction_fragment`, not
  a runtime mismatch, timeout, segfault, test infrastructure problem, or
  prepared classifier failure.
- Do not change runtime comparison, expected output, unsupported markers, or allowlists.
- Do not special-case `src/20001026-1.c` by name.
- Keep generic `unsupported_instruction_fragment` rows in idea 570, not this active plan.

## Proof

Step 4 proof:

- `cmake --build build --target c4cll`
- Focused `src/20001026-1.c` dump/object commands recorded in
  `build/agent_state/569_step4_representative_disposition/commands.sh.txt`.
- Return-code files:
  `dump-bir.rc`, `dump-prepared-bir.rc`, `dump-mir.rc`, `object-route.rc`.
- `git diff --check -- todo.md`
- `scripts/plan_review_state.py show`
