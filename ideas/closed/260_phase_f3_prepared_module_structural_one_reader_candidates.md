# 260 Phase F3 prepared module structural one-reader candidates

## Idea Type

bounded implementation queue.

## Goal

Implement at most one prepared-module structural exit candidate at a time from
the blocker map produced by closed idea 253, limited to a named reader or
helper row with concrete semantic authority and complete fail-closed proof.

## Why This Exists

Idea 253 completed the analysis-only blocker map for
`PreparedBirModule::module`, `names`, `control_flow`, and
`store_source_publications`. That map found some bounded future packets, but it
also proved that the broader fields are not deletion-ready and that public
prepared aggregate compatibility remains authoritative. This follow-up keeps
future implementation work activatable without expanding closed idea 253 or
re-deriving the map.

## In Scope

Choose exactly one candidate per active runbook unless a plan-owner split
narrows it further:

- `module` lookup-reader packet:
  `prepared_bir_function`, `prepared_bir_block`,
  `prepared_bir_block_label_id`, and lookup construction may use a BIR
  function/block structure fact only when current null and
  `kInvalidBlockLabel` behavior is preserved for missing ids, stale BIR
  labels, label-string fallback, and prepared/BIR label drift.
- `module` top-level printer packet:
  prepared-printer BIR body emission may use complete BIR module text only
  when section order, BIR text, and blank-line behavior remain byte-stable for
  empty, function-only, global/string-constant, phase, and note-header cases.
- `names` same-block value-name lookup packet:
  same-block producer and integer-constant lookup may use named BIR values only
  when unnamed, empty, missing prepared id, stale producer, wrong type,
  duplicate spelling, and prepared/BIR name-drift rows fail closed.
- `names` value-home lookup packet:
  value-home lookup may use one authoritative prepared `ValueNameId` or absent
  id fact only when null function locations, immediate values, empty names,
  missing prepared names, missing homes, and stale indexed lookups preserve
  current null behavior.
- `names` semantic resolver API packet:
  prepared function, block-label, and value-name resolver helpers may expose an
  authoritative prepared id or absent-id fact only when lookup never interns,
  never falls back to raw BIR ids, and duplicate, empty, absent, and
  conflicting spelling rows fail closed.
- `control_flow` call-preservation dominance packet:
  prior preserved-value lookup may use route/BIR dominance over call-plan block
  indices plus same-block instruction ordering only when null context, invalid
  ids, empty prior lists, later same-block entries, non-dominating or
  unreachable blocks, duplicate prior entries, and missing preserved rows
  return current null or unavailable results.
- `control_flow` branch-target identity packet:
  prepared branch-target helpers may use BIR conditional true/false
  `BlockLabelId` values only under prepared agreement, while preserving public
  fallback to prepared labels and fail-closed behavior for non-conditional,
  absent, invalid, missing structured-id, mismatch, and non-agreement rows.
- `control_flow` block-index label bridge packet:
  `prepared_block_label_for_index` may use a BIR block-id agreement fact only
  when control-flow absent, control-flow shorter than BIR, BIR shorter than
  index, invalid BIR labels, and prepared/BIR mismatch keep current fallback or
  invalid-label behavior.
- `store_source_publications` recovered-source identity packet:
  recovered narrow-store source lookup may use same-block load/store identity
  only when addressing, frame-slot/object checks, load-result type, lane
  offset parsing, store width, instruction ordering, and all missing/mismatch
  rows fail closed.
- `store_source_publications` byval pointer-source classification packet:
  byval formal pointer-source classification may use a pointer-value memory
  access and formal-name fact only when prepared addressing and formal-name
  lookup remain authoritative and all wrong-kind, missing, base-plus-offset,
  non-byval, and prepared/BIR mismatch rows return false.
- `store_source_publications` direct-global select-chain dependency packet:
  select/direct-global dependency lookup may use same-block producer identity
  only when source-producer lookup, block identity, before-instruction cutoff,
  root-is-select, root index, and all missing/mismatch/after-store rows keep
  dependency fields false or empty.
- `store_source_publications` source-value/source-producer metadata packet:
  publication planning may use source value and route/prepared producer
  identity only when status, intent, destination access, source home, storage
  encoding, recovered/direct-global flags, pointer-base homes, duplicate, and
  pending policy remain in the prepared planner and fail closed.

