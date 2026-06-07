Status: Active
Source Idea Path: ideas/open/117_aarch64_comparison_fused_compare_publication_contract.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Characterize Comparison Publication Contract Residue

# Current Packet

## Just Finished

Step 1 - Characterize Comparison Publication Contract Residue is complete.

Helper-to-contract map:

- `find_prepared_materialized_compare_join_targets`: already routes through
  shared prepared control-flow facts. It starts from authoritative branch-owned
  join transfer, resolves the shared materialized compare join context, then
  consumes published continuation labels. Remaining comparison-local work is
  only adapting the prepared labels back into AArch64 branch targets.
- `find_prepared_fused_compare_operand_producer_facts`: comparison-local wrapper
  for assembling lhs/rhs fact pairs around the shared
  `prepare::find_prepared_fused_compare_operand_producer` query. The shared
  query already classifies immediates, same-block scalar producers, producer
  instruction/index/name, casts, local/global loads, binary, select, and
  evaluated integer constants. The wrapper residue is branch-condition gating,
  current-block `before_instruction_index`, and pair packaging for
  `make_prepared_conditional_branch_record_impl`.
- `collect_prepared_current_block_entry_publications`: thin current-block
  adapter over shared `prepare::collect_prepared_block_entry_publications`.
  Shared facts already include block-entry move-bundle selection, destination
  value id/name/home, destination kind/storage/register name, and availability
  status.
- `prepared_current_block_entry_publication_register`: comparison-local
  duplicate of the existing AArch64 dispatch-publication helper shape. It uses
  shared prepared value-name/value-home lookup and shared block-entry
  publication availability, but still parses the prepared register string into
  an AArch64 GP register and coerces the expected view locally.

Existing shared prepared queries already available:

- `prepare::find_authoritative_branch_owned_join_transfer`
- `prepare::find_materialized_compare_join_context`
- `prepare::published_prepared_compare_join_continuation_targets`
- `prepare::resolve_prepared_compare_branch_target_labels`
- `prepare::find_prepared_fused_compare_operand_producer`
- `prepare::make_prepared_edge_publication_source_producer_lookups`
- `prepare::collect_prepared_block_entry_publications`
- `prepare::prepared_block_entry_publication_available`
- `prepare::resolve_prepared_value_name_id`
- `prepare::find_indexed_prepared_value_home`

Missing or too-private shared facts forcing comparison-local routing:

- No shared current-block publication query returns a typed destination value
  home plus destination register as a target-neutral record; consumers must
  iterate publications and re-match value ids.
- No shared publication-to-AArch64 register adapter is exported from
  `dispatch_publication`, so comparison duplicates
  `current_block_entry_publication_register` behavior under a private helper.
- Fused compare operand producer pair facts are not published as a reusable
  branch-condition-level contract; comparison must check the branch condition,
  compute the current-block cutoff index, call lhs/rhs separately, and package
  partial producer facts locally.
- The shared fused-operand producer contract exposes producer classification,
  but comparison still owns fallback decisions around whether those facts are
  enough for branch-record construction versus AArch64 emission routes.

Target-local keep-local notes:

- Keep AArch64 condition suffix selection, scalar register view selection,
  compare opcode/immediate encodability checks, branch line emission, register
  parsing/printing, and scratch/register role policy target-local.
- Keep materialized compare target adaptation local after shared prepared code
  publishes continuation labels; shared code should not choose AArch64 branch
  record targets or emit labels.
- Keep fail-closed gating for non-fusable compare branches and target-specific
  diagnostics local to comparison lowering.

## Suggested Next

Delegate Step 2 to add or tighten prepared fact visibility before changing
ownership. A narrow first implementation packet should make one missing
contract visible without moving behavior: either a shared/current-block
publication register fact surface or a fused-compare operand producer pair dump
surface.

## Watchouts

- `comparison.cpp` has private current-block publication helpers that overlap
  with `dispatch_publication.cpp`; prefer exporting/reusing an existing
  AArch64 helper or adding a shared prepared fact query over copying another
  local scan.
- Do not replace shared prepared authorities with raw BIR rescans. The current
  shared fused producer query already uses prepared edge-publication source
  producer lookups and should stay authoritative.
- A fused-compare proof alone is insufficient for this idea. Pair it with a
  materialized compare/current-block publication neighbor before supervisor
  acceptance.
- Keep target-local emission details out of shared prepared APIs.

## Proof

Not run. Packet was characterization-only with owned file `todo.md`; no build
or test proof was required and no `test_after.log` was produced.

Supervisor-ready narrow proof recommendations:

- Build: `cmake --build build --target backend_aarch64_instruction_dispatch_test backend_prepare_authoritative_join_ownership_test backend_prealloc_block_entry_publications_test -j`
- Fused-compare path: `ctest --test-dir build -R '^backend_aarch64_instruction_dispatch$' --output-on-failure`
- Materialized/current-block publication path: `ctest --test-dir build -R '^(backend_prepare_authoritative_join_ownership|backend_prealloc_block_entry_publications|backend_aarch64_instruction_dispatch)$' --output-on-failure`
