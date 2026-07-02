# Move-Bundle Target-Shape Full Classification

Status: Step 4 full-bucket classification from current per-case logs.

## Inputs

- Row source: `docs/rv64_gcc_torture_post_contract/unsupported_move_bundle_target_shape_rows.tsv`
- Evidence rules: `docs/rv64_gcc_torture_post_contract/move_bundle_target_shape_classification_rules.md`
- Representative pass: `docs/rv64_gcc_torture_post_contract/move_bundle_target_shape_representatives.tsv`
- Derived inspection artifact: `build/agent_state/544_step4_full_classification/diagnostic_context.tsv`

The classifier keeps the Step 1 row order and copies each `case` and `log`
field exactly from the reconstructed bucket. Each row cites the current
per-case `case.log` diagnostic line range in `evidence_ref`.

## Output

The full classification is stored in:

```text
docs/rv64_gcc_torture_post_contract/move_bundle_target_shape_classification.tsv
```

The TSV uses the Step 2 schema:

```text
case	log	first_owner_lane	evidence_ref	diagnostic_key	move_shape	first_missing_fact	notes
```

## Lane Counts

| Lane | Rows |
| --- | ---: |
| `coherent_rv64_mir_materialization` | 151 |
| `prepared_module_target_shape_authority_gap` | 31 |
| `bir_semantic_producer_gap` | 0 |
| `f128_primary_quarantine` | 0 |
| `evidence_gap` | 1 |
| Total | 183 |

No current bucket log matched `f128`, `_Float128`, `TFmode`, or `quad`, so no
row is classified into `f128_primary_quarantine` from the present artifacts.
No row had row-level evidence that the earliest missing fact is semantic BIR
production, so `bir_semantic_producer_gap` remains empty for this pass.

## Classification Notes

Rows classified as `coherent_rv64_mir_materialization` publish concrete move
coordinates, value ids, compatible source and destination homes, scalar types,
and the generic RV64 materialization failure. This includes coherent
register-to-stack, stack-to-stack, and phi register-to-register move bundles.

Rows classified as `prepared_module_target_shape_authority_gap` have visible
semantic intent but current prepared/module publication is missing,
inconsistent, or too weak for RV64 to lower safely. The common reasons are a
stack-to-stack move whose source home is published as a register or immediate,
a return ABI destination whose destination storage and destination home do not
agree, pointer-base-plus-offset homes under stack storage, missing type facts,
or select-publication intent with `unsupported_source_home`.

The single `evidence_gap` row is `src/960209-1.c`; its current log confirms
`unsupported_move_bundle_target_shape` bucket membership but omits event kind,
phase, authority, value ids, homes, types, and F128-screenable detail.

## Derived Artifacts

- `build/agent_state/544_step4_full_classification/diagnostic_context.tsv`
- `build/agent_state/544_step4_full_classification/lane_counts.tsv`
- `build/agent_state/544_step4_full_classification/shape_counts.tsv`

## Reconciliation

Step 4 proof checks that the classification header matches the Step 2 schema,
that the table has exactly 183 data rows, that every row has exactly one
allowed lane, that every `case` appears exactly once, and that the case set
matches the reconstructed row table.

## Step 5 Follow-Up Routing

Step 5 split the classified bucket into durable follow-up queues:

- `ideas/open/551_rv64_move_bundle_materialization_from_classified_bucket.md`
  owns the 151 `coherent_rv64_mir_materialization` rows.
- `ideas/open/552_prepared_move_bundle_target_shape_authority_gaps.md` owns
  the 31 `prepared_module_target_shape_authority_gap` rows.
- `ideas/open/553_move_bundle_target_shape_evidence_gap_src_960209_1.md`
  owns the single `evidence_gap` row, `src/960209-1.c`.

The `bir_semantic_producer_gap` and `f128_primary_quarantine` lanes remain
empty in the current classification. No new BIR or F128 follow-up idea was
created from this bucket; future rows should use the existing producer and
F128 quarantine lanes only if row-level evidence supports that routing.
