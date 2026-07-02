Status: Active
Source Idea Path: ideas/open/544_rv64_move_bundle_target_shape_bucket_split.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Classify The Full Bucket

# Current Packet

## Just Finished

Step 4 classified all 183 reconstructed `unsupported_move_bundle_target_shape`
rows using the Step 2 evidence rules. The full classification TSV is
`docs/rv64_gcc_torture_post_contract/move_bundle_target_shape_classification.tsv`,
with an audit note in
`docs/rv64_gcc_torture_post_contract/move_bundle_target_shape_classification.md`
and derived inspection artifacts under
`build/agent_state/544_step4_full_classification/`.

Lane counts reconcile to 183 rows:
`coherent_rv64_mir_materialization=151`,
`prepared_module_target_shape_authority_gap=31`,
`bir_semantic_producer_gap=0`, `f128_primary_quarantine=0`, and
`evidence_gap=1`.

## Suggested Next

Execute Step 5: split the classified bucket into follow-up source-idea
recommendations, keeping RV64 materialization work separate from prepared
authority gaps and preserving `src/960209-1.c` as an evidence-gathering row.

## Watchouts

- Do not implement RV64 lowering or producer repair in this review plan.
- Do not infer ownership from testcase names, target register spellings, or raw
  BIR shape.
- The full-bucket classifier found no current row-level F128 tokens and no
  row-level semantic BIR producer gap evidence.
- `src/960209-1.c` remains the only `evidence_gap` row because its current log
  only confirms bucket membership and omits event kind, phase, authority,
  value ids, homes, types, and F128-screenable details.
- Step 5 should not collapse prepared authority rows into RV64 materialization
  just because their final diagnostic is emitted by the RV64 object route.

## Proof

Docs-only proof command and TSV reconciliation check, preserved in
`test_after.log`:

```sh
git diff --check -- todo.md \
  docs/rv64_gcc_torture_post_contract/move_bundle_target_shape_classification.tsv \
  docs/rv64_gcc_torture_post_contract/move_bundle_target_shape_classification.md
python3 - <<'PY'
import csv
from collections import Counter
expected = "case\tlog\tfirst_owner_lane\tevidence_ref\tdiagnostic_key\tmove_shape\tfirst_missing_fact\tnotes"
allowed = {
    "coherent_rv64_mir_materialization",
    "prepared_module_target_shape_authority_gap",
    "bir_semantic_producer_gap",
    "f128_primary_quarantine",
    "evidence_gap",
}
rows_path = "docs/rv64_gcc_torture_post_contract/unsupported_move_bundle_target_shape_rows.tsv"
class_path = "docs/rv64_gcc_torture_post_contract/move_bundle_target_shape_classification.tsv"
with open(class_path, newline="") as f:
    header = f.readline().rstrip("\n")
    if header != expected:
        raise SystemExit(f"bad header: {header}")
    f.seek(0)
    classified = list(csv.DictReader(f, delimiter="\t"))
with open(rows_path, newline="") as f:
    source = list(csv.DictReader(f, delimiter="\t"))
bad = False
if len(classified) != 183:
    print(f"bad classified row count: {len(classified)}")
    bad = True
source_cases = [r["case"] for r in source]
classified_cases = [r["case"] for r in classified]
case_counts = Counter(classified_cases)
dupes = sorted(case for case, count in case_counts.items() if count != 1)
if dupes:
    print("bad duplicate/missing classified case counts:", dupes)
    bad = True
if set(source_cases) != set(classified_cases):
    print("bad case set mismatch")
    print("missing:", sorted(set(source_cases) - set(classified_cases)))
    print("extra:", sorted(set(classified_cases) - set(source_cases)))
    bad = True
for i, row in enumerate(classified, start=2):
    lane = row["first_owner_lane"]
    if lane not in allowed:
        print(f"bad lane at line {i}: {lane}")
        bad = True
    if sum(1 for candidate in allowed if lane == candidate) != 1:
        print(f"bad lane cardinality at line {i}: {lane}")
        bad = True
    if not row["evidence_ref"] or not row["diagnostic_key"] or not row["move_shape"] or not row["first_missing_fact"]:
        print(f"missing required classification field at line {i}: {row['case']}")
        bad = True
counts = Counter(row["first_owner_lane"] for row in classified)
print(f"classified_rows={len(classified)}")
for lane in sorted(allowed):
    print(f"{lane}={counts[lane]}")
raise SystemExit(1 if bad else 0)
PY
```
