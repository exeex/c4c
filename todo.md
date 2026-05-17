Status: Active
Source Idea Path: ideas/open/275_prepared_printer_decomposition.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Establish the Private Printer Boundary

# Current Packet

## Just Finished

Completed `plan.md` Step 1 inventory/build-boundary pass for
`prepared_printer.cpp`.

Ownership map:
- Core/public entry: `print()` owns module header text, completed phases,
  invariants, notes, the embedded prepared BIR dump, and the family print order.
- Family-neutral formatting: `render_value`, `escape_quoted_text`,
  `optional_size_text`, `optional_signed_size_text`, `type_kind_name`, and
  enum/kind name adapters that are reused by multiple family printers.
- Names/rendering lookup helpers: `maybe_function_name`, `maybe_block_label`,
  `maybe_value_name`, `maybe_slot_name`, `maybe_link_name`, `maybe_text_name`,
  `find_prepared_function`, and raw/debug-only symbol rendering in
  `source_symbol_name`.
- Function summaries: `append_function_summaries`; it is a cross-family summary
  over frame, dynamic stack, call, variadic, storage, preservation, and clobber
  data, so it should move with core or a dedicated summaries unit before the
  family files depend on it.
- Frame/stack: `append_stack_layout`, `append_frame_plan`,
  `append_dynamic_stack_plan`, `find_frame_slot`, and saved-register slot
  formatting.
- Calls: `append_call_plans`, `append_call_arg_source_summary`,
  `append_indirect_callee_summary`, `append_indirect_callee_detail`,
  `append_memory_return_summary`, `append_memory_return_detail`,
  `append_preserved_value_summary`, and call clobber/register span summaries.
- Variadic: `append_variadic_entry_plans`; it currently shares optional-size,
  type-kind, value-home, and name rendering helpers only.
- Storage: `append_storage_plans` plus `storage_encoding_kind_name`,
  register/spill placement helpers, and register occupancy formatting.
- Special carriers: `append_i128_carriers` and `append_f128_carriers`.
- Atomics: `append_atomic_operations` plus `address_space_name` and
  `type_kind_name`.
- Intrinsics: `append_intrinsic_carriers`; shares `type_kind_name` and
  `address_space_name`.
- Inline asm: `append_inline_asm_carriers`; shares quoted-text escaping and
  value-home/name rendering.
- Runtime helpers: `append_f128_runtime_helpers` and
  `append_i128_runtime_helpers`; both share register placement, clobber,
  preservation, type, value-home, and carrier formatting.
- Addressing: `append_addressing`; shares address-space, name/link/text, and
  value-home rendering.
- Control flow: `append_prepared_control_flow`,
  `prepared_join_transfer_ownership_name`,
  `prepared_parallel_copy_resolution_name`, and
  `append_parallel_copy_step_detail`.
- Regalloc: `append_regalloc`, `find_regalloc_function`,
  `find_regalloc_value`, and spill/reload register or spill-slot formatting.
- Value locations: `append_value_locations`, move destination/storage/op kind
  names, and register placement helpers.

Build-wiring finding: no code/build skeleton is needed before the first
extraction. `src/backend/CMakeLists.txt` already uses `GLOB_RECURSE` for
`src/backend/prealloc/*.cpp`, so future
`src/backend/prealloc/prepared_printer/*.cpp` files should be included without a
CMake edit. A private helper header should wait until the first extraction so it
contains only declarations that are actually shared.

## Suggested Next

Extract the first behavior-preserving unit as a small core/functions slice:
introduce a private `prepared_printer/` boundary with only the declarations
needed to move `print()` coordination plus `append_function_summaries`, or move
`append_function_summaries` first if the supervisor wants to keep the public
entry in the legacy file for one more packet.

## Watchouts

- Preserve prepared dump output exactly.
- Keep `prepared_printer.hpp` as the small public print API.
- Do not change prepared schema, producer behavior, tests, or snapshots.
- `append_function_summaries` reaches into frame, dynamic stack, call, variadic,
  storage, preservation, and clobber facts; moving it after family extraction
  would force a wider helper surface.
- The current helper cluster is already broad. Start with a minimal private
  helper header and only promote helpers after a moved file needs them.
- Do not split runtime helper printers casually: the i128/f128 helper sections
  have many local lambdas and shared call-preservation/clobber formatting.

## Proof

Inventory-only Step 1; no code/build skeleton was needed, so no build or test
subset was run. Proof command: `git diff --check` passed.
