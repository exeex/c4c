Status: Active
Source Idea Path: ideas/open/240_aarch64_inline_asm_machine_nodes.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Inline-Asm Authority

# Current Packet

## Just Finished

Completed Plan Step 1 inventory for the current inline-asm carrier path and
the first Step 2 implementation packet.

Representative matrix:

| Representative | Current source authority | Current BIR carrier | Current prepared authority | Step 2 target |
| --- | --- | --- | --- | --- |
| positional register input, e.g. `r` with `%0` | HIR/LIR preserve template text, constraint token, side-effect flag, and typed `args_str` | `CallInst` may carry typed `args`; `InlineAsmMetadata` keeps raw `constraints`; no per-operand constraint record | `PreparedValueHome` can publish register, stack slot, rematerializable immediate, and pointer-base-plus-offset homes for named BIR values; no inline-asm carrier consumes them yet | parse/split supported constraint tokens into operand records, link positional operand 0 to `call.args[0]`, require a usable home before marking complete |
| immediate input, e.g. `I`/`i` with an integer constant | LIR can carry typed immediate text in `args_str`; BIR lowerer parses scalar/function-pointer operands only when `parse_typed_operand` and `lower_value` accept them | `call.args` can represent constants as BIR values, but inline-asm metadata has no immediate-kind fact | homes may expose `RematerializableImmediate`; intrinsic carriers already model `immediate_value` as a precedent | add explicit inline-asm immediate operand facts and accept only literal/rematerializable integer immediates with supported constraint spelling |
| named operand, e.g. `%[src]` / `[src] "r"(x)` | current LIR struct has no operand-name field; names are not preserved separately from rendered template/constraint/args strings | no name table or name-to-index map on `InlineAsmMetadata` | prepared value names exist, but inline-asm operand names do not | add an optional operand name per inline-asm operand only once frontend/LIR can provide it; until then unknown named references stay diagnostic-only |
| tied output/readwrite, e.g. `=r,0` from `+r` | HIR lowering renders `+r` as output `=r` plus input tie `0`; current route tests already require this BIR shape | result is `CallInst::result`; tied input is just another `call.args` value; tie remains raw constraint text | `PreparedIntrinsicCarrier` precedent has `result_home` plus `operand_homes`; no tie validation exists for inline asm | parse numeric tie tokens, link output index to input operand index, require both result and tied input homes, and fail closed on malformed/out-of-range ties |
| clobbers, e.g. `cc`, `memory`, register clobbers | current `LirInlineAsmOp` has no clobber vector; clobbers are not preserved by the carrier path | no clobber field in `InlineAsmMetadata` | prepared call/helper paths model clobbered registers elsewhere, but inline asm has no prepared clobber authority | keep clobbers diagnostic-only until source/LIR exposes structured clobber text; first Step 2 packet should reserve carrier shape but not fabricate clobbers |

Current carrier path inventory:

- `LirInlineAsmOp` owns `result`, `ret_type`, `asm_text`, raw `constraints`,
  `side_effects`, and rendered typed `args_str`.
- `lir_to_bir/calling.cpp` lowers inline asm to `CallInst{callee =
  "llvm.inline_asm"}` with `InlineAsmMetadata{asm_text, constraints,
  args_text, side_effects}`.
- If `args_str` parses cleanly, BIR stores typed operands in `CallInst::args`
  and clears `args_text`; otherwise raw args text remains dump-only.
- Non-void inline asm can produce one scalar/function-pointer `CallInst::result`;
  aggregate/multiple outputs currently fail the inline-asm placeholder family.
- `bir_printer.cpp` prints only template text, raw constraints, optional raw
  args text, and side-effect flag. It does not print structured constraints,
  output/tie facts, clobbers, operand names, or modifiers.
- Stack layout currently summarizes only inline-asm instruction count and
  whether any instance has side effects; the summary feeds regalloc hints but
  not operand/clobber selection.
