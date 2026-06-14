Status: Active
Source Idea Path: ideas/open/253_phase_f3_prepared_module_structural_exit_blocker_map.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Map `names` Structural Exit Preconditions

# Current Packet

## Just Finished

Step 3 - Map `names` Structural Exit Preconditions completed as an
analysis-only map for `PreparedBirModule::names`.

`names` structural exit map:

- Semantic name-to-id lookup readers:
  `PreparedNameTables` owns the prepared text table plus function, block,
  value, slot, and link-name tables. Semantic readers use `find` or the
  `resolve_prepared_*_id` wrappers to convert BIR spelling back to prepared
  ids and fail closed on missing entries. Concrete readers include
  `control_flow.hpp::resolve_prepared_function_name_id`,
  `resolve_prepared_block_label_id`, `resolve_prepared_value_name_id`,
  `prepared_lookups.cpp::existing_prepared_value_name_id`,
  `prepared_result_matches_value_name`,
  `find_prepared_same_block_scalar_producer`,
  `evaluate_prepared_same_block_integer_constant`,
  `value_locations.hpp::find_prepared_value_home(..., string_view)`,
  `find_prepared_value_home_for_bir_value`,
  `dynamic_stack.cpp::maybe_named_value_id`,
  `variadic_entry_plans.cpp::prepared_home_for_named_value`, and
  `select_chain_lookups.cpp` value/block lookup helpers. These are the only
  positive structural-exit candidates: each needs a replacement semantic fact
  that supplies the prepared id directly or proves the text-to-id miss remains
  authoritative.
- BIR/prepared label bridge and duplicate/conflict compatibility:
  `prepared_lookups.cpp::prepared_bir_block`,
  `prepared_bir_block_label_id`, `select_chain_lookups.cpp` block lookup
  helpers, AArch64 dispatch block lookup, and x86
  `bir_block_label_id`/`find_bir_block_by_prepared_label` compare prepared
  block labels against both BIR structured ids and raw labels. This is not a
  simple formatting surface: the current compatibility accepts structured-id
  spelling first, then raw-label fallback, and returns null/invalid or throws
  target handoff errors when names are missing or drifted. Any exit must keep
  duplicate-label/conflicting-table behavior fail-closed instead of silently
  selecting a different block.
- Debug text and prepared-printer spelling:
  `prepared_printer/*` readers call `prepared_function_name`,
  `prepared_block_label`, `prepared_value_name`, `prepared_slot_name`, and
  `prepared_link_name` to render prepared sections for control-flow, value
  locations, frame, calls, storage, addressing, select chains, atomics,
  intrinsics, inline asm, runtime helpers, and special carriers. x86
  `debug/debug.cpp` and x86 module debug comments also render prepared value
  and function names. These are output compatibility surfaces, not semantic
  exits; missing-id spelling, blank fields, row order, and exact text remain
  guarded by expected-output behavior.
- Route-debug names and diagnostic/fallback rendering:
  AArch64 diagnostics and route helpers carry prepared function/block/value ids
  but often render names later through `prepared.names`; examples include
  dispatch diagnostics, publication/debug queries with optional
  `PreparedNameTables*`, comparison/materialization debug helpers, and x86
  fast-path comments/errors. These names explain or guard target route choices
  and must stay retained until a packet names the diagnostic row, the semantic
  id source, and the unchanged text/fallback proof.
- Target formatting and policy consumers:
  AArch64 and x86 use prepared names for assembly labels, symbol labels,
  global/string address materialization, loop-countdown labels, pointer/global
  folding, handoff validation, and target-specific fallback. Concrete examples
  include AArch64 `traversal.cpp::find_bir_function`,
  `dispatch.cpp::find_bir_block`, `dispatch_producers.cpp::find_bir_block`,
  `globals.cpp` symbol/target-label rendering, `calls.cpp` value-name
  fallbacks, `asm_emitter.cpp` function/global labels, x86
  `validate_prepared_control_flow_handoff`,
  `append_prepared_loop_join_countdown_function`, and data/function emission.
  These are target-policy/output consumers. They may use semantic ids, but the
  structural exit is blocked until target formatting and fail-closed handoff
  behavior are proved unchanged.
