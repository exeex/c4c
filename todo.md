# Current Packet

Status: Active
Source Idea Path: ideas/open/744_lir_remaining_ordinary_value_identity_publication.md
Source Plan Path: plan.md
Current Step ID: 7.23
Current Step Title: Publish scalar inline-asm input binding authority

## Just Finished

- Stopped Plan Step 7.22 without code changes and classified scalar
  multi-output inline assembly as an exact separately owned blocker.
- Current production takes the void non-scalar path when `outputs.size() != 1`:
  output lvalue pointers remain compatibility arguments, `ordinary_results`
  are raw `fresh_tmp` placeholders, and the producer returns without scalar
  Store edges. `LirInlineAsmOp` has no native aggregate or multi-result mapping.
- Truthful multi-output support therefore requires either one aggregate call
  result with ordered ExtractValue-to-Store edges or a new multi-result/codegen
  carrier. That design belongs to a separate future aggregate/multi-result
  initiative; Step 7.22 must not infer mapping from result order or text.
- Step 7 remains active: the checked matrix's single scalar input binding can
  reuse an already-authoritative ordinary value, native binding facts, and the
  generic ownership verifier without the blocked output mapping.
- Builtin parity remains separately blocked on a native parity-purpose semantic
  carrier and is not reopened here.

## Suggested Next

- Executor: complete Plan Step 7.23 for only PS's single input-only scalar
  integer `LirInlineAsmOp` route. Source the input from the exact CC-LOAD-1
  selected-global result, preserve that same ID in the semantic ordinary input
  and corresponding binding, and keep the statement result authority-free.

## Watchouts

- Own one non-explicit-register scalar integer input binding only. Require exact
  agreement among its native type, Input role, argument index zero, ordinary
  input collection position, and the selected-global load result type/ID.
- Preserve the exact load-to-binding use edge. Do not reopen or weaken the
  CC-LOAD-1 producer contract, and do not allocate a replacement input ID.
- Compatibility `result`, rendered arguments/operands, original assembly and
  constraint text, clobbers, and all other rendered mirrors remain
  presentation/opaque. They may observe native authority but never create,
  repair, select, or validate it.
- Reachable verification must reject invalid, missing, wrong-alternative,
  unknown, or cross-function input IDs and conflicting binding type, role,
  argument index, collection position/count, or source type. Misleading
  displays with unchanged native facts must pass.
- Exclude output, tied/read-write, multi-output, memory, address, immediate,
  clobber, explicit-register, vector, and multiple-input bindings; `insn_r`
  semantics; opaque-text interpretation; compatibility-result authority;
  stack/local/object and body parameters; CFG; BIR receipt; parity; and all
  idea-741 work beyond preserving CC-LOAD-1 unchanged.
- This packet is carrier-ready only on the accepted selected-global load ID,
  existing scalar input binding/type/role/index facts, and generic ownership
  verification. If the input route needs a new binding-semantic carrier or any
  name/text-derived bridge, stop and return that exact blocker rather than
  widening Step 7.23.

## Proof

- Fresh `cmake --build --preset default`.
- Focused `ctest --test-dir build -R '^frontend_hir_tests$' --output-on-failure`
  with a selected-global-load-to-single-input chain, misleading-display
  positives, and malformed ID, binding-fact, position/count, and type cases.
- Preserve Step-7.21 scalar output behavior, CC-LOAD-1, neighboring inline-asm
  metadata/diagnostics, and accepted generic identity coverage unchanged; the
  supervisor owns matched regression logs and any broader/full proof.
- Run `git diff --check` before handoff.