## Out Of Scope

- Deleting, privatizing, wrapping, or broadly retiring any
  `PreparedBirModule` field.
- Implementing more than one unrelated reader/helper row in one packet.
- Moving target policy, formatting policy, printer/debug strings, route-debug
  strings, fallback names, or target output into BIR.
- Rewriting expectations, baselines, helper/oracle statuses, diagnostics, or
  output strings to claim progress.
- Treating classification tables, helper renames, or facade/wrapper movement
  as structural exit progress.

## Blocked Rows To Preserve

- `module` phase mutation, contract-plan publishers, x86/AArch64 backend
  readers, public aggregate exposure, deletion, privatization, wrapping, and
  broad retirement remain blocked until a replacement handoff fact, retained
  adapter, and output-stable proof are concrete.
- `names` printer/debug/route-debug rendering, target formatting,
  BIR/prepared block-label bridge removal, table construction, copy/move
  reattachment, public aggregate exposure, deletion, privatization, and
  replacement remain blocked until exact output and interning/fallback
  compatibility are proved.
- `control_flow` AArch64 branch lowering, fused compare, dispatch, MIR
  successor metadata, operand spelling, traversal/layout, x86/RISC-V setup,
  join/edge transfer, publication, parallel-copy, cycle-temp, execution-site,
  move-bundle readers, construction/mutation, public tests, and
  printer/debug output remain blocked unless one row names its specific fact,
  consumer, adapter, and fail-closed proof.
- `store_source_publications` whole-field deletion, privatization, wrapping,
  aggregate retirement, printer rows, status/intent/pending/duplicate policy,
  stack-homes-only, pointer-writeback, destination access, frame-slot/object,
  stack offset/size/align, pointer-base homes, storage encoding,
  volatile/access facts, memory operands, scratch/register policy, mnemonic
  selection, and emitted assembler text remain blocked as compatibility,
  addressing, storage-output, or target-policy authority.
- Cross-field broad exits remain blocked unless a later packet names one
  semantic fact, one consumer or helper row, one retained compatibility adapter,
  and a complete fail-closed proof set.

## Completed Candidate Notes

- `module` lookup-reader packet completed in the retired active runbook:
  `prepared_bir_function`, `prepared_bir_block`,
  `prepared_bir_block_label_id`, and lookup construction now use a shared BIR
  function/block structure agreement helper only when the prepared lookup path
  would otherwise be null. Public prepared aggregate fields and existing
  prepared lookup maps remain observable.
- Focused proof covered the positive structured-agreement path plus
  fail-closed rows for invalid prepared function ids, absent BIR
  function/module structure, invalid prepared block ids, missing prepared-name
  lookup rows, stale or unknown structured ids, prepared/BIR drift, duplicate
  or conflicting labels, and retained public raw-label compatibility.
- Broader close-readiness proof passed with 180/180 default backend tests and
  2/2 x86 route-debug/handoff tests in the canonical before/after logs.
- Remaining candidates in this idea stay open and require their own future
  one-candidate runbooks; this completed packet is not approval for broad
  `PreparedBirModule` deletion, privatization, wrapping, aggregate retirement,
  or migration of unrelated `module`, `names`, `control_flow`, or
  `store_source_publications` rows.
- `module` top-level printer packet completed in the retired active runbook:
  prepared-printer BIR body emission now uses a local complete-module-text
  agreement boundary only when prepared-printer section order, BIR body text,
  header placement, note output, phase output, and blank-line behavior remain
  byte-stable.
- Focused proof covered empty modules, function-only output, multiple-function
  spacing, global-only and string-constant-only compatibility rows, function
  plus global/string compatibility spacing, phase headers, note headers,
  invariant header placement, and post-body spacing.
- Broader close-readiness proof passed with 180/180 default backend tests and
  2/2 x86 route-debug/handoff tests in the canonical before/after logs.
- Remaining candidates in this idea stay open and require their own future
  one-candidate runbooks; this completed printer packet is not approval for
  broad `PreparedBirModule` deletion, privatization, wrapping, aggregate
  retirement, direct global/string text emission, or migration of unrelated
  `module`, `names`, `control_flow`, or `store_source_publications` rows.
