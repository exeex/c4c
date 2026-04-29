Status: Active
Source Idea Path: ideas/open/127_lir_structured_function_signature_metadata_boundary.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Signature Text Consumers

# Current Packet

## Just Finished

Completed `plan.md` Step 1, `Inventory Signature Text Consumers`.

Runtime `LirFunction::signature_text` consumers found and classified:

- `src/codegen/lir/lir_printer.cpp:24` `render_signature_with_link_name(...)`
  and `src/codegen/lir/lir_printer.cpp:470` `render_fn(...)`:
  legitimate printer/render use. It rewrites only the rendered `@name` from
  `LinkNameId` while preserving final LLVM header spelling.
- `src/codegen/lir/hir_to_lir/hir_to_lir.cpp:1260` and `:1300`:
  producer writes for declarations and definitions, not consumers.
- `src/codegen/lir/hir_to_lir/hir_to_lir.cpp:1117` `collect_fn_refs(...)`:
  classified compatibility fallback. It scans final header/comment payload for
  embedded function references because no structured producer carrier exists
  for those references yet; this is not the function-signature fact authority
  targeted by this runbook.
- `src/codegen/lir/verify.cpp:642` through `:852`:
  verifier parser helpers `function_signature_line`,
  `signature_return_type_text`, `signature_param_texts`,
  `signature_param_declared_struct_name_id`, and
  `signature_type_ref_matches_text`. Classified as render-consistency checks
  today, but suspicious if treated as semantic signature authority. They parse
  the final header to compare `signature_return_type_ref` and
  `signature_param_type_refs` against rendered text.
- `src/backend/bir/lir_to_bir/call_abi.cpp:133`
  `parse_function_signature_params(...)`: suspicious semantic authority. It
  parses parameter type text, names, and `...` from the rendered header.
- `src/backend/bir/lir_to_bir/call_abi.cpp:228`
  `lower_signature_return_info(...)`: suspicious semantic authority. It parses
  return type text from the rendered header for BIR return type and ABI.
- `src/backend/bir/lir_to_bir/call_abi.cpp:271` and `:285` through `:403`
  `lower_function_params_with_layouts(...)`: suspicious semantic authority. It
  uses parsed header params for fixed-param count, variadic detection, void
  sentinel handling, aggregate/byval layout, parameter names, and fallback
  scalar parameter lowering.
- `src/backend/bir/lir_to_bir/aggregate.cpp:150`
  `collect_aggregate_params()`: suspicious semantic authority. It parses
  signature params to map aggregate parameters and byval layouts.
- `src/backend/bir/lir_to_bir/calling.cpp:1168`:
  suspicious semantic authority. Declaration lowering calls
  `lower_signature_return_info(function.signature_text, ...)`.
- `src/backend/bir/lir_to_bir/lowering.hpp:287` and `:526`:
  declarations for the BIR parser helpers above; classify with their
  definitions.
- `src/backend/mir/aarch64/codegen/emit.cpp:244`:
  suspicious semantic authority. `backend_lir_function_signature_uses_nonminimal_types(function.signature_text)`
  gates nonminimal-type handling from final header text.
- `src/backend/mir/aarch64/codegen/emit.cpp:561`, `:639`, `:1390`,
  `:1478`, `:1532`, `:2265`, `:2622`, `:2670`, `:2727`, `:2813`, and
  `:2868`: suspicious semantic authority. Multiple fast paths call
  `backend_lir_is_zero_arg_i32_definition(function.signature_text)`.
- `src/backend/mir/aarch64/codegen/emit.cpp:757` and `:984`:
  suspicious semantic authority. Fast paths call
  `backend_lir_is_i32_definition(function.signature_text)`.
- `src/backend/mir/aarch64/codegen/emit.cpp:4671`
  `gen_parse_function_return_type(...)`: suspicious parser helper, currently
  no direct caller found by `rg`. It parses the rendered signature return type
  for aarch64 generation support and should stay on the audit list until
  removed or replaced.

Test-only reads/helpers found:

- `tests/frontend/frontend_hir_tests.cpp:57` `overwrite_signature_name(...)`
  reads and edits `signature_text` to create a drifted-render test fixture;
  legitimate test helper.
- `tests/frontend/frontend_lir_function_signature_type_ref_test.cpp` and
  backend tests assign `signature_text` on synthetic LIR fixtures; these are
  producers/setup, not runtime semantic consumers.

Structured metadata needed by semantic consumers:

- Return facts: structured rendered-LLVM return type, `LirTypeRef` identity,
  aggregate layout identity/size/alignment, scalar/function-pointer lowering
  kind, and sret-return ABI classification.
- Parameter facts: ordered fixed parameters with stable lowered names,
  rendered-LLVM type text only as final spelling, `LirTypeRef` identity,
  lowered scalar kind, aggregate/byval layout identity/size/alignment, and
  void-parameter sentinel status.
- Variadic facts: explicit function-level variadic flag and fixed parameter
  count so BIR does not infer `...` by parsing header text.
- Declaration/definition facts: declaration bit already exists; aarch64 helper
  predicates need structured return type and parameter-count checks instead of
  `define i32 ...()` string predicates.
- Verifier facts: structured return/parameter mirrors should be primary; any
  retained `signature_text` parser should be only final-render consistency or
  fallback diagnostic text.

Step 1 is complete. Every discovered runtime `signature_text` read/parser
helper is classified above; no implementation behavior changed.

## Suggested Next

Delegate Step 2 to an executor: inspect existing `LirFunction` structured
signature carriers and add/thread the narrow missing metadata needed to replace
the BIR and aarch64 semantic parses listed above.

## Watchouts

Do not remove `signature_text` or change final header rendering. The highest
risk conversion points are BIR aggregate/byval/sret decisions and explicit
variadic metadata; converting only scalar return/param cases would leave the
main semantic authority path in place.

## Proof

No build or test command was required or run for this documentation-only
inventory packet. No proof logs were created or modified.
