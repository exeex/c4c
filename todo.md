Status: Active
Source Idea Path: ideas/open/571_rv64_inline_asm_carrier_lowering.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect Inline Asm Carrier Representation

# Current Packet

## Just Finished

Step 1: Inspect Inline Asm Carrier Representation is complete.

Inventory source: `c4c-clang-tools` queries over
`src/backend/mir/riscv/codegen/object_emission.cpp`, focused reads of
`src/backend/bir/bir.hpp`, `src/backend/prealloc/special_carriers.hpp`,
`src/backend/prealloc/inline_asm.cpp`, prepared-printer snippets, and the 570
diagnostic artifacts.

Representation findings:

- BIR callee shape: inline asm carriers are ordinary `bir::CallInst` nodes with
  `callee="llvm.inline_asm"`, `return_type=Void`, `result=none`,
  `is_indirect=false`, `callee_value=none`, and `inline_asm` metadata present.
  The four 571 representatives all failed first on ownerless `CallInst` nodes.
- BIR inline asm metadata carries `asm_text`, `constraints`, `args_text`,
  `side_effects`, parsed `InlineAsmOperandMetadata`, `clobbers`,
  `unsupported_facts`, `has_named_operand_references`,
  `has_template_modifiers`, and optional `.insn r` metadata.
- Memory clobber shape: `~{memory}` is represented as an inline asm operand
  `kind=clobber`, `constraint="~{memory}"`, `name="memory"`, plus
  `clobbers=["memory"]`; the prepared complete carrier prints it as
  `operand0[kind=clobber,constraint="~{memory}",name="memory",home=no]`
  and `clobber0="memory"`.
- Side-effect marker: the BIR dump prints `[... sideeffect]`; prepared carriers
  preserve this as `side_effects=yes`.
- Prepared-BIR call plan shape: the callsite is still a
  `direct_extern_fixed_arity` call with `callee=llvm.inline_asm`. The call plan
  includes conservative call clobbers such as `gpr:t0`, `fpr:ft0`, and
  `vreg:v0`-`v15`, but no normal call result for the representative carriers.
- Prepared inline asm carrier shape: complete carriers are indexed by
  `function_name`, `block_index`, and `inst_index`. They preserve `asm_text`,
  `constraints`, `side_effects`, operand facts, clobbers, optional result home,
  and `missing_required_facts`; object emission finds them by matching the
  prepared function carrier list against the BIR block/instruction index.
- Argument/value ownership: no-argument memory-clobber representatives
  (`src/20071211-1.c`, `src/pr56982.c`, `src/pr78438.c`) have `args=0`,
  `result_home=no`, and one memory clobber operand. `src/pr51933.c` has BIR args
  `ptr @v1`, `ptr @v2`, `ptr @v3`; prepared call arguments use
  `source_encoding=symbol_address`, `source_symbol=@v1/@v2/@v3`, and GPR call
  argument destinations `a0/a1/a2`.
- Symbol-address operand shape: symbol pointers are `bir::Value` instances with
  symbolic pointer link-name identity when available. Prepared call arguments
  print them as `source_encoding=symbol_address source_symbol=@...` and may
  carry `source_symbol_id`. This is present in `pr51933` for the inline asm
  call and elsewhere for ordinary calls such as `_setjmp`.
- Current RV64 object-emission intercept point: `fragment_for_prepared_instruction`
  is the first instruction dispatcher before generic fallback. For a `CallInst`,
  it obtains the `PreparedCallPlan`, finds the matching
  `PreparedInlineAsmCarrier` with `find_prepared_inline_asm_carrier`, and calls
  `fragment_for_prepared_call`. Inside `fragment_for_prepared_call`, the
  `call.inline_asm.has_value()` branch is the carrier-specific place to emit or
  diagnose inline asm before general call lowering and before
  `unsupported_prepared_instruction_fragment_diagnostic`.
- Generic fallback path to avoid: when `fragment_for_prepared_instruction`
  returns `nullopt`, `prepared_function_to_object_function` next tries narrow
  pre-fallback diagnostics and then constructs
  `unsupported_instruction_fragment: BIR instruction requires unsupported RV64
  object lowering`.

Unsupported forms that should remain diagnostic-only for this 571 route:

- `pr51933`-style `imr,imr,imr,~{memory}` is not a complete prepared carrier
  today; prepared inline-asm facts record `unsupported_constraint0/1/2:imr`,
  `constraint_operand_count_mismatch`, and unsupported operand constraints.
  This should stay on a precise inline-asm diagnostic until the route explicitly
  supports mixed immediate/memory/register alternatives.
- Named operand references, template modifiers, missing required facts,
  unsupported constraints, mismatched operand counts, multiple outputs,
  result-producing forms without a register result home, non-register input or
  output homes, memory/address operands without selectable prepared addresses,
  vector forms outside the existing VR register-class handling, and unparsed
  or unencodable asm text should not fall through to the old generic fallback.

## Suggested Next

Execute Step 2 by adding focused backend object-emission tests for a
no-result side-effecting `llvm.inline_asm` carrier with `~{memory}` and a
negative diagnostic case for the `pr51933` `imr`/symbol-address shape.

## Watchouts

- This plan is limited to RV64 inline asm carrier calls.
- Do not implement general call ABI lowering, pointer arithmetic, select,
  floating-point binary, or branch-published phi lowering here.
- Do not add filename-specific matching for `src/20071211-1.c`,
  `src/pr51933.c`, `src/pr56982.c`, or `src/pr78438.c`.
- Do not change expectations, unsupported markers, allowlists, or runtime
  comparison behavior.
- `src/pr51933.c` is a symbol-address and unsupported-constraint diagnostic
  shape, not a generic symbol-address call-lowering target for this plan.
- Keep unsupported inline asm forms on precise inline-asm-specific diagnostics
  instead of the old generic fallback; do not treat `imr` as supported by
  choosing one named representative-specific interpretation.

## Proof

Inventory-only packet. Run:

`git diff --check -- todo.md`

Result: passed. Log path: `test_after.log`.