- `names` same-block value-name lookup packet completed in the retired active
  runbook: same-block producer, integer-constant, and selected select-chain
  source-producer lookup now use a shared prepared/BIR value-name agreement
  helper only when the named BIR value, prepared `ValueNameId`, producer
  identity, result type, same-block location, and before-instruction cutoff
  agree.
- Focused proof covered the positive structured-agreement path plus
  fail-closed rows for unnamed values, empty names, missing prepared ids,
  stale producers, wrong result types, duplicate spelling, prepared/BIR
  name-drift, drifted producer results, and retained public raw-spelling
  compatibility.
- Broader close-readiness proof passed with 180/180 default backend tests in
  the canonical before/after logs, and the regression guard reported no new
  failures.
- Remaining candidates in this idea stay open and require their own future
  one-candidate runbooks; this completed same-block lookup packet is not
  approval for broad `PreparedBirModule` deletion, privatization, wrapping,
  aggregate retirement, or migration of unrelated `names`, `module`,
  `control_flow`, or `store_source_publications` rows.
- `names` value-home lookup packet completed in the retired active runbook:
  value-home lookup now uses a prepared/BIR value-name agreement helper only
  when the named BIR value, prepared `ValueNameId`, prepared `PreparedValueId`,
  indexed value-home row, and backing prepared value-home record agree.
- Focused proof covered the positive structured-agreement path plus
  fail-closed rows for null function locations, immediate values, empty names,
  missing prepared ids, uninterned prepared names, missing indexed homes,
  missing backing homes, stale value ids, stale home payloads, duplicate or
  conflicting names and ids, conflicting regalloc rows, prepared/BIR
  name-drift, and retained public prepared-lookup compatibility.
- Broader close-readiness proof passed with 180/180 default backend tests in
  the canonical before/after logs, and the regression guard reported no new
  failures.
- Remaining candidates in this idea stay open and require their own future
  one-candidate runbooks; this completed value-home lookup packet is not
  approval for broad `PreparedBirModule` deletion, privatization, wrapping,
  aggregate retirement, or migration of unrelated `names`, `module`,
  `control_flow`, or `store_source_publications` rows.
- `names` semantic resolver API packet completed in the retired active
  runbook: prepared function-name, block-label, and value-name resolver
  agreement helpers now report authoritative prepared ids only when prepared
  names, raw spellings or ids where applicable, and prepared-table round trips
  agree without interning during resolver queries.
- Focused proof covered positive prepared function-name, block-label, and
  value-name rows plus fail-closed rows for empty spellings, absent prepared
  names, immediate values, invalid and out-of-range raw block-label ids,
  raw-only missing prepared ids, prepared/BIR spelling drift, corrupted
  prepared-table round-trip mismatches, repeated non-interning queries, and
  retained direct `names.*.find(...)` compatibility.
- Broader close-readiness proof passed with 180/180 default backend tests in
  the canonical before/after logs, and the regression guard reported no new
  failures.
- Remaining candidates in this idea stay open and require their own future
  one-candidate runbooks; this completed semantic resolver packet is not
  approval for broad `PreparedBirModule` deletion, privatization, wrapping,
  aggregate retirement, construction-time interning changes, or migration of
  unrelated `names`, `module`, `control_flow`, or
  `store_source_publications` rows.
- `control_flow` call-preservation dominance packet completed in the retired
  active runbook: prior preserved-value lookup now uses a local
  prepared/control-flow agreement boundary only when the prepared preservation
  row, value id, source pointer, source call location, and query call location
  are complete and agree with same-block instruction ordering or
  route/control-flow dominance.
- Focused proof covered valid same-block and cross-block prior-preservation
  selection plus fail-closed rows for null context, invalid ids, empty prior
  lists, later same-block entries, non-dominating or unreachable rows,
  duplicate candidate positions, missing preserved pointers, incomplete
  preservation metadata, and non-`Found` consumer fallthrough.
- Broader close-readiness proof passed with 180/180 default backend tests in
  the canonical before/after logs, and the regression guard reported no new
  failures.
- Remaining candidates in this idea stay open and require their own future
  one-candidate runbooks; this completed control-flow call-preservation packet
  is not approval for broad `PreparedBirModule` deletion, privatization,
  wrapping, aggregate retirement, or migration of unrelated `module`, `names`,
  `control_flow`, or `store_source_publications` rows.
