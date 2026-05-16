Status: Active
Source Idea Path: ideas/open/257_aarch64_returns_markdown_shard_implementation_redistribution.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Current Returns Ownership

# Current Packet

## Just Finished

Step 1 audit completed for `src/backend/mir/aarch64/codegen/returns.md`
against current compiled owners.

Durable ownership map:

- `returns.cpp` / `returns.hpp` already own the selected return terminator
  entry point through `lower_prepared_return_terminator`, including immediate
  return operands, named/rematerialized value resolution, emitted scalar result
  lookup from `BlockScalarLoweringState`, `ReturnInstructionRecord` creation,
  origin attachment, and the missing-value-authority diagnostic.
- `dispatch.cpp` still owns only broad routing for return terminators: after
  operation lowering it detects `TerminatorKind::Return` and calls the returns
  owner. No legacy return-register behavior is stranded there.
- `instruction.hpp` still owns the `ReturnInstructionRecord` shape. It is a
  family-neutral record definition surface, not a code-moving candidate for the
  first implementation packet.
- `instruction.cpp` still owns return-family selected-node construction and
  metadata in broad code: `machine_instruction_primary_printer_mnemonic_kind`
  treats `ReturnInstructionRecord` as `ret`, auxiliary mnemonic classification
  treats valued returns as `mov`, and `make_return_instruction` builds the
  selected return node, operands, uses, and `Return`/`ControlFlowTransfer`
  side effects.
- `machine_printer.cpp` still owns return-specific final spelling in
  `print_return`: register-valued returns print only `ret`; small signed
  integer immediates materialize into `w0` or `x0` before `ret`; unsupported
  return operands/types fail closed.
- Ordinary scalar return selected-node behavior is covered by
  `backend_aarch64_return_lowering`: direct return dispatch, immediate return
  payloads, named rematerialized return values, and scalar results chained into
  return ABI registers.
- F128 helper and direct full-width behavior are covered outside the returns
  owner by `backend_aarch64_instruction_dispatch`, including representative
  full-width F128 helper routes, `__extenddftf2` F64-to-F128 carrier authority,
  and terminal selected `ReturnInstructionRecord` presence.
- The audit did not find compiled ownership for legacy second floating-point
  return component get/set helpers (`s1`, `d1`, `q1`) as standalone return
  helpers. Current contracts require those facts to arrive as structured
  prepared or target-MIR carriers, so any missing second-component carrier
  should be split rather than inferred locally.

Stale or obsolete markdown notes:

- Legacy hook names such as `emit_return_impl`, `emit_return_f32_to_reg_impl`,
  `emit_return_f64_to_reg_impl`, `emit_return_int_to_reg_impl`, and
  `current_return_type_impl` are historical only; current authority is the BIR
  return terminator plus `BeforeReturn` moves and ABI-binding records.
- Local register shortcuts that assign `x0`, `x1`, `w0`, `s0`, `d0`, `q0`,
  `s1`, `d1`, or `q1` from markdown examples are stale unless backed by
  prepared carriers, `module::MoveRecord`, `module::AbiBindingRecord`,
  allocation-result facts, or accepted F128 helper carriers.
- The old behavior where missing second-component stack slots silently skipped
  emission is not current compiled behavior and should not be recreated as a
  return-shard move.

## Suggested Next

First narrow code-moving packet: move the return selected-node construction
body from `instruction.cpp` into `returns.cpp`/`returns.hpp`.

Scope:

- Add a returns-owned helper that accepts `ReturnInstructionRecord` and returns
  the selected `InstructionRecord` with the same operands, uses, selection
  status, and `Return`/`ControlFlowTransfer` side effects currently produced by
  `make_return_instruction`.
- Leave `instruction.hpp` record definitions in place.
- Leave `dispatch.cpp` routing unchanged except for any include/callsite
  adjustment needed by the helper move.
- Do not move printer spelling yet; keep `machine_printer.cpp::print_return`
  for Step 3.

## Watchouts

- Broad-owner return-specific candidates are `instruction.cpp` selected-node
  construction/effects first, then `machine_printer.cpp::print_return` spelling
  in a later packet.
- Do not move `ReturnInstructionRecord` out of `instruction.hpp` in the first
  packet; that would widen the interface churn.
- Do not implement legacy F32/F64/F128 or second-component register moves from
  markdown text. They require structured carrier authority under
  `AAPCS64_CALL_RETURN_FRAME_CONTRACT.md`.
- Treat any attempt to preserve second-component `s1`/`d1`/`q1` behavior by
  hard-coded register spelling as a carrier gap, not as progress.

## Proof

Audit-only packet. Per supervisor instruction, no build or tests were run and
no `test_after.log` was produced.

Supervisor-selected proof command for the first code-moving packet:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_aarch64_(return_lowering|signature_metadata|machine_printer|instruction_dispatch)$'
```
