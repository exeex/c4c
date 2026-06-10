# Current Packet

Status: Active
Source Idea Path: ideas/open/149_residual_prepared_lookup_include_cleanup.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Map residual prepared lookup includes and narrow owners

## Just Finished

Step 1 inspection mapped residual `prepared_lookups.hpp` include consumers in
the AArch64 and prealloc target scope.

Search target used the repository's actual AArch64 path
`src/backend/mir/aarch64` plus `src/backend/prealloc`; the initially delegated
`src/backend/aarch64` path is not present in this tree.

Keep broad include because the file still names `PreparedFunctionLookups` or
`make_prepared_function_lookups`:

- `src/backend/mir/aarch64/codegen/alu.cpp`: names
  `prepare::PreparedFunctionLookups` and
  `prepare::make_prepared_function_lookups`; it also names return-chain helper
  APIs that are still declared only by `prepared_lookups.hpp`.
- `src/backend/mir/aarch64/module/module.hpp`: stores
  `const prepare::PreparedFunctionLookups* prepared_lookups`.

Not a cleanup consumer:

- `src/backend/prealloc/prepared_lookups.cpp`: includes its own owning header
  and defines `make_prepared_function_lookups`.

Can remove broad include with current narrow owner headers:

- `src/backend/mir/aarch64/codegen/dispatch_lookup.cpp`: replace with
  `../../../prealloc/control_flow.hpp` for
  `resolve_prepared_value_name_id` and
  `../../../prealloc/value_locations.hpp` for value-home lookups and
  `PreparedRegisterBank`.
- `src/backend/mir/aarch64/codegen/dispatch_value_materialization.cpp`:
  replace with `../../../prealloc/addressing.hpp` for same-block memory access
  helpers and `../../../prealloc/select_chain_lookups.hpp` for same-block
  producer/materialization APIs; `select_chain_lookups.hpp` is already present.
- `src/backend/mir/aarch64/codegen/frame_slot_address.cpp`: replace with
  `../../../prealloc/addressing.hpp` for `find_prepared_addressing` and memory
  access declarations; `../../../prealloc/stack_layout/stack_layout.hpp` is
  already present for stack-layout declarations.
- `src/backend/mir/aarch64/codegen/fp_value_materialization.cpp`: replace with
  `../../../prealloc/addressing.hpp` for memory/global-load helpers,
  `../../../prealloc/control_flow.hpp` for
  `resolve_prepared_value_name_id`,
  `../../../prealloc/names.hpp` for `prepared_link_name`, and
  `../../../prealloc/value_locations.hpp` for value-home/register-bank
  declarations; `../../../prealloc/select_chain_lookups.hpp` is already
  present for same-block producer lookups.
- `src/backend/mir/aarch64/codegen/memory_store_retargeting.cpp`: replace with
  `../../../prealloc/addressing.hpp` for
  `PreparedAddressMaterializationLookups` and indexed frame-address lookup,
  `../../../prealloc/control_flow.hpp` for
  `resolve_prepared_value_name_id`, and
  `../../../prealloc/value_locations.hpp` for `PreparedValueHomeLookups`;
  `../../../prealloc/stack_layout/stack_layout.hpp` is already present.
- `src/backend/mir/aarch64/codegen/select_materialization.cpp`: replace with
  `../../../prealloc/calls.hpp` for call argument publication routing,
  `../../../prealloc/control_flow.hpp` for
  `resolve_prepared_value_name_id`, and
  `../../../prealloc/value_locations.hpp` for value-home APIs.
- `src/backend/mir/aarch64/codegen/comparison.cpp`: replace with
  `../../../prealloc/control_flow.hpp` for branch/control-flow helpers and
  name resolution, `../../../prealloc/publication_plans.hpp` for scalar
  publication planning, `../../../prealloc/value_locations.hpp` for value-home
  APIs, and `../../../prealloc/addressing.hpp` for prepared memory access
  helpers; `../../../prealloc/select_chain_lookups.hpp` and
  `../../../prealloc/stack_layout/stack_layout.hpp` are already present.
