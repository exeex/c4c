Status: Active
Source Idea Path: ideas/open/255_phase_f3_prepared_private_pass_context_metadata_gate.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Final Field Decision Matrix

# Current Packet

## Just Finished

Step 5 - Final Field Decision Matrix completed for idea 255. Final field
states:

| Field | Final state | Decision evidence and proof notes |
| --- | --- | --- |
| `route` | Demoted | `PreparedBirModule::route` moved behind private `route_`; the public read surface is `prepared_route(const PreparedBirModule&)`. Step 1 found no x86/riscv target-facing direct public consumer. Step 2 required the prepared header row to remain `route=semantic_bir_shared`, invalid enum values to continue flowing through `prepare_route_name(...) == "unknown"`, and x86 route-debug/prepared-dump strings to stay unchanged. Step 4 preserved the header path through `prepare_route_name(prepared_route(module))`, kept the default `PrepareRoute::SemanticBirShared`, and updated the only route-specific designated initializer without reworking unrelated fields. Route code proof passed the 3/3 focused prepared-printer/CLI tests selected by the supervisor. Supervisor-side regression guard also passed non-decreasing with 3/3 before and 3/3 after. |
| `invariants` | Retained as public compatibility | Step 1 found pass-context writers, prepared-printer reads, and direct test reads in `backend_prepare_phi_materialize_test.cpp`; no x86/riscv target-facing direct read was found. Step 2 recorded stable invariant-name behavior for `no_target_facing_i1` and `no_phi_nodes`, but also recorded missing printer/CLI proof for rendered `invariants:` rows, missing invalid-enum fail-closed coverage, and missing mismatched-state coverage such as invariant-without-phase or phase-without-invariant. Because this runbook did not add those proof fences or an accepted accessor shape for invariants, the field remains public compatibility rather than demoted. |
| `completed_phases` | Retained as public compatibility | Step 1 found pass-context writers and prepared-printer/CLI readers; no direct x86/riscv target-facing public consumer was found. Step 2 recorded the byte-stable row `completed_phases: legalize stack_layout liveness out_of_ssa regalloc` as required by the prepared CLI dump, but also recorded missing focused omission coverage for empty phase lists and missing mismatched metadata fences such as completed phase without the corresponding invariant or note. Because the field ordering/status row remains a compatibility surface and no demotion slice proved those gaps, the field remains public compatibility. |
| `notes` | Retained as public compatibility | Step 1 found pass-context writers, stack-layout publication append paths, prepared-printer reads, CLI note snippets, and direct test reads in `backend_prepare_stack_layout_test.cpp`; no x86/riscv prepared-module target-facing direct read was found. Step 2 recorded populated note rows and unresolved-PIC diagnostic payload compatibility, but also recorded that absent-note fallback is currently omission-only with no focused fail-closed test proving empty notes omit the section or fallback state avoids synthesizing a note. Because direct payload access and absent-note omission remain under-proved for demotion, the field remains public compatibility. |

`PreparedBirModule::liveness` stayed outside the runbook and was not edited or
classified as part of this field matrix.

No source-idea change is proposed by this executor packet; lifecycle closure or
split decisions remain supervisor/plan-owner work.

## Suggested Next

Supervisor should decide whether to hand the completed Step 5 matrix to the
plan owner for lifecycle close, retirement, or split. The next implementation
packet, if any, should be a new lifecycle-selected field/proof initiative, not
an expansion of this completed route demotion slice.

## Watchouts

- Keep future `route` reads on the `prepared_route(...)` helper; do not
  reintroduce public `PreparedBirModule::route`.
- Do not treat the retained-public decisions for `invariants`,
  `completed_phases`, or `notes` as blocker-free demotion approval. Their
  current reasons are proof gaps and compatibility surfaces named in Step 2,
  not target-facing x86/riscv consumers.
- `PreparedBirModule::liveness` remains explicitly out of scope for idea 255.

## Proof

Analysis/todo-only packet; no build required by the supervisor.

Command:
`git diff --check -- todo.md`

Result: passed.

Related supervisor-side validation note recorded as requested: route code proof
passed 3/3 focused prepared-printer/CLI tests, and non-decreasing regression
guard passed with 3/3 before and 3/3 after.
