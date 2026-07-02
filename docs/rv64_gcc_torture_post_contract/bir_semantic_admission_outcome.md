# BIR Semantic Admission Reconstruction Outcome

Status: Step 4 evidence outcome for the 2026-07-02 RV64 gcc_torture backend
scan reconstruction.

## Evidence Inputs

- Source idea:
  `ideas/open/545_bir_semantic_producer_admission_reconstruction.md`
- Active runbook: `plan.md`, Step 4 - Prove The Reconstruction Outcome
- Row artifact:
  `docs/rv64_gcc_torture_post_contract/bir_semantic_admission_rows.md`
- Classification artifact:
  `docs/rv64_gcc_torture_post_contract/bir_semantic_admission_classification.md`
- Follow-up routing artifact:
  `docs/rv64_gcc_torture_post_contract/bir_semantic_admission_followups.md`
- Current scan pointer:
  `build/agent_state/rv64_gcc_torture_backend_current_log_path.txt` ->
  `build/agent_state/rv64_gcc_torture_backend_current_20260702T151551Z.log`
- Per-case log root:
  `build/rv64_gcc_c_torture_backend/<case-id>/case.log`

## Outcome

The reconstruction outcome is auditable and ready for lifecycle follow-up.

The current source idea is satisfied as an evidence-reconstruction runbook:
the current BIR semantic admission rows were reconstructed from the
timestamped scan pointer and per-case logs, classified by first visible BIR
producer topic, and converted into precise follow-up routing inputs.

The source idea is not an implementation-complete compiler repair. It needs
plan-owner lifecycle follow-up to create or activate separate implementation
ideas from the routed lanes below. No fresh scan is required by this packet:
the row evidence reconciles against the current 2026-07-02 scan artifacts, and
there are no missing-evidence blockers in the exact semantic row set.

## Row Reconciliation

The row artifacts reconcile as follows:

| Row set | Rows | Outcome |
| --- | ---: | --- |
| Exact `semantic lir_to_bir` rows | 373 | all have visible first-topic evidence |
| Related bootstrap/global data-shape rows | 44 | separate BIR handoff lane, excluded from exact semantic rows |
| Total BIR handoff-related rows inspected | 417 | `373 + 44` |

Exact semantic first-owner lanes reconcile as:

| Lane | Rows | Included first-topic families |
| --- | ---: | --- |
| Local-memory facts | 264 | `load`, `gep`, `store`, `scalar/local-memory`, `alloca` |
| Call metadata | 55 | `direct-call`, `call-return` |
| Runtime/intrinsic memory facts | 34 | `memcpy`, `memset` |
| Scalar/signature/control facts | 20 | `scalar-control-flow`, `function-signature`, `scalar-binop` |

Exact semantic rows accounted for by first owner:
`264 + 55 + 34 + 20 = 373`.

The related bootstrap/global data-shape lane remains separate:
`44` related BIR handoff rows are not counted as exact
`semantic lir_to_bir` producer rows.

Rejected first-owner lanes remain at `0` rows for the exact semantic set:
aggregate-first facts, publication gaps, malformed semantic inputs, prepared
contract gaps, RV64/MIR object lowering, runtime mismatch, test
infrastructure, F128 quarantine, and evidence gaps.

## Evidence-Only Proof

The reconciliation can be repeated from the checked-in markdown row table:

```sh
awk -F'|' '/^\| `src\// {
  kind=$3
  path=$7
  gsub(/^ +| +$/,"",kind)
  gsub(/^ +| +$/,"",path)
  if (path ~ /\/workspaces\/c4c\/build\/rv64_gcc_c_torture_backend\//) {
    count[kind]++
  }
}
END {
  for (k in count) print k, count[k]
}' docs/rv64_gcc_torture_post_contract/bir_semantic_admission_rows.md | sort
```

Observed output:

```text
exact semantic 373
related bootstrap 44
```

The per-topic counts from the same table are:

```text
79  `load local-memory semantic family`
62  `gep local-memory semantic family`
58  `store local-memory semantic family`
52  `semantic call family 'direct-call semantic family'`
49  `scalar/local-memory semantic family`
44  `bootstrap global/data shape admission note`
19  `runtime/intrinsic family 'memcpy runtime family'`
16  `alloca local-memory semantic family`
15  `runtime/intrinsic family 'memset runtime family'`
10  `scalar-control-flow semantic family`
9   `function-signature semantic family`
3   `semantic call family 'call-return semantic family'`
1   `scalar-binop semantic family`
```

These topic counts reconcile to the owner lanes:

- Local-memory: `79 + 62 + 58 + 49 + 16 = 264`.
- Call metadata: `52 + 3 = 55`.
- Runtime/intrinsic: `19 + 15 = 34`.
- Scalar/signature/control: `10 + 9 + 1 = 20`.
- Related bootstrap/global data-shape: `44`.

## Non-Regression Statement

This Step 4 packet changed only evidence documentation and canonical execution
state. It did not edit implementation files, tests, expectation files,
unsupported markers, allowlists, runtime comparison behavior, `test_before.log`,
or `test_after.log`.

No compiler behavior was weakened. No row was reclassified by downgrading a
test contract or by moving a supported-path failure into an unsupported,
allowlisted, or expectation-only lane.

## Plan-Owner Lifecycle Inputs

Supervisor should pass this artifact plus
`docs/rv64_gcc_torture_post_contract/bir_semantic_admission_followups.md` to
the plan-owner as lifecycle input.

Recommended plan-owner action:

1. Treat
   `ideas/open/545_bir_semantic_producer_admission_reconstruction.md` as
   satisfied for evidence reconstruction, subject to supervisor acceptance.
2. Create or activate a separate idea for BIR local-memory semantic producer
   admission covering the `264` exact semantic rows.
3. Create or activate a separate idea for BIR call metadata semantic producer
   admission covering the `55` exact semantic rows.
4. Create or activate a separate idea for BIR runtime/intrinsic memory producer
   admission covering the `34` exact semantic rows.
5. Create or activate a separate idea for BIR scalar/signature/control semantic
   producer admission covering the `20` exact semantic rows, or explicitly
   defer this smaller lane until higher-frequency lanes are routed.
6. Create or activate a separate related idea for bootstrap/global data-shape
   handoff support covering the `44` related rows; do not count it as part of
   the `373` exact semantic producer row set.

No executor-created source idea is needed or allowed for this outcome packet.
