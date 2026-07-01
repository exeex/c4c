Status: Active
Source Idea Path: ideas/open/504_select_publication_move_bundle_evidence_authority.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Publish Or Route Select-Publication Authority

# Current Packet

## Just Finished

Step 2 published narrow structured evidence for the early RV64
select-publication move-bundle rejection path without implementing
select-publication RV64 materialization.

The rejection diagnostic is now keyed by prepared function/block/edge and
destination identity and includes prepared move-bundle fields, parallel-copy
fields, `PreparedEdgePublication` fields, `EdgePublicationMoveIntent` fields,
and an exact rejection reason such as
`intent_status_unsupported_source_home`.

Focused backend coverage now proves the evidence is present and
distinguishable while the representative select-publication move remains
fail-closed. Durable evidence was written under
`build/agent_state/504_step2_select_publication_evidence/`.

## Suggested Next

Execute Step 3 by using the newly published select-publication evidence to
classify the three ambiguous gcc-torture rows. If a coherent supported RV64
consumer subset is visible, split a focused consumer idea/packet for that
subset; otherwise record that no select-publication RV64 consumer is ready yet.

## Watchouts

- Do not re-open before-instruction, out-of-SSA, or before-return
  materialization under idea 504.
- Do not lower select-publication move bundles in RV64 in Step 3 unless the
  plan owner/supervisor explicitly activates a consumer packet from the new
  evidence.
- Do not infer authority from source names, absent case-log tokens, raw BIR
  shape, object output, final registers, or target behavior.
- The new diagnostic is evidence, not admission. The existing boolean
  select-publication predicate still controls fail-closed behavior.
- Preserve existing untracked review artifacts and the rejected
  `test_baseline.new.log`.

## Proof

Delegated Step 2 proof:

```sh
cmake --build build -j2 > test_after.log 2>&1 && ctest --test-dir build -j2 --output-on-failure -R '^backend_' >> test_after.log 2>&1 && git diff --check >> test_after.log 2>&1
```

Result: passed. Output preserved in `test_after.log`.
