Status: Active
Source Idea Path: ideas/open/544_rv64_move_bundle_target_shape_bucket_split.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Classify Representative Rows

# Current Packet

## Just Finished

Step 3 classified a seven-row representative subset of the reconstructed
`unsupported_move_bundle_target_shape` bucket using the Step 2 evidence rules.
The representative TSV is
`docs/rv64_gcc_torture_post_contract/move_bundle_target_shape_representatives.tsv`,
with an audit note in
`docs/rv64_gcc_torture_post_contract/move_bundle_target_shape_representatives.md`.
The subset covers the observed current diagnostic shapes: before-instruction
consumer register-to-stack, before-instruction consumer stack-to-stack,
before-return return stack-to-register, select-publication phi register-to-register,
select-publication phi stack-to-stack, pointer-style prepared home ambiguity,
and a diagnostic-only evidence gap. Current bucket logs did not expose an
F128-primary representative.

## Suggested Next

Execute Step 4: apply the Step 2 rules to all 183 reconstructed rows and write
`docs/rv64_gcc_torture_post_contract/move_bundle_target_shape_classification.tsv`.

## Watchouts

- Do not implement RV64 lowering or producer repair in this review plan.
- Do not infer ownership from testcase names, target register spellings, or raw
  BIR shape.
- Keep F128-primary rows routed to the existing F128 quarantine lane.
- The representative pass found no F128 tokens in current bucket logs, but the
  full-bucket classifier should still preserve the F128 lane and route any row
  with primary F128 row-level evidence there.
- Rows like `src/960209-1.c` have only a top-level diagnostic in current logs;
  keep those as `evidence_gap` unless Step 4 can cite a stronger row-level
  artifact.
- Evidence-gap is a valid lane when current artifacts cannot prove first owner
  without guessing.

## Proof

Docs-only proof command and TSV schema/count check, preserved in
`test_after.log`:

```sh
git diff --check -- todo.md \
  docs/rv64_gcc_torture_post_contract/move_bundle_target_shape_representatives.tsv \
  docs/rv64_gcc_torture_post_contract/move_bundle_target_shape_representatives.md
awk -F '\t' '
  BEGIN {
    expected = "case\tlog\tfirst_owner_lane\tevidence_ref\tdiagnostic_key\tmove_shape\tfirst_missing_fact\tnotes"
    allowed["coherent_rv64_mir_materialization"] = 1
    allowed["prepared_module_target_shape_authority_gap"] = 1
    allowed["bir_semantic_producer_gap"] = 1
    allowed["f128_primary_quarantine"] = 1
    allowed["evidence_gap"] = 1
  }
  NR == 1 && $0 != expected { print "bad header: " $0; bad = 1 }
  NR > 1 {
    rows++
    if (NF != 8) { print "bad field count at line " NR ": " NF; bad = 1 }
    if (!($3 in allowed)) { print "bad lane at line " NR ": " $3; bad = 1 }
  }
  END {
    print "representative_rows=" rows
    if (rows != 7) { print "bad representative row count: " rows; bad = 1 }
    exit bad
  }
' docs/rv64_gcc_torture_post_contract/move_bundle_target_shape_representatives.tsv
```
