# Current Packet

Status: Active
Source Idea Path: ideas/open/762_lir_module_declaration_type_shadow_convergence.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit and select one module declaration authority surface

## Just Finished

Step 1 audit complete — selected **struct declarations only**.  Authority is
`LirModule::struct_decls` (`LirStructDecl{name_id, fields: LirTypeRef,
is_packed, is_opaque}`), with `StructNameTable` resolving `name_id`.
`hir_to_lir::build_type_decls` constructs and records that carrier (including
structured padding/composite field refs); `verify_struct_decl_shadows` requires
every field to be a valid module type ref and rejects a missing or mismatched
legacy declaration; `print_llvm` renders only `struct_decls`; and
`lir_to_bir::build_bir_structured_type_spelling_context` plus
`build_backend_structured_layout_table` consume the declarations for structured
type/layout decisions.  The retained `type_decls` vector is the exact legacy
output/compatibility shadow, not the selected authority.

Matrix for the selected row:

- Positive: a named, packed/opaque, or composite-field `LirStructDecl` whose
  rendered declaration equals its `type_decls` shadow verifies, prints the
  structured rendering, and supplies the BIR structured layout table.
- Malformed stale shadow: mutate only the matching `type_decls` line (field,
  packing, opacity, or name spelling); verification must fail before print or
  backend lowering.  A structured-present backend layout must continue to use
  `struct_decls` rather than a conflicting legacy layout (existing structured
  layout probes establish that consumer behavior).

Excluded for this packet: extern declarations — producer constructs
`return_type` from `return_type_str`, printer emits `return_type_str`, and BIR
tries that text before `return_type.str()`; function signatures — printer still
uses `signature_text` and the full header has no structured renderer; globals
— printer emits `llvm_type` and global lowering still takes textual type input
despite optional `llvm_type_ref`.  Each lacks a complete structured
producer/verifier/printer/consumer seam and remains outside Step 2.

## Suggested Next

Step 2: make only the selected struct-declaration shadow contract explicit in
the implementation/tests: render `type_decls` from `struct_decls` at its
compatibility boundary (or retain it as a checked emission shadow), and add
same-feature positive plus stale-shadow rejection coverage without touching
extern/signature/global text routes. Suggested proof: `cmake --build --preset
default && ctest --test-dir build --output-on-failure -R
'^frontend_lir_extern_decl_type_ref$'`; include the existing structured-layout
consumer case or a registered equivalent when selecting the broader checkpoint.

## Watchouts

`type_decls` must never be reparsed to recover selected struct identity or
layout when the matching `LirStructDecl` exists; stale text must fail closed.
Do not expand this row into the unresolved `return_type_str`, `signature_text`,
or `llvm_type` surfaces.  `LirTypeRef::runtime_text` remains an explicit
compatibility boundary for type forms not represented by the 763 composite
carrier, not a reason to select another surface.

## Proof

Audit only; no build or code proof was delegated or run. Step 2 proof proposed
above; `test_after.log` is intentionally untouched for this non-code packet.
