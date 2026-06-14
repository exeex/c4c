Status: Active
Source Idea Path: ideas/open/253_phase_f3_prepared_module_structural_exit_blocker_map.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Map `control_flow` Structural Exit Preconditions

# Current Packet

## Just Finished

Step 4 - Map `control_flow` Structural Exit Preconditions completed as an
analysis-only map for `PreparedBirModule::control_flow`.

`control_flow` structural exit map:

- CFG identity and block-index readers:
  `control_flow.hpp::find_prepared_control_flow_function`,
  `find_prepared_control_flow_block`,
  `find_prepared_linear_join_predecessor`, and
  `find_prepared_control_flow_branch_target_labels` are the direct prepared
  CFG lookup helpers. `prepared_lookups.cpp::prepared_block_index_by_label`,
  `prepared_block_successors`, `make_prepared_dominance_matrix`, and
  `prepared_block_dominates` derive block-index successor and dominance facts
  from prepared blocks. `call_plans.cpp::prepared_block_label_for_index` is a
  block-index compatibility bridge that already falls back to BIR block ids.
  These are semantic-reader candidates only when the replacement fact is the
  same function/block identity, terminator successor identity, or dominance
  relation and the missing/invalid/mismatch behavior remains fail-closed.
- Dominance consumers:
  `prepared_lookups.cpp::make_prepared_call_plan_lookups`,
  `collect_block_entry_republication_effects`,
  `find_dominating_indexed_prior_preserved_value`, and
  `find_unique_indexed_prior_preserved_value_source` use prepared dominance to
  decide whether prior call-preservation or republication facts reach a later
  call. This is the clearest bounded semantic exit family: the reader needs a
  route/BIR dominance fact over the same block indices and instruction
  positions, while duplicate rows, missing control-flow context, invalid block
  indices, unreachable blocks, and non-dominating prior entries keep returning
  no source instead of inventing a preserved value.
- Branch-target identity readers:
  `control_flow.hpp::find_prepared_control_flow_branch_target_labels` and the
  overload with `detail::read_agreeing_bir_branch_target_labels` are already
  agreement-gated for BIR structured successor ids. The semantic fact is BIR
  terminator true/false `BlockLabelId` identity; the compatibility surface is
  the public helper fallback to prepared labels when there is no agreeing BIR
  context. This family is bounded, but later packets must stay helper-row
  scoped and preserve non-conditional, absent, invalid, mismatch, and
  non-agreement fallback behavior.
- Branch lowering and instruction-target policy:
  AArch64 `comparison.cpp`, `dispatch.cpp`, `dispatch.hpp`,
  `dispatch_edge_copies.cpp`, and `instruction.cpp` read
  `PreparedControlFlowBlock` labels, `PreparedBranchCondition`, and resolved
  branch targets to form branch target operands, conditional/unconditional MIR
  successors, fused-compare branch records, late conditional dispatch, and
  target diagnostics. These are target-policy consumers, not structural exit
  candidates. They own emitted branch spelling, successor metadata, target
  diagnostics, fallback paths, and expected-string stability.
- Layout/order and target handoff consumers:
  AArch64 `traversal.cpp::lower_prepared_functions` iterates
  `control_flow.functions` and each function's `blocks` to create target MIR
  function/block order, block indices, block lowering contexts, and lookup
  packages. x86 `consume_plans`/`consume_prepared_function_lookups` and
  `module.cpp::validate_prepared_control_flow_handoff` use the field as a
  handoff completeness gate and throw on missing function/block/target
  agreement. RISC-V prepared emission keeps prepared lookup setup tied to
  function control-flow. These layout and handoff paths are compatibility or
  target-owned; broad traversal, block-order, or lowering rewrites are out of
  Step 4 scope.
- Join, edge-publication, and parallel-copy policy:
  `out_of_ssa.cpp` consumes prepared predecessor/successor counts to classify
  parallel-copy execution sites, uses `find_prepared_linear_join_predecessor`
  to annotate branch-owned join transfers, and publishes
  `parallel_copy_bundles`. `prepared_lookups.cpp::make_prepared_edge_publication_lookups`,
  `find_published_parallel_copy_bundle_for_edge_transfer`, publication-source
  lookups, and AArch64 dispatch producer/edge-copy readers consume
  join-transfer, edge-transfer, move-bundle, cycle-temp, execution-site, and
  block-entry publication details. These are not plain CFG facts; they are
  out-of-SSA/prealloc and target-copy policy surfaces and must remain blocked
  unless a future packet names one reader row and preserves status, move,
  cycle, execution-site, and publication fallback behavior.
