Status: Active
Source Idea Path: ideas/open/482_semantic_frame_slot_materialization_probe_decomposition.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Split The Monolithic Probe Into Focused Cases

# Current Packet

## Just Finished

Completed Step 2 for idea 482 by splitting the monolithic `%t23` route into
focused backend probe candidates without adding tests.

Accepted focused probe candidates:

| Candidate | Proposed future path | Why accepted |
| --- | --- | --- |
| Scalar compare result forced to frame-slot destination | `tests/backend/case/riscv64_scalar_compare_frame_slot_destination.c` | Non-duplicative because it isolates semantic instruction-result identity plus final frame-slot destination without copying the `930930-1` select/pointer/terminator graph. Capability-oriented because it tests whether prepared facts can represent identity and destination separately from materialization authority. |
| Storage-only move rejection | `tests/backend/case/riscv64_storage_only_move_not_semantic_materialization.c` | Non-duplicative because it isolates the rejected `%t22 -> %t23 authority=none` shape as a standalone negative family. Capability-oriented because it protects the rule that storage movement is not semantic producer authority. |
| Select-result stack-destination boundary | `tests/backend/case/riscv64_select_result_stack_destination_boundary.c` | Non-duplicative because it focuses on `%t22` as a select-result stack destination rather than the `%t23` scalar compare result. Capability-oriented because it prevents select publication from being conflated with scalar instruction-result materialization. |
| Explicit synthetic materialization-point positive | `tests/backend/case/riscv64_explicit_compare_frame_slot_materialization_point.c` | Non-duplicative because it is a positive control for explicit event authority, not another audit of the real missing producer. Capability-oriented because it proves the materialization seam can accept supplied authority without raw-shape inference. |

Rejected probe candidates:

| Candidate | Reason rejected |
| --- | --- |
| Copy or trim `930930-1` into `tests/backend/case/` | Duplicates the monolithic graph and continues the stuck `%t23` chase. |
| One combined probe covering scalar destination, storage-only rejection, select boundary, and synthetic positive authority | Conflates seams and would make Step 3 ownership ambiguous. |
| Raw-BIR-adjacency materialization probe | Encodes the rejected inference path; raw order plus final home is not authority. |
| Final-home-only or stack-object-only materialization probe | Repeats the 475 -> 480 failure by treating placement/layout as event production. |
| RV64 branch-load or object-route probe | Downstream non-goal until explicit producer authority and later interval/source facts exist. |
| Pointer/provenance or unsupported-terminator repair probe | Separate protected boundary family from scalar compare/result frame-slot materialization. |
| Expectation, allowlist, baseline, or unsupported-marker probe | Accounting churn, not backend capability decomposition. |

Step 2 keeps the storage-only move rejection, select-result boundary, and
synthetic positive probe as separate families. No future path was created in
this packet; paths are proposals only.

Artifact:

- `build/agent_state/482_step2_focused_probe_candidates/candidates.md`

## Suggested Next

Execute Step 3 by binding each accepted candidate to exactly one backend seam.
The first seam-binding target should be
`tests/backend/case/riscv64_scalar_compare_frame_slot_destination.c` as a
proposal only: semantic instruction-result identity plus frame-slot
destination/layout, with materialization authority, storage-only movement,
select-result publication, interval/source facts, branch-stack-load authority,
and RV64 consumption all marked as separate boundaries.

## Watchouts

- Do not add tests until Step 3 binds each accepted probe to one owner and one
  fail-closed boundary.
- Do not let the scalar compare destination probe become a hidden
  materialization-authority probe.
- Do not merge storage-only move rejection, select-result stack destination,
  and synthetic positive materialization into one case.
- Do not infer authority from raw BIR, final homes, storage, stack objects,
  offsets, names, testcase shape, or dump order.

## Proof

Delegated proof:

```sh
git diff --check
```

Result: passed.
