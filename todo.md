Status: Active
Source Idea Path: ideas/open/191_bir_function_signature_byval_metadata_text_retirement.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Inventory Function Signature Byval Authority

# Current Packet

## Just Finished

Completed Step 1 inventory for function-signature byval authority.

Findings:
- Exact metadata-rich text-authority site: `src/backend/bir/lir_to_bir/aggregate.cpp`
  in `BirFunctionLowerer::collect_aggregate_params`, where
  `signature_text_params = parse_function_signature_params(function_.signature_text)`
  is still computed when `has_structured_signature_params(function_)` is true.
  The byval decision then comes from
  `parse_byval_pointee_type((*signature_text_params)[index].type)`, so stale
  rendered `signature_text` can still decide whether a structured function
  parameter is treated as explicit byval for aggregate-param collection.
- Structured metadata producer: `populate_signature_type_refs` in
  `src/codegen/lir/hir_to_lir/hir_to_lir.cpp` fills
  `LirFunction::signature_params` and `signature_param_type_refs`; byval
  spelling is produced by `rendered_signature_param_type`, which emits
  `ptr byval(<aggregate>) align <n>` for AMD64 fixed aggregate byval
  parameters.
- Structured metadata gap: `LirSignatureParam` currently carries only
  `name` and `TypeSpec`, and `LirTypeRef` carries rendered type text plus
  optional structured aggregate identity. There is no explicit structured
  boolean/field saying this signature parameter was emitted as byval, so the
  backend infers byval from rendered type fragments.
- Existing structured boundary: `has_structured_signature_params` treats
  `signature_has_void_param_list`, `signature_is_variadic`,
  `signature_params`, or `signature_param_type_refs` as metadata-rich;
  `structured_signature_params` synthesizes `ParsedFunctionSignatureParam`
  from `signature_param_type_refs[index].str()` and
  `signature_params[index].name`.
- Legacy/no-metadata fallback boundary: `parse_function_signature_params` in
  `src/backend/bir/lir_to_bir/call_abi.cpp` remains appropriate only when
  `has_structured_signature_params(function)` is false, covering hand-built
  raw LIR fixtures that have only `signature_text`.

## Suggested Next

Execute Step 2: add the smallest explicit byval carrier at the existing
function signature metadata boundary, likely on `LirSignatureParam`, populate
it from `rendered_signature_param_type`/the same AMD64 aggregate-byval decision
used by the signature producer, and update verifier shape checks so
metadata-rich signature params cannot rely on stale `signature_text` to recover
byval state.

## Watchouts

- Do not treat `signature_text` as semantic authority when structured signature
  metadata exists.
- Keep legacy no-metadata text parsing explicit: `collect_aggregate_params` and
  `lower_function_signature_into` may parse `signature_text` only when
  `has_structured_signature_params(function)` is false.
- The call-ABI path in `lower_function_signature_into` already routes
  metadata-rich functions through `structured_signature_params`; avoid widening
  Step 2 into call-argument work from idea 190.
- Existing verifier tests prove signature type refs shadow stale rendered
  signature text, but the remaining `collect_aggregate_params` byval check is
  not covered until a stale-signature BIR aggregate-param test is added.
- Do not broaden into full call ABI redesign or unrelated aggregate local-slot
  lowering.

## Proof

Inventory-only packet; no build or test proof required and no proof logs were
created or modified.
