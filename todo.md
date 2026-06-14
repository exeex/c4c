Status: Active
Source Idea Path: ideas/open/251_phase_f3_route45_edge_publication_parity_blocker_map.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Select One Candidate Edge-Publication Identity Fact

# Current Packet

## Just Finished

Completed `plan.md` Step 2, "Select One Candidate Edge-Publication Identity
Fact", as an analysis-only selection packet.

Selected narrow fact:

- Use the Route 5 CFG-edge publication source identity as the candidate
  Route 4/5 edge-publication fact. The selected fact is: for one predecessor
  block, successor block, and successor phi destination value,
  `Route5CfgEdgePublicationRecord` /
  `BirCfgEdgePublicationSourceIdentity` identifies the same destination value,
  incoming source value, source producer kind, source producer instruction
  identity, and edge labels as the prepared `PreparedEdgePublication` lookup
  answer for the same edge-publication key.
- Route 4 remains only the publication-availability/value context around the
  successor publication. The selected semantic boundary is not broad Route 4
  publication parity; it is the Route 5 edge/source identity that can be checked
  against a prepared edge-publication lookup answer.
- For a load-local publication source, the selected fact may include the Route
  5 `memory_source` row and its Route 3 `source_memory_access` identity, but
  only as evidence attached to the same edge/source publication. Broader
  source-memory parity remains outside this plan.

Agreement rule:

- Prepared side: `find_unique_indexed_prepared_edge_publication(...)` or the
  equivalent prepared lookup answer must produce exactly one
  `PreparedEdgePublication` with `PreparedEdgePublicationLookupStatus::Available`
  for the predecessor, successor, and destination publication key.
- Route/BIR side: `route5_find_cfg_edge_publication(...)` or
  `find_bir_cfg_edge_publication_source_identity(...)` must produce an
  available CFG-edge publication record for the same predecessor/successor
  edge and destination value. Route 5 status `Available` is agreement for
  non-memory source producers; Route 5 status `MemorySource` is agreement only
  when the attached Route 3 source-memory identity also agrees with the prepared
  publication's source-memory fact.
- Identity fields that must agree are predecessor label/id, successor label/id,
  destination value kind/name/type, source value kind/name/type or immediate,
  source producer kind, source producer instruction block/index when present,
  and, for load-local sources, the attached source-memory identity.
- Missing, duplicate, mismatched, unsupported, prepared-only, fallback, or
  policy-sensitive rows must fail closed: the prepared answer remains
  compatibility-visible, but it must not be promoted to shared Route 4/5
  semantic authority.

Semantic identity exclusions:

- Move selection, carrier/helper selection, register choice, source/destination
  home policy, stack/global layout, operand addressing, exact instruction
  spelling, exact status-string spelling, helper/oracle names, module-output
  text, and final output formatting are not part of the selected Route 4/5
  semantic fact.
- x86 `EdgePublicationMoveIntentStatus` rows, x86 module diagnostics, x86
  `mov` text and operand spelling, riscv `mv`/`li`/`lw` text, riscv register
  names, riscv stack offsets, source-home decisions, destination-home
  decisions, and unsupported/fallback publication names remain target policy or
  compatibility rows.
- Prepared `edge_publications` helper names, lookup key spelling, prepared
  status names, source-producer names, and source-memory status names remain
  public compatibility surfaces even if a later adapter proves agreement.

Why broader parity is outside this plan:

- This plan only needs one shared route/BIR edge-publication identity boundary
  before prepared `edge_publications` can be considered compatibility mirrors.
  Broad scalar/store publication plans, generic publication lowering, Route 4
  publication availability rewrites, all source-producer families, register
  allocation, carrier/helper selection, layout, and target emission policy
  would expand beyond the source idea and risk treating compatibility output as
  semantic proof.

## Suggested Next

Execute `plan.md` Step 3, "Prove or Block x86 Evidence", as an analysis-only
packet for the selected Route 5 CFG-edge publication source identity.

## Watchouts

- Keep source idea 251 unchanged unless durable intent truly changes.
- Do not implement an adapter during this blocker map.
- Step 3 should test x86 against the selected Route 5 CFG-edge source identity,
  not against scalar/store publication plans or module-output formatting.
- Preserve prepared edge-publication lookup/status, helper/oracle names,
  fallback publication rows, x86 dispatch/status/module output, riscv status
  fields, and riscv/x86 instruction/output strings as compatibility-owned
  unless direct route/BIR agreement proves otherwise.
- Treat testcase-shaped shortcuts, expectation weakening, helper renames, and
  output rewrites as route drift.

## Proof

No build or test proof required; analysis-only packet. Local validation:
`git diff --check -- todo.md`.
