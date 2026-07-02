Status: Active
Source Idea Path: ideas/open/544_rv64_move_bundle_target_shape_bucket_split.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Split Follow-Up Queues

# Current Packet

## Just Finished

Step 5 split the 183 classified `unsupported_move_bundle_target_shape` rows
into durable follow-up queues without implementation or test-contract changes.

Created follow-up ideas:

- `ideas/open/551_rv64_move_bundle_materialization_from_classified_bucket.md`
  for the 151 `coherent_rv64_mir_materialization` rows.
- `ideas/open/552_prepared_move_bundle_target_shape_authority_gaps.md` for
  the 31 `prepared_module_target_shape_authority_gap` rows.
- `ideas/open/553_move_bundle_target_shape_evidence_gap_src_960209_1.md` for
  the single `evidence_gap` row, `src/960209-1.c`.

Updated
`docs/rv64_gcc_torture_post_contract/move_bundle_target_shape_classification.md`
with the Step 5 follow-up routing cross-reference. The
`bir_semantic_producer_gap` and `f128_primary_quarantine` lanes remain empty in
the current classification, so no new BIR or F128 follow-up idea was created
from this bucket.

## Suggested Next

Execute Step 6: close-readiness review for the active splitter runbook. Confirm
the 183-row reconstruction, classification, and follow-up routing are complete;
confirm no implementation, expectation, unsupported-marker, allowlist, or
runtime-comparison changes were made; then decide whether the source idea is
ready to close.

## Watchouts

- Do not implement RV64 lowering, prepared authority repair, or producer
  repair in this review plan.
- Do not infer ownership from testcase names, target register spellings, raw
  BIR shape, or final RV64 destination shape.
- The full-bucket classifier found no current row-level F128 tokens and no
  row-level semantic BIR producer gap evidence; those lanes remain empty for
  this bucket split.
- `src/960209-1.c` remains evidence-only until a future packet gathers event
  kind, phase, authority, coordinate, value ids, homes, types, and
  F128-screenable detail.
- The prepared authority idea must stay separate from the RV64 materialization
  idea even though both lanes currently surface through the same final
  `unsupported_move_bundle_target_shape` diagnostic.

## Proof

Docs-only lifecycle proof command:

```sh
git diff --check -- todo.md \
  docs/rv64_gcc_torture_post_contract/move_bundle_target_shape_classification.md \
  ideas/open/551_rv64_move_bundle_materialization_from_classified_bucket.md \
  ideas/open/552_prepared_move_bundle_target_shape_authority_gaps.md \
  ideas/open/553_move_bundle_target_shape_evidence_gap_src_960209_1.md
python3 - <<'PY'
import csv
from collections import Counter
path = "docs/rv64_gcc_torture_post_contract/move_bundle_target_shape_classification.tsv"
rows = list(csv.DictReader(open(path), delimiter="\t"))
counts = Counter(row["first_owner_lane"] for row in rows)
expected = {
    "coherent_rv64_mir_materialization": 151,
    "prepared_module_target_shape_authority_gap": 31,
    "bir_semantic_producer_gap": 0,
    "f128_primary_quarantine": 0,
    "evidence_gap": 1,
}
for lane, expected_count in expected.items():
    if counts[lane] != expected_count:
        raise SystemExit(
            f"bad lane count for {lane}: got {counts[lane]}, expected {expected_count}"
        )
gap_cases = [row["case"] for row in rows if row["first_owner_lane"] == "evidence_gap"]
if gap_cases != ["src/960209-1.c"]:
    raise SystemExit(f"bad evidence gap cases: {gap_cases!r}")
for label, count in expected.items():
    print(f"{label}={count}")
print("followup_ideas=3")
PY
```
