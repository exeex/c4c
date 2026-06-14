Status: Active
Source Idea Path: ideas/open/251_phase_f3_route45_edge_publication_parity_blocker_map.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Decide Adapter Readiness and Close or Split

# Current Packet

## Just Finished

Completed `plan.md` Step 6, "Decide Adapter Readiness and Close or Split", as
an analysis-only readiness decision for the selected Route 5 CFG-edge
publication source identity.

Selected fact evaluated:

- Route 5 CFG-edge publication source identity checked against prepared
  `PreparedEdgePublication` lookup agreement for the same predecessor,
  successor, destination value, source value, and source producer.
- Route 4 remains publication-availability/value context only for this plan;
  the selected adapter boundary is Route 5 agreement or rejection.
- For dynamic `LoadLocal` publication sources, Route 5 agreement also requires
  the Route 3 source-memory identity to agree with the prepared source-memory
  publication before it can be treated as an agreeing diagnostic row.

Adapter readiness decision: blocked.

One shared Route 4/5 edge-publication identity is not proven across named x86
and riscv evidence:

- x86 is blocked, not non-applicable. It consumes prepared edge-publication
  lookup/status data through `consume_edge_publication_move_intent(...)` and
  prepared-backed module output in `module.cpp`, but no x86 consumer directly
  or indirectly reads `Route5CfgEdgePublicationRecord` /
  `BirCfgEdgePublicationSourceIdentity` and joins the same-edge prepared
  publication to reject disagreement.
- riscv provides diagnostic evidence for the selected Route 5 fact through
  `route5_edge_status`, `route5_edge_source_agrees`, and, for dynamic
  `LoadLocal` sources, `route3_source_memory_agrees`; however prepared
  lookup/status and target emission still remain authoritative.
- Route 4 provides only publication-availability/value context in this plan;
  it is not enough by itself to claim migrated semantic authority for prepared
  edge-publication lookup answers.

Exact missing x86 bridge:

- A narrow x86 Route 5/BIR agreement consumer or MIR query facade that can join
  the same predecessor/successor/destination/source/source-producer
  `Route5CfgEdgePublicationRecord` to the prepared `PreparedEdgePublication`
  lookup row used by `consume_edge_publication_move_intent(...)`.
- The bridge must fail closed when Route 5 is duplicate, mismatched, absent,
  unsupported, prepared-only, or, for dynamic `LoadLocal` publication sources,
  when Route 3 source-memory identity disagrees or is incomplete.

Fail-closed rows that prevent adapter readiness:

