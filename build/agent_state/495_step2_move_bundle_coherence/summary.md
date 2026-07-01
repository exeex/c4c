# 495 Step 2 Move-Bundle Coherence Classification

## Input Evidence

- Step 1 evidence directory:
  `build/agent_state/495_step1_move_bundle_bucket/`
- Row source: `parsed_rows.tsv`
- Case logs: the `log` column from Step 1 rows.

Step 2 made no implementation changes. It classified the reproduced
`unsupported_move_bundle_target_shape` bucket by prepared event kind, authority,
parallel-copy status, execution site, destination storage, source storage where
reported, move reason, coherence family, and first owner.

## Durable Artifacts

- `classification.tsv`: one row per reproduced bucket row, enriched with
  execution-site tokens and first-owner/coherence classification.
- `coherence_counts.tsv`: grouped coherence and authority disposition counts.
- `owner_matrix.tsv`: grouped event/authority/storage/reason matrix with first
  owner.
- `representative_classification.tsv`: one representative row per classified
  matrix shape.

## Row Counts

The Step 2 row-level artifact has 423 data rows, preserving the Step 1 bucket
size.

```text
count  coherence_family                              authority_disposition
328    coherent_prepared_authority_unlowered_rv64    complete_ordinary_move_bundle_authority
 91    coherent_prepared_authority_unlowered_rv64    complete_parallel_copy_authority
  3    missing_case_log_detail_select_publication    ambiguous_prepared_authority_until_select_tokens_are_published
  1    coherent_prepared_authority_unlowered_rv64    complete_return_move_bundle_authority
```

There are no contradictory prepared-authority rows in the reproduced bucket.
The 420 ordinary rows carry coherent prepared authority and fail at RV64
materialization. The 3 select-publication rows remain ambiguous because the
current case-log tokens do not report the function/block/event/phase/authority
details for the prepared select-publication move bundle.

## First-Owner Families

```text
278 before_instruction_copies / before_instruction / authority none / stack_slot / consumer_register_to_stack
    first owner: rv64_before_instruction_prepared_move_materialization

 50 before_instruction_copies / before_instruction / authority none / stack_slot / consumer_stack_to_stack
    first owner: rv64_before_instruction_prepared_move_materialization

 62 pre_terminator_copies / block_entry / out_of_ssa_parallel_copy / predecessor_terminator / register / phi_join_register_to_register
    first owner: rv64_out_of_ssa_parallel_copy_move_materialization

 26 pre_terminator_copies / block_entry / out_of_ssa_parallel_copy / predecessor_terminator / register / edge_consumer_preservation_register_to_register
    first owner: rv64_out_of_ssa_parallel_copy_move_materialization

  3 pre_terminator_copies / block_entry / out_of_ssa_parallel_copy / predecessor_terminator / stack_slot / edge_consumer_preservation_register_to_stack
    first owner: rv64_out_of_ssa_parallel_copy_move_materialization

  1 pre_terminator_copies / before_return / authority none / register / return_stack_to_register
    first owner: rv64_before_return_prepared_move_materialization

  3 select-publication move-bundle rows without reported event/phase/authority/storage tokens
    first owner: select_publication_move_bundle_evidence_splitter
```

Source storage is not reported by the current case-log token stream for these
representatives, so `classification.tsv` records `<not_reported>` instead of
inferring a storage class.

## Coherence Split

- Coherent prepared authority, not currently RV64-lowered: 420 rows.
- Missing or ambiguous prepared authority due to absent select-publication case
  tokens: 3 rows.
- Contradictory prepared authority: 0 rows.

The next packet should split Step 3 follow-up work along those owner boundaries:
one RV64 before-instruction move-materialization packet, one RV64 out-of-SSA
parallel-copy materialization packet, one before-return move-materialization
packet, and a select-publication evidence/authority packet before treating those
3 rows as lowerable.

## Proof

Delegated proof command:

```sh
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_before.log --allow-non-decreasing-passed > test_after.log 2>&1 && git diff --check >> test_after.log 2>&1
```

Result: passed. Output preserved in `test_after.log`.
