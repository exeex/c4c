Status: Active
Source Idea Path: ideas/open/115_hir_to_lir_layout_legacy_decl_demotion.md
Source Plan Path: plan.md
Current Step ID: Step 5
Current Step Title: Consolidate Remaining HIR-to-LIR Size And Alignment Consumers

# Current Packet

## Just Finished

Step 5 final HIR-to-LIR `legacy_decl` consolidation scan completed after the
structured size/alignment helper work. Focused reads of every remaining hit
under `src/codegen/lir/hir_to_lir` found no remaining active semantic
`legacy_decl` authority bug.

Remaining reads classify as:
- observation/carrier state: `StructuredLayoutLookup::legacy_decl` itself and
  `record_structured_layout_observation` legacy presence, size, alignment, and
  field-type reporting
- helper parity gate: `structured_layout_align_bytes`,
  `structured_layout_size_bytes`, and `lookup_structured_layout` compare
  structured declarations against the legacy declaration before trusting
  derived layout
- structured-first fallback: module/statement object alignment, variadic
  aggregate argument payload size, and `va_arg` aggregate payload size all ask
  the structured helper first, then fall back only when structured coverage is
  absent, conservative, or parity-rejected

## Suggested Next

Next coherent packet should move to Step 6 acceptance validation for the
HIR-to-LIR layout demotion route.

## Watchouts

- Keep `legacy_decl` fallback for incomplete structured coverage; do not remove
  `StructuredLayoutLookup::legacy_decl`.
- Do not edit `src/backend/mir/` as part of this route.
- Do not weaken tests or add testcase-shaped shortcuts.
- Step 5 was scan-only; no code churn was needed because the remaining
  `legacy_decl` reads are observation, helper parity gates, or structured-first
  fallbacks.
- Deferred/out-of-scope coverage remains structured union/member layout
  derivation and any broader raw LLVM type-text/nested-`LirStructDecl`
  coverage that would allow fewer conservative helper fallbacks later.
- `structured_layout_align_bytes` intentionally rejects unions because the
  current structured union mirror is `[N x i8]` and does not encode union
  member alignment.
- Size derivation remains conservative for unknown raw LLVM type text, missing
  nested `LirStructDecl`s, parity absence, parity mismatch, or derived-size
  mismatch; those cases continue to use legacy fallback.
- `structured_layout_size_bytes` allows zero as a valid structured result so
  zero-sized aggregate skip/zeroinitializer behavior remains explicit.

## Proof

Passed delegated scan-only proof, with output preserved in `test_after.log`:
`rg -n "legacy_decl" src/codegen/lir/hir_to_lir > test_after.log 2>&1`

Result: the scan returned only classified observation, helper parity-gate, and
structured-first fallback reads. No build was required because this packet made
no code changes.
