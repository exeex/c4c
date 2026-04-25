# HIR To LIR TextId Bridge Inventory And Cleanup

Status: Open
Created: 2026-04-25
Last Updated: 2026-04-25

Parent Ideas:
- [103_hir_post_dual_path_legacy_readiness_audit.md](/workspaces/c4c/ideas/closed/103_hir_post_dual_path_legacy_readiness_audit.md)
- [104_hir_safe_legacy_lookup_demotion.md](/workspaces/c4c/ideas/closed/104_hir_safe_legacy_lookup_demotion.md)

## Goal

Inventory and start cleaning the HIR-to-LIR boundary where semantic identity is
still passed as rendered strings instead of `TextId`, `LinkNameId`, typed IDs,
or structured references.

The long-term direction is that text-table-backed identity should flow through
the whole frontend-to-backend pipeline. A `TextId` should let later stages reach
`len + const char*` with near-zero lookup cost, rather than repeatedly
allocating or comparing rendered strings as semantic keys.

## Why This Idea Exists

HIR structured lookup cleanup cannot fully finish while downstream lowering
still requires strings for semantic handoff. Current LIR already includes the
shared text table header, but the HIR-to-LIR seam still has many string bridges:

- fallback callee and global names
- raw extern declaration names
- struct tags from `TypeSpec::tag`
- rendered LLVM type strings
- pre-formatted call argument strings
- constant initializer text
- field and designator names
- labels and user labels
- string pool names and raw bytes
- printer-only textual operands

Some of these strings are legitimate final-output spellings. Others are still
semantic identity crossing the HIR-to-LIR boundary in rendered form. This idea
marks the seam precisely and starts moving identity-bearing strings to
structured IDs.

## Inventory Scope

Audit at least:

- `src/codegen/lir/ir.hpp`
- `src/codegen/lir/types.hpp`
- `src/codegen/lir/operands.hpp`
- `src/codegen/lir/hir_to_lir/`
- `src/codegen/shared/fn_lowering_ctx.hpp`
- `src/codegen/shared/llvm_helpers.hpp`
- `src/codegen/llvm/`
- HIR fields consumed by LIR lowering:
  - `Function::name`, `Function::link_name_id`, `Function::name_text_id`
  - `GlobalVar::name`, `GlobalVar::link_name_id`, `GlobalVar::name_text_id`
  - `DeclRef::name`, `DeclRef::link_name_id`, structured decl metadata
  - `TypeSpec::tag`
  - `HirStructDef::tag` and record-owner structured keys
  - `HirStructField::name` and field text/member IDs

## Required Seam Classification

Classify every relevant HIR-to-LIR string use as:

- `semantic-identity`: should become `TextId`, `LinkNameId`, typed ID, or
  structured key.
- `type-identity`: should become a typed LIR type ref or HIR record-owner key,
  with rendering delayed to printer/backend.
- `abi-link-spelling`: link-visible spelling; keep `LinkNameId` authoritative
  and string spelling as emitted text.
- `printer-only`: final textual LLVM or assembly output.
- `diagnostic-debug`: debug or error text only.
- `literal-bytes`: string literal payload or inline asm payload, not symbol
  identity.
- `temporary-rendered-lowering`: still rendered today, but should be scheduled
  for structured replacement.

## Suggested Cleanup Direction

The first cleanup slices should prefer low-risk identity handoffs:

1. Use `LinkNameId` and `TextId` for function/global identity across HIR-to-LIR
   call and const-init lowering before falling back to raw names.
2. Add explicit LIR fields for symbol identity where raw `std::string name`
   currently acts as both identity and spelling.
3. Split type identity from LLVM text by introducing or extending typed LIR type
   refs before removing rendered type strings.
4. Keep string literal bytes, inline asm text, and final printer output as
   strings because they are payload or output text rather than lookup identity.
5. Preserve rendered fallback during the migration and prove parity before
   deletion.

## Out Of Scope

- Removing all LIR strings in one pass.
- Rewriting final LLVM printer output away from text.
- Replacing literal payload strings such as string literals or inline asm.
- Changing ABI/link-visible spelling behavior.
- HIR-internal legacy cleanup already covered by idea 104.

## Required Output

Write a review or plan artifact that lists:

- each HIR-to-LIR seam string category
- whether it should become `TextId`, `LinkNameId`, typed ID, structured record
  owner key, or remain string text
- first low-risk cleanup slice
- validation subsets needed for LIR/codegen stability

## Acceptance Criteria

- HIR-to-LIR string seams are explicitly marked and classified.
- Semantic identity crossing the boundary has a migration path toward
  text-table-backed IDs or typed structured references.
- The plan preserves near-zero-cost `TextId` lookup as the intended long-term
  identity path through backend stages.
- Printer-only, literal-byte, diagnostic, and ABI/link spelling strings are not
  confused with semantic lookup keys.
- Any implemented cleanup remains behavior-preserving and is validated with
  focused HIR/LIR/codegen proof.