| Case | Prepared compatibility surface that stays observable | Required Route 4/5 agreement or rejection | Common or target-specific | Existing evidence |
| --- | --- | --- | --- | --- |
| Duplicate Route 5 edge row | Prepared lookup remains `prepare::find_unique_indexed_prepared_edge_publication(...)`; target status/output stays prepared-backed. | Route 5 must reject ambiguity by not producing an agreeing semantic row for the same predecessor/successor/destination; duplicates must not be accepted as semantic authority. | Common semantic rejection; target output remains target-specific. | `check_route5_route3_oracle_rows_preserve_prepared_riscv_fallback(...)` constructs a duplicate Route 5 index as a diagnostic fixture, but existing proof is partial because no current adapter consumes a duplicate Route 5 rejection. x86 remains blocked because it has no Route 5 consumer to observe this case. |
| Route 5 mismatch | Prepared publication lookup/status and old fallback output stay observable. | Route 5 `NoMatch` or equivalent mismatch must set agreement false and must not override prepared output. | Common semantic rejection. | RISC-V test uses a mismatched destination type and expects `Route5PublicationStatus::NoMatch`, `route5_edge_source_agrees == false`, while preserving `mv a1, a0`. x86 has no direct or indirect `Route5CfgEdgePublicationRecord` / `BirCfgEdgePublicationSourceIdentity` consumer. |
| Absent or wrong-edge source | Prepared missing-publication and prepared fallback rows stay observable. | Route 5 `NoSource`, `MissingPublication`, `MissingPredecessor`, `MissingSuccessor`, or `MissingDestination` must reject agreement and cannot synthesize a source identity. | Common semantic rejection. | RISC-V test uses a wrong predecessor and expects `Route5PublicationStatus::NoSource`; prepared missing rows still produce `EdgePublicationMoveIntentStatus::MissingPublication`. Prepared lookup helper tests cover Route 5 missing-successor/no-source rows. x86 preserves prepared status rows but lacks Route 5 agreement checks. |
| Unsupported prepared publication | `EdgePublicationMoveIntentStatus::UnsupportedPublication` remains observable; prepared move/op-kind compatibility is unchanged. | Route 5 cannot make unsupported prepared move shapes available; agreement may be diagnostic only after the prepared publication itself is supported. | Target-specific compatibility gate over common identity. | x86 `consume_edge_publication_move_intent(...)` and riscv `consume_prepared_backed_move_intent(...)` both return `UnsupportedPublication` when the prepared lookup row is not an available move. RISC-V tests cover non-move publication rejection. |
| Prepared-only publication | Public prepared `edge_publications` lookup, helper/oracle names, and missing Route 5 diagnostic fields stay observable. | Without a matching Route 5/BIR row, the adapter must remain compatibility-owned or mark agreement false; no prepared-only row can become semantic authority. | Common adapter gate; target output stays target-specific. | RISC-V overload without a Route 5 pointer still emits prepared-backed output and leaves Route 5 diagnostics non-authoritative; missing shared lookups/publication rows fail closed. x86 currently exists only in this prepared-only class for selected Route 5 agreement. |
| Prepared fallback on non-agreeing Route 5/Route 3 facts | Prepared-backed instruction/status rows remain observable, including scalar fallback and dynamic source-memory fallback. | Route 5 `NoMatch` and Route 5 `MemorySource` with Route 3 disagreement/incomplete source-memory identity must set agreement false and preserve prepared fallback instead of weakening expectations. | Common rejection for Route 5/Route 3 identity; target-specific fallback text. | RISC-V tests preserve `mv a1, a0` on Route 5 mismatch and `lw a1, 12(s2)` on Route 3 offset mismatch or incomplete Route 3 source-memory identity, with `route5_edge_source_agrees == false` and `route3_source_memory_agrees == false`. |
| Policy-sensitive output | x86 dispatch/status/module output, riscv status fields, exact instruction text, register choices, helper/oracle names, and formatting stay observable. | Route 5 agreement only owns source identity. It must not own move selection, carrier/helper selection, register choice, stack/source-home support, immediate materialization, scratch policy, offsets, or textual output. | Target-specific policy. | RISC-V rows include `mv`, `lw`, `li`, `addi`, `ld`, `sw`, register names such as `a0`, `a1`, `s2`, `t0`, `t6`, and source/destination-home status rows. x86 rows include `mov` operands, prepared dispatch status, and module output in `module.cpp`; these remain compatibility output, not Route 5 authority. |

Acceptance/rejection rule for a later adapter:

- Accept only when prepared lookup is present and supported, Route 5 identifies
  the same predecessor/successor/destination/source/source-producer row, and
  dynamic `LoadLocal` source rows also pass Route 3 source-memory agreement.
- Reject or remain prepared-backed for duplicate, mismatch, absent, unsupported,
  prepared-only, non-agreeing fallback, and policy-sensitive rows.
- Do not weaken existing prepared status/output expectations, rewrite helper
  names, or add named-case shortcuts to call a row migrated.

Preserved classifications:

- Step 3 x86 remains blocked, not non-applicable: x86 consumes prepared
  edge-publication lookup/status data through
  `consume_edge_publication_move_intent(...)` and `module.cpp`, but it has no
  direct or indirect Route 5/BIR agreement consumer that joins the same-edge
  prepared publication and rejects disagreement.
- Step 4 riscv remains diagnostic-only: riscv exposes
  `route5_edge_status`, `route5_edge_source_agrees`, and
  `route3_source_memory_agrees`, but prepared lookup/status and target
  emission remain authoritative.

## Suggested Next

Ask the plan owner to close idea 251 as a completed blocker map and, if the
supervisor accepts the blocked conclusion, create a separate narrow follow-up
implementation idea for the x86 Route 5 `PreparedEdgePublication` agreement
bridge.

## Watchouts

- Keep source idea 251 unchanged unless durable intent truly changes.
- Do not implement an adapter during this blocker map; the safe follow-up is a
  separate implementation idea.
- The fail-closed matrix is sufficient to reject expectation weakening,
  helper/status renames, named-case shortcuts, and old-failure retention as
  proof of migration.
- Keep the x86 result classified as blocked unless a later implementation
  introduces a Route 5/BIR agreement consumer or MIR query facade for x86.
- Keep the riscv result diagnostic-only unless a later plan explicitly moves
  authority behind a fail-closed adapter.
- Preserve prepared edge-publication lookup/status, helper/oracle names,
  fallback publication rows, x86 dispatch/status/module output, riscv status
  fields, and riscv/x86 instruction/output strings as compatibility-owned.
- Treat testcase-shaped shortcuts, expectation weakening, helper renames, and
  output rewrites as route drift.

## Proof

No build or test proof required; analysis-only packet. This packet synthesized
Steps 3-5 and did not inspect new implementation files.

Local validation: `git diff --check -- todo.md`.
