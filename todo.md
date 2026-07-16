Status: Active
Source Idea Path: ideas/open/847_lir_universal_model_string_escape_hatch_deletion.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit deletion readiness and remaining target inventory

# Current Packet

## Just Finished

Completed the audit-only inventory for `plan.md` Step 1. The first safe
deletion packet is narrow: remove the mutable non-const
`LirTypeRef::str()` accessor from `src/codegen/lir/types.hpp`. Inspection found
no selected semantic caller mutating a `LirTypeRef` through `.str()`; the only
`.str() =` assignments in `src/codegen/lir/` are `LirOperand` mutations in
call-argument normalization/lowering.

## Suggested Next

Execute `plan.md` Step 2 by deleting only
`[[nodiscard]] std::string& LirTypeRef::str()` from
`src/codegen/lir/types.hpp`, then run a fresh build plus the focused LIR proof
selected by the supervisor. Do not delete const `str()`, constructors,
`runtime_text`, implicit conversions, or equality operators in the same packet.

## Watchouts

- `LirTypeRef::runtime_text` still has active callers in HIR lowering,
  call/vararg lowering, lvalue handling, parsed typed-call argument/return
  helpers, extern declaration return storage, inline assembly, and rendered
  aggregate/field/signature boundaries; do not delete it as the first packet.
- Deprecated factories
  `parsed_typed_call_argument_text`, `parsed_typed_call_return_text`,
  `stored_extern_declaration_return_text`, `hir_inline_asm_type_text`, and
  `hir_rendered_aggregate_field_signature_type_text` still have direct callers
  and need separate owner/gate classification before deletion.
- `LirTypeRef` implicit string conversions and textual equality/classification
  remain broad compatibility surfaces with many selected and unselected
  callsites; do not combine them with the mutable accessor deletion.
- Switch selector surfaces are explicitly out of scope for 847 execution:
  `LirSwitch.selector_type_ref.str()` in printer/verifier and related selector
  authority are owned by ideas 821/822, with closed 846 also calling them out as
  outside its handoff.
- Residual non-type string authority remains owned by ideas 812/813, not this
  first deletion packet.

## Proof

Audit/inspection only; no build required because only `todo.md` changed. Proof
commands run:
`rg -n "LirTypeRef|runtime_text|\\.str\\(|str\\(\\)|operator std::string|operator string|operator==|classif|adapter|factory|from_.*string|to_.*string" src/codegen/lir`;
`rg -n "parsed_typed_call_argument_text|parsed_typed_call_return_text|stored_extern_declaration_return_text|hir_inline_asm_type_text|hir_rendered_aggregate_field_signature_type_text|runtime_text\\(" src/codegen/lir`;
`rg -n "[A-Za-z0-9_>\\)\\]]+\\.str\\(\\)\\s*=" src/codegen/lir`;
`rg -n "selector_type|LirSwitch|switch selector|switch" ideas/open ideas/closed`.
No `test_after.log` was written for this audit-only packet.
