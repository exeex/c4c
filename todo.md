# Current Packet

Status: Active
Source Idea Path: ideas/open/849_hir_function_signature_direct_aggregate_ref_carrier.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Locate the signature semantic carrier seam

## Just Finished

- Plan 849 Step 1 complete: the smallest legal seam is an explicit HIR-only
  function-signature carrier passed into `Lowerer::lower_function`
  (`src/frontend/hir/impl/lowerer.hpp`; definition
  `src/frontend/hir/hir_functions.cpp:504`). It holds the already-issued,
  definition-backed `HirAggregateRef` fact for the return and each explicit
  parameter; `lower_function` forwards it via `append_callable_params` to
  `append_explicit_callable_param` (`hir_functions.cpp:1062`) without using
  the `Node` or normalized `TypeSpec` to obtain identity. Step 2 adds only
  this carrier/API; 848 Step 2b alone passes it to `qtype_from`.
- Direct source/contract: `HirStructDef::aggregate_ref` or
  `Module::aggregate_ref_for_definition(*definition)` in
  `src/frontend/hir/hir_ir.hpp` is acceptable only when complete and
  `module_->owns_aggregate_ref(ref)`. The carrier boundary rejects absent,
  incomplete, invalid/unissued, and foreign-module facts, retains no
  substitute, and forwards no canonical ref (fail closed).
- `TypeSpec` cannot be authority: both sites start from `fn_node->type` or
  `p->type` and mutate the result during callable/template/typedef preparation;
  current helper routes inspect `record_def`, structured owner/key, tag, and
  text. Those parser/type-derived recovery inputs are forbidden, so direct
  carrier state must be parallel to normalized `TypeSpec`.

## Suggested Next

- Plan 849 Step 2: define the compact carrier in `src/frontend/hir/hir_ir.hpp`;
  add validated construction/acceptance plus forwarding boundaries in
  `src/frontend/hir/impl/lowerer.hpp` and `src/frontend/hir/hir_functions.cpp`;
  update `src/frontend/hir/hir_build.cpp` only to provide a pre-existing direct
  HIR definition/ref fact (or no fact), never deriving one from metadata.

## Watchouts

- Do not use parser, `TypeSpec`, `record_def`, owner/structured-owner, tag,
  text, parser-pointer, `Node*` map, or reconstructed lookup as canonical
  identity.
- Do not edit `qtype_from`, attach occurrence refs, or change LIR; those are
  outside this blocker and 848 resumes Step 2b after acceptance.
- Existing `qtype_from` validation is not Step 849 work. The new carrier
  boundary must itself fail closed; it may not fall back to any lookup.

## Proof

- No-code discovery packet; no build or test required. AST-backed
  `c4c-clang-tool` queries confirmed `lower_function` at :504,
  `append_explicit_callable_param` at :1062, and the existing
  `HirAggregateRef` input boundary in `lowerer.hpp:379`. The compile-db route
  is unavailable because `build/compile_commands.json` has no entry here.
