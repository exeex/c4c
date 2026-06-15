Status: Active
Source Idea Path: ideas/open/267_phase_f4_edge_publication_source_producers_x86_consumer_boundary_fail_closed_blocker_map.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Decide Implementation Eligibility

# Current Packet

## Just Finished

Completed Step 4, `Decide Implementation Eligibility`, for idea 267 as an
analysis-only lifecycle decision using the Step 3 fail-closed row map.

Decision: implementation remains blocked in this runbook. The Step 3 map found
Route 5/BIR source-producer identity evidence and partial x86 agreement checks,
but it did not find both required conditions for implementation eligibility:

- Semantic authority exists only as partial comparison evidence. Route 5/BIR
  records and MIR queries can describe source-producer identity, and x86 can
  compare copied prepared publication identity against Route 5 for selected
  move cases. That is not yet a complete semantic authority for replacing or
  demoting the public
  `PreparedFunctionLookups::edge_publication_source_producers` lookup.
- A supported x86 consumer path does not exist for the full relation. x86
  dispatch consumes copied `PreparedEdgePublication` fields, not the public
  source-producer map, and the Route 5 agreement gate is fail-closed only for
  the `LoadLocal` subset. Non-`LoadLocal` rows still fall back as
  compatibility behavior after failed agreement.

Blockers tied to the row map:

- Duplicate prepared rows, duplicate Route 5 rows, kind conflicts, label/value
  mismatches, missing prepared fields, and missing Route 5/BIR identity all
  need a same-identity x86 consumer that rejects disagreement instead of
  preserving compatibility fallback as proof.
- Prepared-only rows remain compatibility authority because x86 reads copied
  publication fields and no x86 boundary joins the public lookup to Route 5/BIR
  identity.
- Fallback rows for `LoadGlobal`, `Cast`, `Binary`,
  `SelectMaterialization`, `Immediate`, and `Unknown` are explicit blockers:
  the current x86 path may still emit after failed agreement and therefore
  cannot prove fail-closed semantics.
- `LoadLocal` memory-source rows provide only partial eligibility evidence.
  They do fail closed in the copied-publication local-slot path, but this does
  not cover the public lookup or the non-`LoadLocal` row families.
- Policy-sensitive rows block any migration into BIR or broad prepared
  retirement. Storage, addressing, helpers, formatting, wrappers, ABI,
  registers, stack layout, instruction spelling, emission policy, output
  contracts, and baseline behavior must remain target-owned.

No separate proof-only idea is needed now; this runbook already produced the
proof map needed to explain the blocker. A separate implementation idea should
be created only if the supervisor wants to pursue a complete x86 fail-closed
consumer boundary. That later idea would need to satisfy these minimum
conditions before implementation:

- name the concrete x86 consumer boundary for the public
  `edge_publication_source_producers` relation;
- join prepared source-producer rows to the same Route 5/BIR producer/source
  identity;
- reject disagreement across duplicate, conflict, mismatch, missing,
  prepared-only, fallback, `LoadLocal`, immediate, and policy-sensitive row
  families;
- preserve existing compatibility surfaces, statuses, route-debug output,
  prepared-printer output, wrappers, exact target output, unsupported behavior,
  baselines, and all target-owned policy.

## Suggested Next

Supervisor should route Step 5, `Validate the Map`, or ask the plan owner to
decide lifecycle closure/split after validation. Do not start implementation
from this active runbook.

## Watchouts

- Do not treat Step 4 as authorization to demote, hide, wrap, migrate, or
  implement `edge_publication_source_producers`.
- Non-`LoadLocal` compatibility fallback remains the key fail-closed blocker.
- RISC-V evidence remains comparison-only and does not satisfy the required x86
  consumer boundary.
- No implementation files, expectations, route-debug output, printer output,
  wrappers, or baselines were changed.

## Proof

Analysis-only proof delegated by supervisor for Step 4:

- `sed -n '1,220p' todo.md` after the update
- `git status --short`
- `git diff --check`

No build or CTest was required for this todo-only boundary packet. Per the
delegated proof command, no `test_after.log` update was made.
