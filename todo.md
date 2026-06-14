Status: Active
Source Idea Path: ideas/open/253_phase_f3_prepared_module_structural_exit_blocker_map.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Normalize The Blocker Map And Proof Needs

# Current Packet

## Just Finished

Step 6 - Normalize The Blocker Map And Proof Needs completed as an
analysis-only synthesis of Steps 1-5. This records future packet candidates
and blocked rows for `module`, `names`, `control_flow`, and
`store_source_publications`; it does not approve deletion, privatization,
wrapping, implementation, expectation rewrites, or compatibility weakening.

Normalized future packet candidates:

- `module` lookup-reader packet:
  Semantic fact: prepared function/block ids resolve to the same BIR
  function/block structure through function names, block label ids, and label
  spelling fallback.
  Consumer: `prepared_lookups.cpp::prepared_bir_function`,
  `prepared_bir_block`, `prepared_bir_block_label_id`, and lookup construction
  around `make_prepared_function_lookups`.
  Compatibility surface: current null return and `kInvalidBlockLabel` behavior
  for missing function ids, missing block ids, stale BIR labels, and
  label-string fallback.
  Fail-closed proof set: valid lookup, missing function id, missing block id,
  stale BIR label id, label-string fallback, and prepared/BIR label drift must
  return the same null/invalid result instead of inventing authority.
- `module` top-level printer packet:
  Semantic fact: complete BIR module text supplied to `bir::print`.
  Consumer: `prepared_printer.cpp::print` BIR body emission and
  non-empty-module blank-line check.
  Compatibility surface: exact prepared-printer section order, BIR text, and
  blank-line behavior.
  Fail-closed proof set: empty module, function-only module,
  global/string-constant module, and prepared phase/note header cases produce
  unchanged printer output.
- `names` same-block value-name lookup packet:
  Semantic fact: named BIR values map to existing prepared `ValueNameId`
  before source-producer, value-home, or integer-constant lookup.
  Consumer: `prepared_lookups.cpp::existing_prepared_value_name_id`,
  `prepared_result_matches_value_name`, `prepared_same_block_source_producer`,
  and `evaluate_prepared_same_block_integer_constant`.
  Compatibility surface: unnamed values, empty names, missing prepared names,
  stale producer indexes, type mismatches, and duplicate text conflicts return
  `nullopt`/false.
  Fail-closed proof set: valid named value, unnamed value, missing prepared id,
  stale producer record, wrong type, duplicate-same spelling, and prepared/BIR
  name drift cases must not select a different producer or value.
- `names` value-home lookup packet:
  Semantic fact: caller supplies or resolves one prepared `ValueNameId` before
  value-home lookup.
  Consumer: `value_locations.hpp::find_prepared_value_home(..., string_view)`
  and `find_prepared_value_home_for_bir_value`.
  Compatibility surface: null function locations, non-named BIR values, empty
  names, missing prepared names, and missing home rows return `nullptr`.
  Fail-closed proof set: valid home, missing name id, missing home, immediate
  value, empty named value, and stale indexed lookup cases preserve current
  null behavior.
- `names` semantic resolver API packet:
  Semantic fact: caller has an authoritative prepared id or explicit absent-id
  fact.
  Consumer: `control_flow.hpp::resolve_prepared_function_name_id`,
  `resolve_prepared_block_label_id`, and
  `resolve_prepared_value_name_id`.
  Compatibility surface: no interning on lookup, no fallback to raw BIR ids,
  and current missing-name behavior.
  Fail-closed proof set: present name, absent name, empty spelling, duplicate
  prepared table entries, and conflicting prepared/BIR spelling do not create
  new ids.
- `control_flow` call-preservation dominance packet:
  Semantic fact: route/BIR dominance over the same call-plan block indices
  plus same-block instruction ordering.
  Consumer: `prepared_lookups.cpp::find_dominating_indexed_prior_preserved_value`
  and `find_unique_indexed_prior_preserved_value_source`.
  Compatibility surface: null control-flow context, invalid value id, empty
  prior list, same-block later prior entry, unreachable or non-dominating
  blocks, duplicate prior entries, and missing preserved rows return `nullptr`
  or unavailable.
  Fail-closed proof set: present dominating prior, same-block earlier/later
  prior, no control-flow context, invalid block index, non-dominating
  predecessor, unreachable block, and duplicate prior source cases must not
  select a different source.