- Name construction and public aggregate compatibility:
  prealloc phases intern prepared names while deriving control-flow,
  stack-layout, addressing, liveness, out-of-ssa, regalloc, call, publication,
  variadic, storage, carrier, intrinsic, inline-asm, and helper facts.
  `PreparedNameTables` copy/move reattachment also preserves public aggregate
  behavior for tests and handoff helpers. This construction/compatibility
  surface is retained; broad deletion, privatization, or table replacement is
  not opened by Step 3.

Concrete future packets:

- One-reader packet candidate:
  `prepared_lookups.cpp::existing_prepared_value_name_id`,
  `prepared_result_matches_value_name`, `prepared_same_block_source_producer`,
  and `evaluate_prepared_same_block_integer_constant` can be grouped only as a
  same-block value-name lookup packet. Reader: prepared source-producer and
  integer-constant helpers. Semantic fact: named BIR values map to existing
  prepared `ValueNameId` before producer/value-home lookup. Compatibility
  surface: unnamed values, empty names, missing prepared names, stale producer
  indexes, type mismatches, and duplicate text conflicts keep returning
  `nullopt`/false. Fail-closed proof: valid named value, unnamed value,
  missing prepared id, stale producer record, wrong type, and duplicate-same
  spelling cases must not invent authority.
- One-reader packet candidate:
  `value_locations.hpp::find_prepared_value_home(..., string_view)` and
  `find_prepared_value_home_for_bir_value` can be grouped as a value-home
  lookup packet. Reader: text/BIR-value entrypoints into value homes. Semantic
  fact: caller supplies or resolves one prepared `ValueNameId` before lookup.
  Compatibility surface: null function locations, non-named BIR values, empty
  names, missing prepared names, and missing home rows return `nullptr`.
  Fail-closed proof: valid home, missing name id, missing home, immediate
  value, empty named value, and stale indexed lookup cases must retain current
  null behavior.
- One-reader packet candidate:
  `control_flow.hpp::resolve_prepared_function_name_id`,
  `resolve_prepared_block_label_id`, and `resolve_prepared_value_name_id` can
  be isolated only as a semantic resolver API packet. Reader: name-to-id
  wrappers that currently return `std::nullopt` for missing table entries.
  Semantic fact: the caller has an authoritative prepared id or an explicit
  absent-id fact. Compatibility surface: no interning on lookup, no fallback to
  BIR ids, and current missing-name behavior. Fail-closed proof: each resolver
  covers present name, absent name, empty spelling, and duplicate/conflicting
  prepared/BIR table scenarios without creating a new id.

Blocked exits:

- Prepared-printer and x86 debug spelling exits are blocked because they are
  expected-text compatibility surfaces, not semantic authority changes.
- Route-debug and diagnostic names are blocked unless a packet names one
  diagnostic row, its semantic id source, the retained fallback text, and exact
  output proof.
- AArch64 and x86 target readers are blocked because prepared names participate
  in assembly formatting, symbol policy, control-flow handoff validation,
  target fallback, and target-specific fast paths.
- BIR/prepared block-label bridge removal is blocked until duplicate,
  conflicting, missing, structured-id, and raw-label fallback cases are proved
  fail-closed.
- Name-table construction, copy/move reattachment, public aggregate exposure,
  and broad `PreparedBirModule::names` deletion/privatization are blocked by
  public compatibility.

## Suggested Next

Proceed to Step 4 by mapping the next `PreparedBirModule` field named by the
plan. Keep the run in analysis-only mode; the same-block value-name lookup
candidate above is future packet evidence, not approval to start code work
before the structural blocker map is complete.

## Watchouts

Do not treat spelling-only printer/debug readers as safe semantic exits.
Preserve the distinction between prepared-name lookup misses that return
`nullopt`/`nullptr`/false and target handoff misses that intentionally throw.
The concrete packet candidates above are not broad permission to remove
`PreparedNameTables`; each requires duplicate/conflict and fail-closed proof.

## Proof

Analysis-only packet; no build or test proof required. Ran
`git diff --check -- todo.md`.
