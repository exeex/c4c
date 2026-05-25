Status: Active
Source Idea Path: ideas/open/06_prepared_call_preservation_source_authority.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Map Current Prior-Preservation Authority

# Current Packet

## Just Finished

Completed Step 1 of `plan.md`: mapped the current prior-preservation source
authority and the replacement facts already present on prepared call records.

Exact AArch64 helpers to retire or redirect in later packets:

- `find_prior_preserved_value_for_call_argument` in
  `src/backend/mir/aarch64/codegen/calls_moves.cpp` wraps
  `prepare::find_latest_indexed_prior_preserved_value` and is called by
  `lower_before_call_move`. This is the main target-local semantic selector for
  call-argument prior preservation.
- The fallback branch inside `lower_before_call_move` still calls
  `find_prior_preserved_value_for_call_argument` when
  `PreparedCallArgumentPlan::source_selection` is absent, incomplete, or a
  local-frame materialization that can fall back to a callee-saved preserved
  record. Later packets should redirect this path to complete prepared
  `PriorPreservation` selections or reject missing authority before emission.
- `find_prior_stack_preserved_value_before_instruction` in
  `src/backend/mir/aarch64/codegen/calls_moves.cpp` wraps
  `prepare::find_latest_indexed_prior_stack_preserved_value_before_instruction`
  and is called by `lower_value_moves` for before-instruction value moves. It is
  not the direct call-argument selector, but it is still AArch64-local
  prior-preserved stack source selection and should be either redirected to a
  prepared move/source fact or explicitly kept out of this call-argument
  authority move.
- `make_prior_stack_preserved_value_source` in `calls_moves.cpp` is the operand
  builder paired with that stack lookup. It can remain only as target operand
  construction after source identity is supplied by prepared data.
- `make_prior_preserved_call_argument_source` overloads in
  `src/backend/mir/aarch64/codegen/calls_argument_sources.cpp` should be split
  by responsibility: the `PreparedCallArgumentSourceSelection` overload already
  consumes prepared authority; the `PreparedCallPreservedValue` overload is used
  by the AArch64 fallback and should be retired from semantic selection once
  selections are complete.

Prepared facts already available:

- `PreparedCallArgumentPlan` has per-argument position and ABI facts:
  `instruction_index`, `arg_index`, `source_value_id`, `source_base_value_id`,
  `source_encoding`, source home snapshots (`source_register_name`,
  `source_slot_id`, `source_stack_offset_bytes`, `source_register_bank`,
  `source_register_placement`, `source_pointer_byte_delta`), destination ABI
  snapshots (`destination_register_name`, `destination_register_bank`,
  `destination_register_placement`, `destination_contiguous_width`,
  `destination_occupied_register_names`, `destination_stack_offset_bytes`,
  `destination_stack_size_bytes`), and optional `source_selection`.
- `PreparedCallArgumentSourceSelection` already carries complete
  prior-preservation payload for many cases: `kind`, `source_value_id`,
  `source_value_name`, `source_home_kind`, preserved call position
  (`preserved_call_block_index`, `preserved_call_instruction_index`),
  `preservation_route`, callee-saved facts (`preserved_register_name`,
  `preserved_register_bank`, `preserved_register_contiguous_width`,
  `preserved_occupied_register_names`, `preserved_register_placement`), and
  stack facts (`preserved_stack_slot_id`, `preserved_stack_offset_bytes`,
  `preserved_stack_size_bytes`, `preserved_stack_align_bytes`).
- `PreparedCallPlan` has call position (`block_index`, `instruction_index`),
  wrapper/callee facts, `arguments`, `result`, `preserved_values`, and
  `clobbered_registers`. Its `preserved_values` are the durable source records
  indexed by prepared lookups.
- `PreparedCallPreservedValue` has the underlying preservation identity and
  storage facts: `value_id`, `value_name`, `route`,
  `callee_saved_save_index`, `contiguous_width`, `register_name`,
  `register_bank`, `occupied_register_names`, stack slot/offset/size/align,
  `register_placement`, and `spill_slot_placement`.
- `PreparedCallPlanLookups::prior_preserved_by_value` indexes
  `PreparedPriorPreservedValueEntry{block_index, instruction_index,
  preserved}` and currently powers both prepared and AArch64 prior-preserved
  queries.

Complete prepared authority today:

- A `PreparedCallArgumentPlan::source_selection` with
  `kind == PriorPreservation` and complete callee-saved fields is enough for
  AArch64 to build a register source through the selection overload without
  rescanning preserved records.
- A `source_selection` with `kind == PriorPreservation`,
  `preservation_route == StackSlot`, and populated source value plus preserved
  stack slot/offset/size/align fields is enough for AArch64 to build the memory
  source through the selection overload.
- `preserved_call_block_index` and `preserved_call_instruction_index` identify
  the prior call whose `preserved_values` supplied the selection; this is the
  prepared replacement for rediscovering the prior preserved record at emission
  time.

Facts still reconstructed by target-local or non-final code:

- `lower_before_call_move` still reconstructs a prior preserved source by
  calling `find_prior_preserved_value_for_call_argument` when the prepared
  selection is missing or not directly consumable.
- The prepared selection builder in `src/backend/prealloc/call_plans.cpp` uses
  `find_latest_prior_preserved_value` to choose a prior preserved value while
  building `source_selection`; later packets need to make that selection match
  the intended shared indexed/dominance-aware lookup contract instead of
  preserving a second ad hoc scan.
- `lower_value_moves` still reconstructs same-block prior stack preservation
  for generic before-instruction value moves through
  `find_prior_stack_preserved_value_before_instruction`.
- AArch64 still derives register views and memory operands locally from the
  prepared facts. That operand construction can stay target-local, but choosing
  which prior preserved semantic source wins must move fully to prepared
  planning or prepared lookup helpers.

## Suggested Next

Execute Step 2: add or tighten the shared prepared prior-preservation lookup
contract so a call argument receives one valid prepared source selection, or an
explicit rejection, before AArch64 emission.

## Watchouts

- Do not move the AArch64 scan into a differently named AArch64 helper.
- Do not weaken tests or mark supported cases unsupported.
- Keep routine execution details in this file; do not rewrite `plan.md` for
  ordinary packet progress.

## Proof

No build proof required; todo-only mapping packet. Used `c4c-clang-tools`
symbol and caller/callee queries plus targeted source reads; no `test_after.log`
was generated because the delegated proof explicitly required no build proof.
