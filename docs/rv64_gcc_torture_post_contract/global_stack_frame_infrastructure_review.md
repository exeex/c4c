# RV64 Global Data And Stack Frame Infrastructure Review

Status: Step 3 row-backed handoff for idea 424.

## Scope

This review covers the ordinary-C infrastructure buckets from the current
RV64 gcc_torture backend scan:

| Bucket | Count | Handoff owner |
| --- | ---: | --- |
| `unsupported_stack_frame` | 84 | Split between coherent RV64 prepared-frame emission and parked broad FPR/F128 work. |
| `unsupported_global_data` | 40 | Split between coherent RV64 selected object-data emission and producer object-data contract gaps. |
| `unsupported_param_home` | 4 | Producer ABI/prealloc parameter-home publication. |

Source artifacts:

- `docs/rv64_gcc_torture_post_contract/infrastructure_bucket_evidence.md`
- `build/agent_state/424_step2_infrastructure_classification/classification_summary.md`
- focused row artifacts under
  `build/agent_state/424_step2_infrastructure_classification/`
- full-scan rows from
  `build/agent_state/rv64_gcc_c_torture_backend_summary.full.tsv`

The first ownership split is by prepared authority, not by final diagnostic
text alone. Rows with coherent prepared facts can feed RV64 emission follow-up
ideas. Rows with missing labels, extents, frame homes, offsets, or ABI homes
must remain fail-closed until producer contracts publish those facts.

## Coherent RV64 Emission Gaps

These rows already expose enough prepared authority for a target-emission
follow-up. RV64 object emission should consume the prepared facts directly and
must not reconstruct them from testcase shape, symbol spelling, or register
names.

| Row | Bucket | Evidence artifacts | Prepared authority | Follow-up direction |
| --- | --- | --- | --- | --- |
| `src/20000603-1.c` | `unsupported_stack_frame` save-slot row | `build/agent_state/424_step2_infrastructure_classification/20000603-1/` | Prepared frame facts publish fixed frame sizes and alignments for `f` and `main`; both functions have explicit `gpr:s1` saved-register slots (`slot#10+stack16` for `f`, `slot#10+stack8` for `main`) with size/alignment `8`. | Create an RV64 object-emission idea for prepared callee-saved GPR save-slot prologue/epilogue support. |
| `src/20030209-1.c` | `unsupported_stack_frame` generic frame row | `build/agent_state/424_step2_infrastructure_classification/20030209-1/` | Prepared stack layout publishes `frame_size=80000`, `frame_alignment=8`, 10000 fixed slots, no dynamic stack, and a prepared frame plan. | Create an RV64 object-emission idea for general fixed prepared stack-frame support, including large frames and fixed slots. |
| `src/20000412-1.c` | `unsupported_global_data` coherent selected object-data row | `build/agent_state/424_step2_infrastructure_classification/20000412-1/` | Prepared addressing for `i` is coherent, and selected object data reports `status=unsupported_but_coherent object_label_id=2 object_size_bytes=1656`. | Create an RV64 object/global-data emission idea to emit coherent selected object-data extents, data, zero-fill, and relocations from the prepared contract. |

The `20030209-1.c` evidence also contains prepared FPR save-slot details
(`fs1`, `fs2`). That evidence should stay visible as a compatibility watchout,
but it is not a reason to expand this infrastructure review into broad FPR or
F128 ABI work.

## Producer-Contract Gaps

These rows do not yet give RV64 enough prepared authority to emit correct
object code. They should become producer-owned follow-up ideas before any RV64
consumer route is accepted.

| Row | Bucket | Evidence artifacts | Missing authority | Follow-up direction |
| --- | --- | --- | --- | --- |
| `src/930513-2.c` | `unsupported_global_data` missing object-label row | `build/agent_state/424_step2_infrastructure_classification/930513-2/` | Prepared direct global accesses exist for `__static_local_eq_0`, but selected object-data reports `status=missing_object_label object_size_bytes=0 emitted_byte_count=0 zero_fill_byte_count=0`. Access facts alone do not define the storage object. | Create a producer/global-data contract idea to publish static-local object labels, object size, and data/zero-fill extents for function-scope static storage. |
| `src/20001017-1.c` | `unsupported_param_home` | `build/agent_state/424_step2_infrastructure_classification/20001017-1/` | The 13-argument call is visible, but stack-passed callee homes for `%p.B`, `%p.fdB`, `%p.b`, `%p.C`, and `%p.fdC` are `none`; caller stack arguments 8-12 have stack-slot bindings without destination offsets or register/bank homes. | Create a producer ABI/prealloc idea to publish stack-passed parameter homes with offsets, widths, alignment, and storage kind. |

RV64 must keep these routes fail-closed until the producer contracts are
explicit. Reconstructing object labels from global names or stack homes from
argument indexes would move missing authority into the target backend and
would be route drift.

## Parked And Out Of Scope

- F128 local memory, F128 parameter homes, and F128 ABI plumbing remain parked
  outside this infrastructure review.
- Broad FPR/F128 ABI expansion is out of scope. FPR save-slot facts can be
  cited as nearby evidence, but should not be bundled into the first ordinary-C
  RV64 stack-frame follow-up.
- Runtime mismatch ownership, inline assembly gaps, scalar FPR arithmetic, and
  pointer provenance work remain separate tracks.
- Expectation rewrites, unsupported-marker changes, allowlist edits, runtime
  comparison changes, and pass/fail accounting changes are not capability
  progress for this route.

## Reject Signals

Reject any follow-up or implementation slice that:

- infers labels, relocations, zero-fill ranges, object extents, frame slots,
  stack offsets, widths, alignments, save slots, or parameter homes inside RV64
  when the prepared contract has not published them
- combines producer repair and RV64 consumption in one implementation idea
- changes expectations, unsupported markers, allowlists, or runtime comparison
  instead of repairing the underlying producer or emission capability
- uses testcase-shaped shortcuts based on source filenames, symbol names,
  argument counts, raw slot numbers, or one representative row
- treats F128/local-memory/F128 parameter-home work as part of ordinary-C
  stack-frame or global-data recovery

## Step 4 Recommendations

Step 4 should create separate open follow-up ideas, each citing the row-backed
evidence above:

- RV64 prepared callee-saved GPR save-slot emission, seeded by
  `src/20000603-1.c` and nearby save-slot rows.
- RV64 general fixed prepared stack-frame emission, seeded by
  `src/20030209-1.c` and other `requires a supported prepared stack frame`
  rows.
- RV64 selected object-data emission for coherent prepared object-data
  contracts, seeded by `src/20000412-1.c`.
- Producer object-data contract publication for static-local labels, object
  size, data, and zero-fill extents, seeded by `src/930513-2.c`.
- Producer ABI/prealloc stack-passed parameter-home publication, seeded by
  `src/20001017-1.c` and the remaining `unsupported_param_home` rows.

Do not create a combined umbrella implementation idea for all five owners. The
producer gaps must be repaired before RV64 consumers can lower those rows, and
the coherent RV64 emission gaps should stay narrow enough to prove with
ordinary-C coverage rather than scan-accounting changes.
