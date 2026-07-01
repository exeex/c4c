Status: Active
Source Idea Path: ideas/open/422_bir_semantic_producer_high_impact_cleanup.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Current Semantic Producer Failure Evidence

# Current Packet

## Just Finished

Completed Step 1 for idea 422 by auditing current RV64 gcc_torture backend
evidence for failures whose first owner is semantic `lir_to_bir` lowering.

Evidence sources inspected:

- `docs/rv64_gcc_torture_post_contract/current_scan_summary.md`
- `docs/rv64_gcc_torture_post_contract/failure_bucket_map.md`
- `docs/rv64_gcc_torture_post_contract/followup_idea_plan.md`
- `build/agent_state/rv64_gcc_torture_backend_current_log_path.txt`
- `build/agent_state/rv64_gcc_torture_backend_current_20260630T032216Z.log`
- per-case logs under `build/rv64_gcc_c_torture_backend/*/case.log`
- mutable summary files under `build/agent_state/rv64_gcc_c_torture_backend_*`

Row/count summary from the coherent timestamped scan:

| Route | Count | Classification |
| --- | ---: | --- |
| `semantic lir_to_bir` | 373 | Main BIR semantic producer target for idea 422. |
| `bootstrap` | 44 | Adjacent bootstrap/pre-semantic lowering rows; keep separate from Step 2 semantic family bucketing. |
| Total object-route semantic-prerequisite failures | 417 | All rows gated by `backend object route requires semantic lir_to_bir lowering before the prepared object handoff`. |

Semantic family summary:

| Family | Count | Representative rows |
| --- | ---: | --- |
| load local-memory | 79 | `src/20041124-1.c`, `src/20071219-1.c`, `src/991228-1.c`, `src/multi-ix.c`, `src/pr22098-1.c` |
| GEP local-memory | 62 | `src/pr44468.c`, `src/pr48571-1.c`, `src/pr65956.c`, `src/pr80421.c`, `src/20000717-4.c` |
| store local-memory | 58 | `src/20010605-2.c`, `src/920501-5.c`, `src/930526-1.c`, `src/pr35472.c`, `src/pr57344-1.c` |
| direct-call semantic | 52 | `src/pr40022.c`, `src/pr41239.c`, `src/pr49390.c`, `src/20000412-2.c`, `src/20000419-1.c` |
| scalar/local-memory | 49 | `src/20020411-1.c`, `src/20041201-1.c`, `src/20070614-1.c`, `src/complex-2.c`, `src/complex-5.c` |
| memcpy runtime | 19 | `src/20030408-1.c`, `src/pr79354.c`, `src/pr86528.c`, `src/strlen-7.c`, `src/20000703-1.c` |
| alloca local-memory | 16 | `src/20180921-1.c`, `src/pr38151.c`, `src/pr85169.c`, `src/20050604-1.c`, `src/20050607-1.c` |
| memset runtime | 15 | `src/pr30778.c`, `src/pr51877.c`, `src/20041218-1.c`, `src/20050826-1.c`, `src/20071030-1.c` |
| scalar-control-flow | 10 | `src/20000314-3.c`, `src/20080502-1.c`, `src/930614-1.c`, `src/980604-1.c`, `src/ieee/fp-cmp-8.c` |
| function-signature | 9 | `src/20071029-1.c`, `src/ieee/pr72824-2.c`, `src/pr70903.c`, `src/20050316-3.c`, `src/pr60960.c` |
| call-return semantic | 3 | `src/complex-1.c`, `src/20050121-1.c`, `src/pr38969.c` |
| scalar-binop | 1 | `src/960513-1.c` |

Artifact:

- `build/agent_state/422_step1_semantic_producer_evidence/audit.md`
- `build/agent_state/422_step1_semantic_producer_evidence/semantic_lir_to_bir_rows.tsv`
- `build/agent_state/422_step1_semantic_producer_evidence/route_counts.tsv`
- `build/agent_state/422_step1_semantic_producer_evidence/semantic_family_counts.tsv`
- `build/agent_state/422_step1_semantic_producer_evidence/semantic_family_representatives.tsv`

## Suggested Next

Execute Step 2: bucket BIR producer families from the current evidence. Start
from the Step 1 row TSV and group the 373 `semantic lir_to_bir` rows by
producer family, owning layer, representative rows, rejected adjacent shapes,
and whether an implementation packet is coherent.

## Watchouts

- Use the timestamped scan log and Step 1 TSV as the coherent source. The
  mutable summary TSV/failed list are stale/truncated in the current checkout.
- Keep the 44 `bootstrap` rows separate from the 373 semantic producer rows.
- `failure_bucket_map.md` is useful context for `unsupported_instruction_fragment`
  but is not the row-level semantic producer artifact for idea 422.
- Step 2 can bucket from current evidence, but implementation selection should
  still require focused BIR/prepared probes for selected representatives.

## Proof

Delegated proof:

```sh
git diff --check
```

Result: passed.
