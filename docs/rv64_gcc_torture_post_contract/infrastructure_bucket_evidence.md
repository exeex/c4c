# RV64 Infrastructure Bucket Evidence

Status: Step 1 evidence inventory for idea 424.

## Source Artifacts

- `docs/rv64_gcc_torture_post_contract/failure_bucket_map.md`
- `build/agent_state/rv64_gcc_c_torture_backend_summary.full.tsv`
- `build/agent_state/rv64_gcc_c_torture_backend_failed.full.txt`
- per-row logs under `build/rv64_gcc_c_torture_backend/<case-id>/case.log`
- derived local row inventory:
  `build/agent_state/424_step1_infrastructure_bucket_evidence/bucket_rows.tsv`

The full scan summary records `1467` total rows, `314` pass rows, and `1153`
fail rows. The target infrastructure buckets remain separate from runtime
mismatch ownership, instruction-fragment residual work, and F128 quarantine.

## Target Bucket Counts

| Bucket | Count | Representative diagnostics |
| --- | ---: | --- |
| `unsupported_stack_frame` | 84 | 68 callee-saved GPR save-slot rows; 16 unsupported prepared stack-frame rows. |
| `unsupported_global_data` | 40 | mixed producer/object-data contract and RV64 global-memory emission diagnostics. |
| `unsupported_param_home` | 4 | unsupported parameter homes outside supported GPR or prepared FPR register homes. |

## Representative Rows

### `unsupported_stack_frame`

- `src/20000603-1.c`: `RV64 object route requires supported prepared callee-saved GPR save slots`
- `src/20030209-1.c`: `RV64 object route requires a supported prepared stack frame`
- `src/20040313-1.c`: `RV64 object route requires a supported prepared stack frame`
- `src/920710-1.c`: `RV64 object route requires supported prepared callee-saved GPR save slots`
- `src/align-2.c`: `RV64 object route requires supported prepared callee-saved GPR save slots`

### `unsupported_global_data`

- `src/20000412-1.c`: `prepared selected object-data contract status=unsupported_but_coherent object_label_id=2`
- `src/930513-2.c`: `prepared selected object-data contract status=missing_object_label object_size_bytes=0`
- `src/20001121-1.c`: `RV64 object route supports only 1-, 2-, 4-, and 8-byte prepared global memory accesses`
- `src/20020118-1.c`: `RV64 object route cannot emit prepared global symbol`
- `src/20140212-1.c`: `RV64 object route requires supported prepared global memory facts`
- `src/pr42721.c`: `RV64 object route requires prepared direct global-symbol base-plus-offset memory addressing`

### `unsupported_param_home`

- `src/20001017-1.c`: `RV64 object route requires all parameters in supported GPR or prepared FPR register homes`
- `src/20010518-1.c`: `RV64 object route requires all parameters in supported GPR or prepared FPR register homes`
- `src/pr27073.c`: `RV64 object route requires all parameters in supported GPR or prepared FPR register homes`
- `src/pr69447.c`: `RV64 object route requires all parameters in supported GPR or prepared FPR register homes`

## Evidence Availability

The full-scan row evidence has one `case.log` per failed case. No matching
per-row BIR, prepared-BIR, MIR, or object-emission dumps were present in the
scan case directories for the target representative rows; Step 2 should create
focused dumps for selected representatives before classifying first ownership.

Local derived files for this packet:

- `build/agent_state/424_step1_infrastructure_bucket_evidence/bucket_counts.txt`
- `build/agent_state/424_step1_infrastructure_bucket_evidence/bucket_rows.tsv`
- `build/agent_state/424_step1_infrastructure_bucket_evidence/representatives.txt`

## Step 2 Classification Input

Step 2 should classify representatives by prepared fact completeness versus
RV64 emission gaps:

- stack-frame: save slots, frame shape, stack offsets, widths, alignments, and
  lifetime facts
- global-data: object labels, selected data contracts, memory access widths,
  direct symbol addressing, zero-fill/object extents, and alignment facts
- parameter-home: ABI home kind, offset, width, and whether prepared producers
  publish enough authority for RV64 consumption

Rows with missing or ambiguous producer authority should remain fail-closed and
be routed to producer-contract follow-up work rather than RV64 reconstruction.

## Step 2 Representative Classification

Fresh focused dumps were written under
`build/agent_state/424_step2_infrastructure_classification/`.

| Row | Classification | Handoff note |
| --- | --- | --- |
| `src/20000603-1.c` | Coherent RV64 emission gap | Prepared callee-saved GPR save-slot facts are explicit; RV64 object emission still rejects save-slot support. |
| `src/20030209-1.c` | Coherent RV64 emission gap | Prepared fixed-frame facts are explicit for a large `80000` byte frame; RV64 object emission still rejects generic prepared frame support. |
| `src/20000412-1.c` | Coherent RV64 emission gap | Selected object data is `unsupported_but_coherent` with object label and size; RV64 must emit the coherent object-data contract. |
| `src/930513-2.c` | Producer-contract gap | Direct global accesses exist, but the static-local object has no object label, size, or zero-fill/data extent authority. |
| `src/20001017-1.c` | Producer-contract gap | Stack-passed parameter homes are `none`/offsetless in prepared facts; RV64 must keep failing closed until ABI homes are published. |

Step 3 should turn this into the durable row-backed handoff doc, preserving the
split between producer-contract work and RV64 emission work.