- `control_flow` branch-target identity packet:
  Semantic fact: BIR conditional terminator true/false `BlockLabelId` values
  under prepared agreement.
  Consumer: `control_flow.hpp::find_prepared_control_flow_branch_target_labels`
  and its agreement-gated overload.
  Compatibility surface: public helper fallback to prepared labels without an
  agreeing BIR context, plus current non-conditional, absent block, invalid
  labels, missing structured ids, mismatch, and non-agreement behavior.
  Fail-closed proof set: valid agreement, no function block,
  return/unconditional terminator, invalid prepared labels, invalid BIR ids,
  mismatched BIR ids, and conflicting structured/raw label cases preserve
  current `std::nullopt` or prepared fallback behavior.
- `control_flow` block-index label bridge packet:
  Semantic fact: call-plan block index names the same BIR block id as the
  prepared control-flow block at that index.
  Consumer: `call_plans.cpp::prepared_block_label_for_index`.
  Compatibility surface: existing fallback to `function.blocks[index].label_id`
  when control flow is absent or too short, and `kInvalidBlockLabel` for
  out-of-range indexes.
  Fail-closed proof set: prepared/BIR agreement, control flow absent, control
  flow shorter than BIR, BIR shorter than index, invalid BIR label id, and
  prepared/BIR mismatch cases.
- `store_source_publications` recovered-source identity packet:
  Semantic fact: a same-block wide load-local can recover the immediately
  prior narrower store-local value that targets the loaded byte.
  Consumer: prealloc publication population, AArch64 store-local publication
  planning, and AArch64 dispatch value materialization through
  `find_prepared_recovered_narrow_store_source_for_wide_local_load`.
  Compatibility surface: addressing, stack-layout slot/object checks,
  load-result type, lane offset parsing, store width, and instruction ordering
  remain authoritative.
  Fail-closed proof set: null block, null addressing, invalid block label,
  load/access mismatch, missing frame slot, non-integer load, no prior store,
  prior store/access mismatch, wrong byte lane, non-8-bit store, store width
  not narrower than load, and multiple earlier stores do not invent a recovered
  value.
- `store_source_publications` byval pointer-source classification packet:
  Semantic fact: the load-local source producer reads through a pointer-value
  memory access whose pointer value name is a byval formal.
  Consumer: prealloc publication population and AArch64 store-local source
  publication planning through
  `prepared_store_source_load_local_is_byval_formal_pointer_source`.
  Compatibility surface: prepared addressing and formal-name lookup remain
  authoritative.
  Fail-closed proof set: null addressing, null producer, wrong producer kind,
  missing load-local pointer, invalid producer block label, missing memory
  access, non-pointer base kind, missing pointer value name, base-plus-offset
  false, non-byval formal, and prepared/BIR name mismatch all return false.
- `store_source_publications` direct-global select-chain dependency packet:
  Semantic fact: the store source depends on a same-block select/direct-global
  load producer before the store.
  Consumer: prealloc publication population and AArch64 store-local
  publication planning through
  `find_prepared_store_source_direct_global_select_chain_dependency`.
  Compatibility surface: source producer lookups, block identity,
  before-instruction cutoff, root-is-select flag, and root instruction index
  remain row policy.
  Fail-closed proof set: null names/query, null source producers, invalid
  block label, null block, non-named value, unknown value name, producer after
  the store, producer in another block, missing direct-global load, mismatched
  select root, and missing root instruction index keep dependency fields
  false/empty.
- `store_source_publications` source-value/source-producer metadata packet:
  Semantic fact: the row carries the same BIR source value and route/prepared
  producer identity for the named store source.
  Consumer: `plan_prepared_store_source_publication` for prealloc records and
  AArch64 recomputed publication plans.
  Compatibility surface: status, intent, destination access, source home,
  storage encoding, recovered/direct-global flags, pointer-base homes, and
  duplicate/pending policy remain in the prepared planner.
  Fail-closed proof set: missing source, missing destination, invalid source
  value name, absent source home, unknown producer, wrong producer kind,
  producer block mismatch, source value mismatch, and prepared/BIR name
  mismatch preserve current status and empty producer fields.

Blocked rows:

- `module` phase mutation remains blocked because `run_legalize` and
  `run_out_of_ssa` mutate the BIR module as the phase output carrier. Missing
  field: a replacement mutable phase handoff fact and proof that phase output
  identity is unchanged.
- `module` contract-plan publishers remain blocked because each publisher
  needs a named semantic BIR fact, retained prepared aggregate/output adapter,
  and fail-closed proof for missing ids, stale labels, policy fallbacks, and
  printer stability before implementation.
