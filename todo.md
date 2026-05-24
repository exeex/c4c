Status: Active
Source Idea Path: ideas/open/target-neutral-publication-plan-record.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Map Publication Planning Surfaces

# Current Packet

## Just Finished

Completed Step 1: `Map Publication Planning Surfaces`.

Mapped the current publication planning surfaces with AST-backed symbol and
callee checks in `dispatch_publication.cpp` and
`dispatch_value_materialization.cpp`, then spot-read the relevant prealloc
helpers.

Candidate target-neutral planning facts:
- `prepare::PreparedValueHome` already carries the logical value id/name,
  home kind, register spelling, stack slot/offset/size/alignment,
  rematerializable immediates, and pointer-base-plus-offset identity.
- `prepare::collect_prepared_block_entry_publications` already summarizes
  block-entry parallel-copy publications with destination value id/name,
  destination storage kind, destination register spelling, and availability
  status.
- `prepared_value_home_for_value`, `value_has_current_block_entry_publication`,
  and the early decision points in `emit_value_publication_to_register` are the
  current AArch64-side publication-plan questions: which logical value is being
  published, whether a Prepared home exists, whether current block entry
  publication should be honored, and whether the value should be loaded from a
  home rather than recomputed.
- `storage_encoding_from_home` and `build_storage_plan_value` show the neutral
  storage encoding names already available for record shape:
  register/frame-slot/immediate/computed-address/none.

Keep-local AArch64 emission facts:
- Register parsing/spelling and view selection via
  `abi::parse_aarch64_register_name`, `abi::gp_register`,
  `abi::register_name`, `scalar_view_for_type`, `gp_register_name`, and
  `scalar_fp_register_view`.
- Instruction construction for `mov`, `ldr`, `str`, `adrp`, `add`, `cmp`,
  `cset`, FP compare, extension/truncation, and arithmetic materialization.
- Frame-slot address spelling, relocation/GOT addressing, local-slot address
  emission, va-list field address spelling, and pointer-value load lowering.
- Scratch register choice and emitted scalar register recording in
  `BlockScalarLoweringState`.

Mixed facts requiring target hooks:
- Register homes and block-entry publication records are neutral enough to
  identify the planned destination, but consuming `register_name` requires a
  target hook that validates the bank/view and returns a target-local register
  operand.
- Stack-slot homes are neutral as offsets/slot ids, but the target must choose
  frame-pointer vs stack-pointer base, memory operand format, and load/store
  mnemonic.
- Rematerializable immediates are neutral as values, but the target owns
  immediate encodability and the materialization instruction sequence,
  especially FP and width-sensitive integer cases.
- Pointer-base-plus-offset homes are neutral as base value plus byte delta, but
  target lowering owns address-register materialization and scratch usage.
- Clobber checks such as
  `block_entry_move_clobbers_current_join_publication` and
  `value_publication_may_read_register_index` mix neutral dependency/home facts
  with AArch64 physical-register aliasing, so the first record should expose
  the dependency/home inputs and leave alias testing target-local.

## Suggested Next

Execute Step 2 by defining the smallest neutral publication plan record in
prealloc/shared MIR code:

- Input: `BlockLoweringContext`-equivalent neutral prepared state, source
  `bir::Value`, instruction index, and the discovered `PreparedValueHome`.
- Output: an inspectable record with source value, optional producer/current
  block entry publication fact, optional Prepared home, neutral destination
  kind, storage encoding, and target hook kind.
- Exclude target register operands, register views, scratch register indexes,
  memory operand strings, mnemonics, and machine instruction lines.

The first implementation slice should build only the record and a query/helper
that can represent register-home, stack-slot, rematerializable-immediate, and
pointer-base-plus-offset publication intent. Do not rewire AArch64 emission
until that record compiles and has focused coverage.

## Watchouts

`emit_value_publication_to_register` is larger than the first extraction
boundary. Moving its recursive producer lowering, binary/select/cast
materialization, global-load sequences, or narrow-store recovery would combine
planning with target emission and should wait. The initial slice should not
touch implementation files until the neutral record boundary is reviewed by the
supervisor.

## Proof

Audit-only packet. No build or tests run, and no `test_after.log` was created.
