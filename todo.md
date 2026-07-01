Status: Active
Source Idea Path: ideas/open/504_select_publication_move_bundle_evidence_authority.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Split Any Coherent RV64 Consumer

# Current Packet

## Just Finished

Step 3 reran the three previously ambiguous gcc-torture rows with the newly
published select-publication diagnostic and classified the resulting evidence
without implementation changes.

Two rows are real select-publication evidence but are not ready for a final
RV64 consumer packet:
`src/builtin-constant.c` fails as
`intent_status_unsupported_source_home` for a stack-source to register
publication, and `src/pr58726.c` fails as
`intent_status_unsupported_destination_home` for a register-source to stack
publication. `src/pr37924.c` is no longer a select-publication row under the
new diagnostic; it is a generic out-of-SSA `phi_join_immediate_materialization`
move-bundle failure outside idea 504.

Durable evidence was written under
`build/agent_state/504_step3_select_publication_consumer_classification/`.

## Suggested Next

Ask the plan owner to record residual disposition for idea 504. No coherent
final RV64 select-publication consumer packet is ready from these three rows;
future work should first decide whether to open a lower intent-support packet
for select-publication stack-source and stack-destination publications. The
`src/pr37924.c` row should route with generic out-of-SSA/immediate move
materialization work, not idea 504.

## Watchouts

- Do not re-open before-instruction, out-of-SSA, or before-return
  materialization under idea 504.
- Do not lower select-publication move bundles in RV64 from this Step 3
  classification; it found lower intent-support gaps, not a ready consumer.
- Do not infer authority from source names, absent case-log tokens, raw BIR
  shape, object output, final registers, or target behavior.
- The new diagnostic is evidence, not admission. The existing boolean
  select-publication predicate still controls fail-closed behavior.
- Preserve the split between the two select-publication evidence rows and the
  generic `src/pr37924.c` out-of-SSA immediate-materialization row.
- Preserve existing untracked review artifacts and the rejected
  `test_baseline.new.log`.

## Proof

Delegated Step 3 proof:

```sh
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_before.log --allow-non-decreasing-passed > test_after.log 2>&1 && git diff --check >> test_after.log 2>&1
```

Result: passed. Output preserved in `test_after.log`.