- There is no prepared inline-asm carrier today. The closest precedent is
  `PreparedIntrinsicCarrier`, which already combines operation metadata,
  operands, result, `operand_homes`, `result_home`, side effects, missing facts,
  and prepared-printer output.
- AArch64 currently has a generic assembler/external-input record with
  `has_inline_asm_payload`, but it is intentionally deferred unsupported and
  not a selected structured inline-asm machine node.

Prepared home authority inventory:

- `PreparedValueHome` is the existing authority for value placement:
  `Register`, `StackSlot`, `RematerializableImmediate`,
  `PointerBasePlusOffset`, or `None`.
- Register homes carry physical spelling through `register_name`; stack homes
  carry frame slot, offset, size, and alignment; immediate homes carry integer
  literal authority; pointer-base-plus-offset homes carry a named base plus
  byte delta.
- `prepared_home_for_named_value` and `find_prepared_value_home` are the
  current lookup route for named values. Inline asm should consume this route
  instead of adding local allocation.
- The prepared intrinsic carrier builder already shows the desired missing-fact
  pattern: collect operand/result homes, validate shape, and mark the carrier
  complete only when required facts exist.

Unsupported / diagnostic-only notes:

- Unsupported constraints for the first Step 2 packet: memory operands (`m`),
  address operands, explicit physical-register constraints, register classes
  beyond AArch64 scalar `r`, vector/floating constraints, alternatives (`|`),
  commutative `%`, early-clobber `&`, indirect `*`, and aggregate/multi-result
  outputs.
- Unsupported modifiers for the first Step 2 packet: all template modifiers
  should remain diagnostic-only until Step 4; Step 2 should preserve raw
  template text but not interpret `%w0`, `%x0`, `%[name]`, or target-specific
  modifier spelling.
- Missing-home cases must be fail-closed prepared facts: missing operand home,
  missing result home for output asm, non-register home for `r` when selection
  requires a register, non-immediate value for immediate constraints, malformed
  numeric tie, tie pointing at a missing operand, and any named operand without
  structured name authority.
- Clobbers must not be inferred from template text. Without structured
  clobbers from source/LIR, `cc`, `memory`, and register clobbers remain
  unsupported metadata.

## Suggested Next

First implementable Step 2 packet: add BIR/prepared inline-asm carrier structs
and prepared-printer visibility for the minimal complete carrier shape without
AArch64 selection.

Scope:

- Add structured BIR inline-asm operand records derived from existing
  `CallInst::args`, `CallInst::result`, raw constraint tokens, and
  `side_effects`.
- Support only positional scalar/pointer register inputs (`r`), one scalar or
  pointer register output (`=r`), numeric tied input (`0`) linked to that
  output, and integer immediate inputs with an explicitly supported immediate
  constraint.
- Add a prepared inline-asm carrier mirroring the intrinsic-carrier validation
  style: template, side effects, raw constraints, operands, result, operand
  homes, result home, output/tie/immediate facts, carrier completeness, and
  missing-required-facts.
- Print complete carriers and missing facts from `prepared_printer.cpp`.
- Add nearby BIR/prepared-printer tests for positional input, output+tied input,
  immediate input, and fail-closed missing/malformed unsupported facts.
- Do not add dispatch, machine records, template substitution, clobber modeling,
  named operands, or physical-register allocation in this packet.

## Watchouts

- Keep the source idea stable unless durable intent actually changes.
- Do not claim inline-asm machine-node support from a single hard-coded
  fixture.
- Do not infer operand, clobber, output, or side-effect authority from rendered
  assembly text.
- Do not add an allocator, spill planner, atomic helper route, or assembler
  parser under this plan.
- Current named operands and clobbers appear blocked on source/LIR carrier
  shape, not on AArch64 printing. Preserve placeholders or diagnostics, but do
  not invent names/clobbers by scanning template text.
- The first implementation packet should follow the intrinsic-carrier missing
  fact pattern so unsupported constraints remain visible in prepared dumps
  instead of silently falling back to raw inline-asm text.

## Proof

Inventory-only lifecycle/progress update. No build or test command was required
or run, and no proof log was created or modified.
