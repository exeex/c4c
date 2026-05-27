Status: Active
Source Idea Path: ideas/open/54_aarch64_global_value_materialization_consumer_authority_repair.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Existing Consumer Recovery

# Current Packet

## Just Finished

Completed Step 1 audit for the non-dispatch global-load materialization
consumers without implementation edits.

Exact recovery sites found:

- `src/backend/mir/aarch64/codegen/globals.cpp:762`
  `make_load_global_got_materialization_instruction` recovers authority locally
  at lines 767-771 by calling `find_load_global_target(context, load_global)`
  and testing `target_global->address_materialization_policy ==
  GotRequired`, then recovers the emitted symbol at line 804 with
  `load_global_symbol_label(context, load_global, target_global)`. This should
  consume the prepared `PreparedMemoryAccess` for the same block/instruction
  and use `PreparedMemoryAccess::address` plus
  `prepare::prepared_global_symbol_address_policy(address,
  context.function.target_profile)` as the policy authority. The existing
  private `prepared_global_memory_access(context, instruction_index)` helper at
  `globals.cpp:207` already performs the exact block/instruction lookup, but
  it only returns a raw access and does not validate that the access matches the
  `LoadGlobal` result.
- `src/backend/mir/aarch64/codegen/fp_value_materialization.cpp:234`
  `emit_fp_value_to_register` handles same-block `LoadGlobal` FP producers by
  recovering the producer through `find_same_block_named_producer`, then
  locally recovers symbol identity and policy at lines 241-248 with
  `find_load_global_target`, `load_global_symbol_label`, and
  `target_global->address_materialization_policy == GotRequired`; lines
  249-257 emit GOT and lines 259-265 emit direct. This should consume the same
  prepared global-load access used by dispatch:
  `prepare::find_prepared_same_block_global_load_access(...)` when starting
  from a prepared same-block producer, or an equivalent shared query that
  returns `{load_global, access}` for the raw same-block producer.
- `src/backend/mir/aarch64/codegen/globals.cpp:719`
  `emit_prepared_global_symbol_load_to_register` already uses prepared
  authority (`prepared_global_memory_access` and
  `prepared_global_symbol_address_policy`) but is direct-only at lines 736-740.
  It is not the bad recovery site, but its body is the closest reusable
  emission shape if the next packet extracts a shared prepared global-load
  emitter.

Prepared authority to preserve:

- `src/backend/prealloc/addressing.hpp:42` stores `PreparedAddress` with
  `symbol_name`, `byte_offset`, `can_use_base_plus_offset`, and
  `global_address_materialization_policy`; `prepared_global_symbol_address_policy`
  at lines 55-72 is the authority for explicit prepared policy or static-model
  direct fallback.
- `src/backend/prealloc/stack_layout/coordinator.cpp:627` prepares
  `LoadGlobal` memory accesses by resolving semantic link-name identity and
  global-address policy into `PreparedMemoryAccess::address`.
- `src/backend/prealloc/prepared_lookups.cpp:2207`
  `find_prepared_same_block_global_load_access` validates the same-block
  `LoadGlobal` producer, fetches `PreparedMemoryAccess` by prepared block label
  and instruction index, checks result identity, requires
  `PreparedAddressBaseKind::GlobalSymbol`, `symbol_name`, and
  `can_use_base_plus_offset`, and returns both the `LoadGlobal` and the access.

## Suggested Next

Delegate Step 2 implementation for `globals.cpp`: route
`make_load_global_got_materialization_instruction` through prepared
global-address authority by replacing the `find_load_global_target` /
`load_global_symbol_label` / `target_global->address_materialization_policy`
gate with a prepared access lookup and
`prepare::prepared_global_symbol_address_policy`. Add the smallest shared query
only if `prepared_global_memory_access(context, instruction_index)` plus local
validation cannot safely prove the `LoadGlobal` result match.

Suggested proof subset for that packet: fresh build or targeted compile proof
for the AArch64 backend target, then the narrow AArch64 global-load
materialization tests that exercise GOT-required and direct global loads.

## Watchouts

`find_prepared_same_block_global_load_access` is currently dispatch-shaped
because it requires `PreparedSameBlockScalarProducer`; `globals.cpp` has the
current `LoadGlobal` instruction and instruction index, not that producer
wrapper. Prefer a narrow shared query keyed by prepared addressing function,
block label, instruction index, and expected `LoadGlobal` when needed, rather
than recreating symbol or policy recovery in either consumer.

When Step 3 reaches `fp_value_materialization.cpp`, the local raw
`find_same_block_named_producer` path should either be paired with a prepared
same-block producer query or replaced by one before calling
`find_prepared_same_block_global_load_access`; otherwise the FP route would
still trust raw producer discovery and only partially consume prepared
authority.

## Proof

Audit-only packet; no build/test required by the supervisor, and no
`test_after.log` was created or modified. Used `c4c-clang-tools` symbol
queries first, then narrow source inspection.
