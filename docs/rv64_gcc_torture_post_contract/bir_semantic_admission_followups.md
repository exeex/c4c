# BIR Semantic Admission Follow-Up Routing

Status: Step 3 routing candidates from the 2026-07-02 classified BIR semantic admission evidence.

## Evidence Anchor

- Source idea: `ideas/open/545_bir_semantic_producer_admission_reconstruction.md`
- Active runbook: `plan.md`, Step 3 - Generate Follow-Up Routing
- Row artifact: `docs/rv64_gcc_torture_post_contract/bir_semantic_admission_rows.md`
- Classification artifact: `docs/rv64_gcc_torture_post_contract/bir_semantic_admission_classification.md`
- Current scan pointer: `build/agent_state/rv64_gcc_torture_backend_current_log_path.txt` -> `build/agent_state/rv64_gcc_torture_backend_current_20260702T151551Z.log`
- Per-case log root: `build/rv64_gcc_c_torture_backend/<case-id>/case.log`

This artifact does not create source ideas. It records lifecycle routing inputs
for the supervisor and plan-owner.

## Routing Basis

The classified exact semantic row set contains `373` current rows with visible
`semantic lir_to_bir` first-topic evidence:

| Lane | Rows | Included first-topic families | Routing basis |
| --- | ---: | --- | --- |
| Local-memory facts | 264 | `load`, `gep`, `store`, `scalar/local-memory`, `alloca` | High-frequency BIR semantic local-memory producer boundary |
| Call metadata | 55 | `direct-call`, `call-return` | High-frequency BIR semantic call producer boundary |
| Runtime/intrinsic memory facts | 34 | `memcpy`, `memset` | Runtime/intrinsic producer boundary, separate from generic local-memory |
| Scalar/signature/control facts | 20 | `scalar-control-flow`, `function-signature`, `scalar-binop` | Smaller BIR semantic scalar/signature/control boundary |
| Bootstrap/global data-shape handoff | 44 | scalar integer/pointer globals, linear integer-array globals, aggregate-backed globals with byte-address semantics | Related BIR handoff lane, not counted in the `373` exact semantic rows |

Rejected first-owner lanes from the exact semantic row set remain at `0` rows:
aggregate-first facts, publication gaps, malformed or intentionally rejected
semantic inputs, prepared contract gaps, RV64/MIR object lowering, runtime
mismatch, test infrastructure, F128 quarantine, and evidence gaps.

## Follow-Up Candidates

| Candidate | Rows | Intended owner | Proof shape | Reviewer reject signals | Lifecycle action |
| --- | ---: | --- | --- | --- | --- |
| BIR local-memory semantic producer admission | 264 | BIR local-memory producer in `src/backend/bir/lir_to_bir.cpp` and `src/backend/bir/lir_to_bir/memory/` | Focused BIR tests for local-memory load/gep/store/scalar-local/alloca fact publication, then a narrow RV64 gcc_torture subset drawn from representative local-memory rows | Named-case shortcuts, expectation or unsupported-marker downgrades, RV64/MIR-side inference of facts missing from BIR, or proof that only one testcase shape moved while nearby local-memory families remain unexamined | Strong new idea candidate. Keep separate from call metadata, runtime/intrinsic, scalar/control, and bootstrap lanes. |
| BIR call metadata semantic producer admission | 55 | BIR call producer in `src/backend/bir/lir_to_bir/calling.cpp` and adjacent call semantic emission | Focused BIR tests for direct-call callee/argument metadata and call-return result metadata, then narrow RV64 subset covering direct-call and call-return representatives | Treating call failures as generic scalar/local-memory, adding downstream call lowering assumptions, weakening call semantic admission, or proving only direct-call while leaving call-return unaddressed | Strong new idea candidate. Separate call metadata route from local-memory and runtime/intrinsic routes. |
| BIR runtime/intrinsic memory producer admission | 34 | BIR runtime/intrinsic lowering path that emits semantic facts for `memcpy` and `memset` memory effects | Focused BIR tests for intrinsic memory-effect fact publication for `memcpy` and `memset`, then narrow RV64 subset covering both runtime/intrinsic families | Folding intrinsics into generic local-memory without proving the intrinsic producer boundary, replacing runtime calls with named-case lowering, or proving `memcpy` while `memset` remains unsupported | New idea candidate, but lower priority than the larger local-memory and call-metadata lanes. Keep separate from generic local-memory unless code evidence proves a shared producer. |
| BIR scalar/signature/control semantic producer admission | 20 | BIR scalar, function-signature, and control-flow semantic producers | Focused BIR tests for scalar-control-flow, function-signature, and scalar-binop fact publication, then narrow RV64 subset covering all three first-topic families | Mixing this lane into local-memory because some cases later touch memory, treating signature failures as ABI/RV64 lowering before BIR publication is proven, or proving only the single scalar-binop row as a named case | Candidate for a small follow-up idea or explicit deferred packet after higher-frequency lanes. Keep as one scalar/signature/control lane only if producer inspection confirms shared semantic admission code; otherwise split before implementation. |
| Bootstrap/global data-shape handoff support | 44 | BIR bootstrap/global data-shape support before prepared object handoff | Focused BIR/global bootstrap tests for scalar pointer/integer globals, linear integer-array globals, and aggregate-backed globals with byte-address semantics, then a narrow RV64 subset from the `44` related handoff rows | Counting these rows as exact `semantic lir_to_bir` rows, merging bootstrap data-shape support into local-memory semantic producer work, or claiming prepared/RV64 ownership before the BIR handoff limitation is removed | Separate related new idea candidate. Do not merge into the `373` exact semantic producer follow-up unless fresh row evidence shows a shared producer boundary. |

