Status: Active
Source Idea Path: ideas/open/424_prepared_global_stack_frame_infrastructure_review.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Classify Producer Facts Versus RV64 Emission Gaps

# Current Packet

## Just Finished

Completed `plan.md` Step 2 by generating fresh focused BIR, prepared-BIR, MIR,
trace, and RV64 object-route artifacts for five representative infrastructure
rows under
`build/agent_state/424_step2_infrastructure_classification/`.

Classification:

| Row | Classification | Evidence |
| --- | --- | --- |
| `src/20000603-1.c` | Coherent RV64 emission gap | Prepared frame facts publish fixed frame sizes/alignments, explicit frame slots, and callee-saved `gpr:s1` save-slot facts (`slot#10+stack16` for `f`, `slot#10+stack8` for `main`); object route still rejects supported prepared callee-saved GPR save slots. |
| `src/20030209-1.c` | Coherent RV64 emission gap | Prepared stack/frame facts publish `frame_size=80000`, `frame_alignment=8`, 10000 fixed slots, and no dynamic stack; object route still rejects generic prepared stack-frame support. |
| `src/20000412-1.c` | Coherent RV64 emission gap | Prepared addressing for `i` is coherent, and the object route reports selected object data as `unsupported_but_coherent object_label_id=2 object_size_bytes=1656`; RV64 lacks emission for that coherent object-data contract. |
| `src/930513-2.c` | Producer-contract gap | Prepared direct global accesses exist for `__static_local_eq_0`, but object route reports `missing_object_label object_size_bytes=0 emitted_byte_count=0 zero_fill_byte_count=0`; access facts are not enough object-data authority. |
| `src/20001017-1.c` | Producer-contract gap | Prepared call plans expose a 13-argument call, but stack-passed callee params `%p.B`, `%p.fdB`, `%p.b`, `%p.C`, and `%p.fdC` have `none` homes and caller stack args 8-12 lack destination offsets. |

The detailed packet summary is in
`build/agent_state/424_step2_infrastructure_classification/classification_summary.md`.
Concise classification notes were also added to
`docs/rv64_gcc_torture_post_contract/infrastructure_bucket_evidence.md`.

## Suggested Next

Execute Step 3: write
`docs/rv64_gcc_torture_post_contract/global_stack_frame_infrastructure_review.md`
from the row-backed classifications, separating coherent RV64 emission work
from producer-contract gaps.

## Watchouts

- Step 3 should preserve the owner split: RV64 emission candidates are
  callee-saved save slots, general fixed-frame emission, and coherent selected
  object-data emission; producer-contract candidates are static-local object
  labels/extents and stack-passed parameter-home publication.
- Do not turn producer gaps into RV64 inference from testcase shape, source
  names, raw global symbols, or argument indexes.
- Keep F128 local-memory, F128 parameter homes, expectation rewrites,
  unsupported-marker changes, runtime comparison, and pass/fail accounting out
  of this review.
- `src/20000603-1.c` also exposes an FPR save slot, but this packet's first
  stack-frame owner is the coherent prepared save-slot emission gap, not a
  broad FPR/F128 ABI route.

## Proof

Classification proof was written to `test_after.log`:

- `scripts/plan_review_state.py set-step --step-id 2 --step-title 'Classify Producer Facts Versus RV64 Emission Gaps'`
- `cmake --build build --target c4cll`
- For each representative row:
  - `./build/c4cll --dump-bir --target riscv64-linux-gnu <case>`
  - `./build/c4cll --dump-prepared-bir --target riscv64-linux-gnu <case>`
  - `./build/c4cll --dump-mir --target riscv64-linux-gnu <case>`
  - `./build/c4cll --trace-mir --target riscv64-linux-gnu <case>`
  - `./build/c4cll --codegen obj --target riscv64-linux-gnu <case> -o <artifact>/c4c.o`
- `git diff --check -- todo.md docs/rv64_gcc_torture_post_contract`

All dump commands returned `0`; all object-route commands returned `2` with
the expected fail-closed unsupported diagnostics used for classification.