- `control_flow` branch-target identity packet completed in the retired
  active runbook: prepared branch-target helpers now use BIR conditional
  true/false `BlockLabelId` identity only when the prepared control-flow row
  and structured BIR label ids agree, while preserving public prepared-label
  fallback.
- Focused proof covered positive prepared/BIR agreement plus fail-closed rows
  for absent private BIR context, raw-name drift or non-agreement, absent
  prepared blocks, absent prepared control-flow rows, invalid prepared target
  labels, invalid BIR structured ids, structured-id mismatch, private
  invalid/conflict rejection, non-conditional BIR fallback, missing structured
  label ids, and invalid prepared source labels.
- Broader close-readiness proof passed with 180/180 default backend tests in
  the canonical before/after logs, and the regression guard reported no new
  failures.
- Remaining candidates in this idea stay open and require their own future
  one-candidate runbooks; this completed control-flow branch-target packet is
  not approval for broad `PreparedBirModule` deletion, privatization,
  wrapping, aggregate retirement, or migration of unrelated `module`, `names`,
  `control_flow`, or `store_source_publications` rows.
- `control_flow` block-index label bridge packet completed in the retired
  active runbook: `prepared_block_label_for_index` now uses BIR block-id /
  prepared block-label agreement only when the prepared control-flow row,
  prepared label id, BIR block index, and structured BIR block-label id are
  complete and agree, while preserving prepared fallback and invalid-label
  behavior.
- Focused proof covered the positive agreement path plus fail-closed rows for
  absent prepared control-flow rows, invalid prepared target labels, missing
  structured BIR label ids, BIR-shorter public call-plan selection, and
  prepared/BIR mismatch.
- Broader close-readiness proof passed with 180/180 default backend tests in
  the canonical before/after logs, and the regression guard reported no new
  failures.
- Remaining candidates in this idea stay open and require their own future
  one-candidate runbooks; this completed block-index label bridge packet is
  not approval for broad `PreparedBirModule` deletion, privatization,
  wrapping, aggregate retirement, or migration of unrelated `module`, `names`,
  `control_flow`, or `store_source_publications` rows.
- `store_source_publications` recovered-source identity packet completed in
  the retired active runbook: recovered narrow-store source lookup now accepts
  same-block BIR load/store identity only behind a local prepared/BIR
  agreement boundary, while preserving prepared recovered-source publication
  planning as the policy owner.
- Focused proof covered the positive prepared/BIR recovered-source agreement
  path plus fail-closed rows for absent BIR identity fallback, complete BIR
  identity mismatch, frame-slot or object mismatch, lane offset mismatch,
  store width mismatch, wrong stored value, after-store ordering, missing
  memory-access evidence, and nearby public publication-planner compatibility.
- Broader close-readiness proof passed with 180/180 default backend tests in
  the canonical before/after logs, and the regression guard reported no new
  failures.
- Remaining candidates in this idea stay open and require their own future
  one-candidate runbooks; this completed recovered-source packet is not
  approval for broad `PreparedBirModule` deletion, privatization, wrapping,
  aggregate retirement, or migration of unrelated `module`, `names`,
  `control_flow`, or `store_source_publications` rows.
- `store_source_publications` byval pointer-source classification packet
  completed in the retired active runbook: byval formal pointer-source
  classification now requires the prepared memory-access result to agree with
  the BIR load-local result, while the prepared pointer-value address name
  remains the authority for the byval formal decision.
- Focused proof covered the positive prepared agreement path plus fail-closed
  rows for absent addressing, absent source producer, wrong-kind producer,
  invalid/conflicting block identity, missing prepared memory access,
  non-pointer-value addressing, missing pointer name, base-plus-offset
  rejection, non-byval formal, missing formal, and prepared/BIR name mismatch.
- Broader close-readiness proof passed with 180/180 default backend tests in
  the canonical before/after logs, and the regression guard reported no new
  failures.
- Remaining candidates in this idea stay open and require their own future
  one-candidate runbooks; this completed byval packet is not approval for the
  direct-global select-chain dependency packet, the source-value/source-producer
  metadata packet, broad `PreparedBirModule` deletion, privatization, wrapping,
  aggregate retirement, or migration of unrelated `module`, `names`,
  `control_flow`, or `store_source_publications` rows.