- `module` x86/AArch64 backend readers remain blocked because they mix BIR
  structure with target policy, assembly/data output, target-triple fallback,
  and handoff validation. Missing adapter/proof: target-output-stable access
  that is not just the old public prepared authority behind a facade.
- `module` public aggregate deletion, privatization, wrapping, and broad
  retirement remain blocked by tests, handoff helpers, printer/module output,
  and phase handoff compatibility.
- `names` printer, debug, route-debug, and diagnostic rendering remain
  blocked because the missing proof is exact output stability for missing-id
  spelling, blank fields, row order, fallback text, and expected diagnostics.
- `names` target formatting remains blocked because AArch64/x86 label,
  symbol, global/string address, loop-countdown, pointer/global folding, and
  handoff validation readers are target-policy consumers. Missing adapter:
  target-owned formatting/fallback proof separate from semantic id lookup.
- `names` BIR/prepared block-label bridge removal remains blocked until
  duplicate, conflicting, missing, structured-id, raw-label fallback, and
  thrown handoff-error cases are proved fail-closed.
- `names` table construction, copy/move reattachment, public aggregate
  exposure, deletion, privatization, and replacement remain blocked by public
  compatibility and interning authority.
- `control_flow` AArch64 branch lowering, fused compare, dispatch, MIR
  successor metadata, and branch-target operand spelling remain blocked as
  target policy. Missing proof: exact emitted branch/successor/diagnostic
  stability, not just CFG identity.
- `control_flow` traversal, block order, x86/RISC-V handoff/setup, backend
  filters, and broad lowering rewrites remain blocked by layout and public
  aggregate compatibility.
- `control_flow` join-transfer, edge-transfer, publication, parallel-copy,
  cycle-temp, execution-site, and move-bundle readers remain blocked because
  the missing fact is not plain CFG; a later packet must name one row and prove
  status, move, cycle, execution-site, and publication fallback behavior.
- `control_flow` construction/mutation in `legalize.cpp`, `out_of_ssa.cpp`,
  `label_identity.cpp`, public tests, and prepared-printer/debug output remain
  blocked by construction authority and expected-text compatibility.
- `store_source_publications` whole-field deletion, privatization, wrapping,
  or aggregate retirement remains blocked because no non-printer production
  reader of stored records was found; the missing proof is confirmed consumer
  inventory plus an explicit printer compatibility adapter.
- `store_source_publications` printer rows remain blocked as byte-stable
  output compatibility, including status/intent strings, producer fields,
  booleans, and direct-global select-chain columns.
- `store_source_publications` status, intent, pending/duplicate,
  stack-homes-only, pointer-writeback, and availability gates remain blocked
  as publication compatibility. Missing proof: wrong intent, duplicate,
  non-pending/pending, unsupported intent, stack-only, and pointer-writeback
  rows fail closed without changing emitted publications.
- `store_source_publications` destination access, frame-slot/object, stack
  offset/size/align, pointer-base homes, storage encoding, volatile/access
  facts, memory operands, scratch/register policy, mnemonic selection, and
  emitted assembler text remain blocked as addressing/storage output and target
  fallback policy.
- Cross-field broad prepared aggregate exits remain blocked unless a later
  packet names one semantic fact, one consumer or helper row, one retained
  compatibility adapter, and a complete fail-closed proof set. Classification
  tables, helper renames, output rewrites, expectation downgrades, or
  field-shaped shortcuts are not structural exit progress.

## Suggested Next

Supervisor should choose one future one-reader/helper-row implementation
packet from the normalized candidates, or call plan-owner for lifecycle
completion if the analysis runbook is considered exhausted. A small first code
packet should prefer a helper whose semantic fact and fail-closed proof are
already concrete, such as a `names` resolver/value-home lookup, a
`control_flow` branch-target helper, or a store-source helper row.

## Watchouts

This map is an analysis-only handoff. It preserves public prepared aggregate
compatibility and does not approve deletion, privatization, wrapping, broad
`PreparedBirModule` retirement, target-policy movement into BIR, printer/debug
string rewrites, testcase-shaped shortcuts, or implementation changes. Any
future packet must keep missing, invalid, mismatch, duplicate, fallback, and
policy-sensitive rows fail-closed before claiming structural exit progress.

## Proof

Analysis-only packet; no build or test proof required. Ran
`git diff --check -- todo.md`.