- `src/backend/mir/aarch64/codegen/calls.cpp`: replace with
  `../../../prealloc/control_flow.hpp` for name resolution,
  `../../../prealloc/publication_plans.hpp` for edge-publication source
  producer types used by select-chain/call routing, and keep existing
  `../../../prealloc/addressing.hpp`, `../../../prealloc/calls.hpp`,
  `../../../prealloc/select_chain_lookups.hpp`,
  `../../../prealloc/stack_layout/stack_layout.hpp`, and
  `../../../prealloc/value_locations.hpp`.
- `src/backend/prealloc/call_plans.cpp`: broad include is removable; existing
  local includes already bring the concrete owners (`call_plans.hpp`/`module.hpp`,
  `calls.hpp`, `select_chain_lookups.hpp`, `regalloc/call_return_abi.hpp`,
  `target_register_profile.hpp`, and `variadic.hpp`).
- `src/backend/prealloc/prepared_printer/select_chains.cpp`: broad include is
  removable; existing `private.hpp`, `publication_plans.hpp`, and
  `select_chain_lookups.hpp` cover the used declarations.
- `src/backend/prealloc/select_chain_lookups.cpp`: broad include is removable;
  existing `select_chain_lookups.hpp` and `module.hpp` cover the used
  declarations.
- `src/backend/prealloc/formal_publications.cpp`: replace with
  `value_locations.hpp` for `find_indexed_prepared_value_id`,
  `find_indexed_prepared_value_home`, and value-home declarations.
- `src/backend/prealloc/decoded_home_storage.hpp`: broad include is removable;
  existing `value_locations.hpp`, `regalloc.hpp`, and `storage.hpp` cover the
  declarations.
- `src/backend/prealloc/module.hpp`: broad include is removable with no
  replacement; this header already includes the concrete owner headers for
  `PreparedBirModule` fields.

Can remove by the Step 1 aggregate/factory rule, but current narrow owner is
missing:

- `src/backend/mir/aarch64/codegen/dispatch_producers.cpp`: does not name
  `PreparedFunctionLookups` or `make_prepared_function_lookups`, but it calls
  `make_prepared_edge_publication_lookups`, which is still declared only in
  `src/backend/prealloc/prepared_lookups.hpp`. The natural owner appears to be
  `../../../prealloc/publication_plans.hpp`, which already owns
  `PreparedEdgePublicationLookups`, but the declaration has not moved there.

## Suggested Next

Execute the next coherent cleanup packet only after the supervisor decides how
to handle `dispatch_producers.cpp`'s missing narrow owner for
`make_prepared_edge_publication_lookups`. If that blocker is accepted for this
runbook, start AArch64 include cleanup for the files classified removable above
and leave `alu.cpp`, `module/module.hpp`, and `dispatch_producers.cpp` alone.

## Watchouts

- Do not remove `prepared_lookups.hpp` from files that still name
  `PreparedFunctionLookups` or `make_prepared_function_lookups`.
- Do not replace the old facade with another broad umbrella header.
- Keep the cleanup behavior-preserving and include-focused.
- `make_prepared_edge_publication_lookups`, `PreparedReturnChainLookups`,
  `find_prepared_return_chain_terminal_value`, and
  `find_prepared_return_chain_next_operand_value` currently have no narrow
  owner header; they are declared only in `prepared_lookups.hpp`.

## Proof

Inspection-only per delegated proof. No build or tests run, and no
`test_after.log` was created because only `todo.md` changed.

Inspection commands/results:

- `rg -n '#include .*prepared_lookups\.hpp' src/backend/mir/aarch64 src/backend/prealloc`
  found residual includes in the AArch64 codegen/module files listed above and
  prealloc files listed above, plus the owning `prepared_lookups.cpp`.
- `for f in $(rg -l '#include .*prepared_lookups\.hpp' src/backend/mir/aarch64 src/backend/prealloc | sort); do rg -n 'PreparedFunctionLookups|make_prepared_function_lookups' "$f" || true; done`
  found aggregate/factory use only in `alu.cpp`, `module/module.hpp`, and the
  owning `prepared_lookups.cpp`.
- `rg -n 'make_prepared_edge_publication_lookups|PreparedReturnChainLookups|find_prepared_return_chain' src/backend/prealloc/*.hpp src/backend/prealloc/**/*.hpp`
  confirmed the edge-publication lookup builder and return-chain helpers remain
  declared only in `prepared_lookups.hpp`.