- `store_source_publications` direct-global select-chain dependency packet
  completed in the retired active runbook: direct-global select-chain
  dependency lookup now reports dependency fields only when the prepared
  source-producer row, BIR value identity, block identity, root producer index,
  and consumer cutoff agree, while preserving prepared publication and
  call-plan policy ownership.
- Focused proof covered the positive prepared agreement path plus fail-closed
  rows for absent or stale source-producer lookup, block conflict, value drift,
  incomplete producer payloads, invalid or absent block identity, null block,
  root-after-consumer cutoff, wrong producer kind, missing child payload,
  conflicting producer-map entries, non-direct-global select chains,
  child-after-root cutoff, and public store-source alias compatibility.
- Broader close-readiness proof passed with 180/180 default backend tests in
  the canonical before/after logs, and the regression guard reported no new
  failures.
- Remaining candidates in this idea stay open and require their own future
  one-candidate runbooks; this completed direct-global packet is not approval
  for the source-value/source-producer metadata packet, broad
  `PreparedBirModule` deletion, privatization, wrapping, aggregate retirement,
  or migration of unrelated `module`, `names`, `control_flow`, or
  `store_source_publications` rows.
- `store_source_publications` source-value/source-producer metadata packet
  completed in the closed active runbook: store-source publication planning
  now publishes source value and source-producer metadata only behind complete
  prepared planner agreement, while leaving status, intent, destination
  access, source home, storage encoding, recovered/direct-global flags,
  pointer-base homes, pending policy, and duplicate policy under prepared
  planner authority.
- Focused proof covered populated source metadata plus fail-closed rows for
  missing source values, missing or stale source homes, stale prepared value
  ids, invalid or drifted source value names, incomplete or mismatched producer
  payloads, wrong producer kinds, block and instruction-order mismatches,
  recovered/direct-global/pointer-base disagreements, pending-publication
  policy, duplicate-publication policy, storage-policy rejection, and byval
  load-local authority.
- Broader close-readiness proof passed with 180/180 default backend tests in
  the canonical before/after logs, and the regression guard reported no new
  failures.
- All candidates named by this bounded implementation queue have now been
  completed through one-candidate runbooks. This closure is not approval for
  broad `PreparedBirModule` deletion, privatization, wrapping, aggregate
  retirement, expectation rewrites, or migration of unrelated backend,
  printer, debug, fallback, target-output, or policy rows.

## Acceptance Criteria

- The active packet chooses one named candidate and keeps the implementation
  limited to that candidate's reader/helper row.
- The patch identifies the semantic fact being used and the retained prepared
  compatibility surface that remains authoritative.
- Missing, invalid, mismatch, duplicate, fallback, unsupported, prepared-only,
  route/BIR-only, and policy-sensitive rows fail closed without weaker
  diagnostics, status names, printer/debug output, route-debug output, helper
  output, or target output.
- Public prepared aggregate compatibility remains observable unless a retained
  adapter is explicitly specified and proved.
- The proof covers the focused candidate and nearby same-feature fail-closed
  rows; a single named testcase or expectation rewrite is not sufficient.

## Reviewer Reject Signals

- Reject testcase-shaped shortcuts that special-case one function, block label,
  name, source-publication row, target, or expected string while claiming
  structural exit progress.
- Reject unsupported expectation downgrades, weaker printer/debug contracts,
  route-debug/fallback string rewrites, helper/oracle status renames, or
  target-output baseline rewrites without explicit approval.
- Reject helper renames, accessor renames, classification-only tables, facade
  movement, or wrappers claimed as semantic ownership transfer.
- Reject broad `PreparedBirModule`, `PreparedFunctionLookups`, naming, CFG,
  module traversal, store-source, backend, printer, debug, fallback, or target
  rewrites outside the selected one-reader/helper-row candidate.
- Reject routes that preserve the exact old public prepared authority behind a
  new BIR, route, private-accessor, adapter, or wrapper name.
- Reject any route that uses idea 253 as approval for deletion,
  privatization, wrapping, broad aggregate retirement, implementation outside
  the selected candidate, expectation rewrites, or compatibility weakening.