## Deferred Or Rejected Routes

| Route | Rows | Decision | Reason |
| --- | ---: | --- | --- |
| Aggregate facts as a first owner | 0 | Reject for current follow-up creation | No exact semantic row exposes aggregate-first producer evidence. Aggregate behavior may appear inside other lanes but is not a first-owner route here. |
| Publication gaps or prepared/RV64 handoff | 0 | Reject for exact semantic rows | Current diagnostics stop at BIR semantic admission; routing to publication or downstream RV64/MIR would bypass the missing producer fact. |
| Malformed or intentionally rejected semantic inputs | 0 | Reject | No exact semantic row currently exposes this as the first owner. |
| Prepared contract gaps | 0 | Reject | No exact semantic row reaches prepared contract ownership before BIR semantic admission fails. |
| RV64/MIR object lowering | 0 | Reject | These rows lack BIR facts and should not be repaired by downstream inference. |
| Runtime mismatch | 0 | Reject | Exact semantic rows fail before runtime comparison. |
| Test infrastructure | 0 | Reject | No evidence of harness or infrastructure ownership in the exact semantic set. |
| F128 quarantine | 0 | Reject as first owner | Case names may include floating-point-oriented tests, but first-topic evidence is BIR semantic producer admission. |
| Evidence gaps | 0 | Reject | All `373` exact semantic rows expose a visible first-topic line. |

## Representative Proof Seeds

Use these current rows as initial proof seeds, then expand within each lane
before accepting implementation progress:

| Lane | Representative rows |
| --- | --- |
| Local-memory facts | `src/20000314-1.c` (`load`), `src/20000717-4.c` (`gep`), `src/20001026-1.c` (`store`), `src/20000519-1.c` (`scalar/local-memory`), `src/20050604-1.c` (`alloca`) |
| Call metadata | `src/20000412-2.c` (`direct-call`), `src/20050121-1.c` (`call-return`) |
| Runtime/intrinsic memory facts | `src/20000703-1.c` (`memcpy`), `src/20041218-1.c` (`memset`) |
| Scalar/signature/control facts | `src/20000314-3.c` (`scalar-control-flow`), `src/20050316-3.c` (`function-signature`), `src/960513-1.c` (`scalar-binop`) |
| Bootstrap/global data-shape handoff | `src/strlen-2.c` as the inspected representative from classification |

## Exact Lifecycle Action Needed Next

Supervisor should pass this artifact to the plan-owner as lifecycle input. The
recommended plan-owner action is to create or activate separate follow-up ideas
for:

1. BIR local-memory semantic producer admission.
2. BIR call metadata semantic producer admission.
3. BIR runtime/intrinsic memory producer admission.
4. BIR scalar/signature/control semantic producer admission, or an explicit
   defer decision until the higher-frequency lanes are routed.
5. Bootstrap/global data-shape handoff support as a separate related idea, not
   as part of the exact semantic producer row set.

No current lifecycle source idea should be edited by this executor packet.