- Prepared-printer, debug, and public aggregate compatibility:
  `prepared_printer/control_flow.cpp` prints block terminators, branch
  conditions, join transfers, edge transfers, parallel copies, execution sites,
  move/step indexes, and cycle-temp facts from `module.control_flow`. Backend
  filters in `backend.cpp` copy, erase, and trim `filtered.control_flow`
  alongside the public aggregate. `legalize.cpp`, `out_of_ssa.cpp`,
  `label_identity.cpp`, call/publication/regalloc construction, and tests
  construct or mutate the public field. These are retained compatibility
  surfaces; Step 4 does not open deletion, privatization, aggregate retirement,
  printer rewrites, or broad control-flow construction changes.

Concrete future packets:

- One-reader packet candidate:
  `prepared_lookups.cpp::find_dominating_indexed_prior_preserved_value` and
  `find_unique_indexed_prior_preserved_value_source` can be grouped as a
  call-preservation dominance lookup packet. Reader: prior preserved-value
  lookup for one current call plan. Semantic fact: route/BIR dominance over the
  same call-plan block indices plus same-block instruction ordering.
  Compatibility surface: null control-flow context, invalid value id, empty
  prior list, same-block later prior entry, unreachable or non-dominating
  blocks, duplicate prior entries, and missing preserved rows keep returning
  `nullptr` or an unavailable result. Fail-closed proof: present dominating
  prior, same-block earlier/later prior, no control-flow context, invalid
  block index, non-dominating predecessor, unreachable block, and duplicate
  prior source cases must not select a different source.
- One-reader packet candidate:
  `control_flow.hpp::find_prepared_control_flow_branch_target_labels` remains
  a helper-row candidate only for structured conditional successor identity.
  Reader: branch-target label helper for one conditional block. Semantic fact:
  BIR terminator true/false `BlockLabelId` values under prepared agreement.
  Compatibility surface: public helper still returns prepared labels without
  an agreeing BIR context, and non-conditional, absent block, invalid labels,
  missing structured ids, mismatch, and non-agreement paths preserve current
  `std::nullopt` or prepared fallback behavior. Fail-closed proof: valid
  agreement, no function block, return/unconditional terminator, invalid
  prepared labels, invalid BIR ids, mismatched BIR ids, and conflicting
  structured/raw label cases.
- One-reader packet candidate:
  `call_plans.cpp::prepared_block_label_for_index` can be isolated as a
  block-index label bridge packet. Reader: call-plan block index to
  `BlockLabelId` mapping. Semantic fact: the call-plan block index names the
  same BIR block id as the prepared control-flow block at that index.
  Compatibility surface: existing fallback to `function.blocks[index].label_id`
  when control-flow is absent or too short, and `kInvalidBlockLabel` for
  out-of-range indexes. Fail-closed proof: prepared/BIR agreement,
  control-flow absent, control-flow shorter than BIR, BIR shorter than the
  index, invalid BIR label id, and prepared/BIR mismatch cases.

Blocked exits:

- AArch64 branch lowering, fused compare, dispatch, MIR successor metadata,
  and branch-target operand spelling are target-policy owned.
- AArch64 traversal and x86/RISC-V prepared handoff/setup are layout and
  compatibility surfaces; block order and broad lowering rewrites are out of
  scope.
- Join-transfer, edge-transfer, publication, parallel-copy, cycle-temp,
  execution-site, and move-bundle readers are blocked as prealloc/out-of-SSA
  or target-copy policy unless a later packet names one row and exact status
  proof.
- Prepared-printer/debug text and x86 handoff error strings are output
  compatibility surfaces, not semantic exits.
- `legalize.cpp`, `out_of_ssa.cpp`, `label_identity.cpp`, backend filters,
  public aggregate exposure, and broad `PreparedControlFlow` construction or
  deletion remain blocked by compatibility.

## Suggested Next

Proceed to Step 5 by mapping
`PreparedBirModule::store_source_publications`. Keep the next packet
analysis-only and separate source/publication semantic records from
source-memory status, addressing/storage output, and target-policy fallback.

## Watchouts

Do not treat CFG identity helpers as permission to rewrite branch lowering,
layout, join-transfer construction, or parallel-copy publication. Dominance
and block-index candidates need explicit mismatch and missing-context proof
before they are implementation-ready. Branch-target identity work must stay
helper-row scoped and keep public prepared fallback behavior.

## Proof

Analysis-only packet; no build or test proof required. Ran
`git diff --check -- todo.md`.
