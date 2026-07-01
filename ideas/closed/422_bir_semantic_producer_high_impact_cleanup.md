# BIR Semantic Producer High-Impact Cleanup

Status: Closed
Type: Implementation planning idea
Parent: `ideas/open/420_rv64_gcc_torture_post_contract_umbrella.md`
Owning Layer: BIR/semantic producer
Handoff Directory: `docs/rv64_gcc_torture_post_contract/`
Closed By: lifecycle review after Step 4

## Completion Notes

Idea 422 is complete as high-impact BIR semantic producer cleanup planning and
selection work.

The runbook established current evidence for failures that stop before prepared
object handoff with:

```text
backend object route requires semantic lir_to_bir lowering before the prepared object handoff
```

Step 1 extracted 417 failures from the coherent 2026-06-30 RV64 gcc_torture
backend scan, including 373 semantic `lir_to_bir` rows and 44 adjacent
bootstrap rows. Step 2 bucketed the semantic producer families. Step 3 selected
the first implementation-ready ordinary C family. Step 4 recorded residual
disposition and close readiness.

Completed artifacts:

- `build/agent_state/422_step1_semantic_producer_evidence/audit.md`
- `build/agent_state/422_step1_semantic_producer_evidence/semantic_lir_to_bir_rows.tsv`
- `build/agent_state/422_step2_bir_producer_buckets/bucket_table.tsv`
- `build/agent_state/422_step2_bir_producer_buckets/rationale.md`
- `build/agent_state/422_step3_first_producer_packet/decision.md`
- `build/agent_state/422_step4_residual_disposition/disposition.md`

## Selected Follow-Up

The selected first implementation owner is:

`ideas/open/483_bir_semantic_local_memory_scalar_load_producer.md`

Selected packet:

- first owner: BIR semantic local-memory load producer;
- selected bucket: local-memory load, 79 rows;
- representative rows: `src/20041124-1.c`, `src/20071219-1.c`,
  `src/991228-1.c`, `src/multi-ix.c`, `src/pr22098-1.c`;
- required future contract: semantic load identity, source memory/address
  value, local object or frame-slot identity, layout, size, alignment, access
  range, scalar result type, and ordinary C origin must be explicit producer
  facts.

## Residual Producer Buckets

The following remain separate producer owners and should not be silently folded
into the local-memory scalar load implementation idea:

| Residual bucket | Count | First owner |
| --- | ---: | --- |
| Local-memory GEP/address | 62 | BIR semantic local-memory GEP/address producer |
| Local-memory store | 58 | BIR semantic local-memory store producer |
| Direct-call argument metadata | 52 | BIR semantic direct-call argument/result metadata producer |
| Scalar/local-memory mixed | 49 | BIR scalar plus local-memory boundary, split before implementation |
| Memcpy/memset byte/object-representation | 34 | BIR runtime/intrinsic byte/object-representation producer |
| Alloca-derived storage | 16 | BIR semantic alloca/local stack object producer |
| Scalar-control-flow | 10 | BIR scalar control-flow producer |
| Function-signature | 9 | BIR function signature/type producer |
| Call-return | 3 | BIR semantic call-return producer |
| Scalar-binop | 1 | BIR scalar operation producer |
| Bootstrap adjacent | 44 | Bootstrap/pre-semantic global/type lowering |

## Closed Scope

Closed as complete for:

- semantic producer failure evidence audit;
- producer-family bucketing with counts and representatives;
- first implementation packet selection;
- residual producer-owner disposition.

Not closed as:

- implementation of local-memory load producer facts;
- GEP/store/call/runtime byte/alloca/scalar/bootstrap producer repairs;
- RV64/MIR recovery from missing BIR semantic facts.

## Validation

Lifecycle validation:

```sh
git diff --check
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_before.log --allow-non-decreasing-passed
```

Both passed. `test_after.log` was absent at close time, and this lifecycle task
forbade touching canonical logs, so no new after log was generated.

## Reviewer Reject Signals

Reject reopening this source as progress if the change:

- implements RV64/MIR recovery for BIR-owned semantic facts;
- broadens local-memory load implementation to GEP, store, call, byte runtime,
  alloca, scalar mixed, or bootstrap buckets without separate owner review;
- fabricates semantic pointer or object facts from raw BIR shape, value names,
  testcase names, final homes, or target fallback inference;
- presents expectation rewrites, unsupported downgrades, allowlists,
  pass/fail accounting changes, or classification-only churn as producer
  capability progress;
- makes F128 or `conversion.c` the route driver for this closed ordinary C
  semantic producer planning slice.
