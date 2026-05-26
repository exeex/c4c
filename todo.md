Status: Active
Source Idea Path: ideas/open/36_aarch64_dispatch_publication_foldback_cleanup.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Dispatch Helper Ownership

# Current Packet

## Just Finished

Step 1 - Inventory Dispatch Helper Ownership completed the helper ownership
inventory without implementation edits:

- `dispatch_diagnostics.cpp` is the only remaining helper translation unit in
  `src/backend/CMakeLists.txt`.
- `dispatch_diagnostics.hpp` is included by `dispatch.cpp` and `calls.cpp`.
- `dispatch_diagnostics.cpp` owns one private helper,
  `classify_instruction`, used only by
  `append_unsupported_instruction_diagnostic`.
- Dispatch-only declarations/call sites:
  `append_block_diagnostic`, `unsupported_terminator_message`, and
  `append_unsupported_instruction_diagnostic` are only called from
  `dispatch.cpp`.
- `append_call_diagnostic` is called from `calls.cpp`; `variadic.cpp` and
  `inline_asm.cpp` each have their own local same-named helper, not callers of
  `dispatch_diagnostics.hpp`.
- `dispatch_publication_common.hpp` is header-only and is included by
  `alu.cpp`, `calls.cpp`, `cast_ops.cpp`, `dispatch.cpp`,
  `dispatch_edge_copies.cpp`, `dispatch_publication.cpp`,
  `dispatch_value_materialization.cpp`, `fp_value_materialization.cpp`,
  `globals.cpp`, `memory_store_sources.cpp`,
  `prepared_value_home_materialization.cpp`, and `variadic.cpp`.
- Publication-common declarations with external consumers outside
  `dispatch_publication.cpp` include `registers_alias`,
  `integer_bit_width`, `power_of_two_shift`, `find_frame_slot`,
  `find_stack_object`, `relocation_operand`, `register_indirect_address`,
  `fixed_slots_use_frame_pointer`, `frame_slot_address`,
  `scalar_view_for_type`, `gp_register_name`, `scalar_load_mnemonic`,
  `dispatch_publication_scalar_type_size_bytes`,
  `scalar_load_mnemonic_for_width`, `scalar_store_mnemonic`,
  `scalar_integer_width_bits`, `scalar_gp_register_view`, and
  `scalar_fp_register_view`.
- `prepared_frame_slot_load_address` appears publication-owner private: its
  observed non-declaration use is in `dispatch_publication.cpp`.

## Suggested Next

First mechanical fold-back packet: move dispatch-only diagnostics
(`append_block_diagnostic`, `unsupported_terminator_message`,
`append_unsupported_instruction_diagnostic`, and private
`classify_instruction`) into `dispatch.cpp`, move the calls-only
`append_call_diagnostic` declaration/definition to the calls owner if needed by
`calls.cpp`, remove `dispatch_diagnostics.cpp` from build metadata, and delete
`dispatch_diagnostics.hpp` after `dispatch.cpp`/`calls.cpp` no longer include
it. Leave `dispatch_publication_common.hpp` for a later packet because its
surface is broadly consumed and should be folded through the publication owner
API separately.

## Watchouts

This idea is mechanical fold-back only. Do not change prepared
edge-publication authority, value-home lookup semantics, diagnostic meaning, or
unsupported-path contracts. `dispatch_publication_common.hpp` is not currently
publication-private as a header; most of its declarations still need an owner
API visible to several AArch64 files.

## Proof

Inventory-only lifecycle update. No build required by the packet.

`git diff --check` passed after editing `todo.md`.
