Status: Active
Source Idea Path: ideas/open/127_lir_structured_function_signature_metadata_boundary.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Complete Structured LIR Signature Metadata

# Current Packet

## Just Finished

Completed `plan.md` Step 2, `Complete Structured LIR Signature Metadata`.

Metadata that already existed:

- `LirFunction::is_declaration` already carries declaration versus definition.
- `LirFunction::return_type` already carries the HIR return `TypeSpec`.
- `LirFunction::signature_return_type_ref` already carries rendered LLVM
  return spelling plus `StructNameId` when available.
- `LirFunction::signature_param_type_refs` already carries ordered fixed
  parameter rendered LLVM fragments plus `StructNameId` when available,
  including byval spelling such as `ptr byval(%struct.Big) align 8`.
- Declaration `LirFunction::params` already carried lowered names and HIR
  parameter `TypeSpec`; definition `params` intentionally remains unchanged
  because current BIR still reads it as a legacy behavior surface.

Metadata added/threaded:

- Added `LirSignatureParam` and `LirFunction::signature_params` as the
  behavior-preserving ordered fixed-parameter carrier for lowered parameter
  names plus HIR `TypeSpec` on both declarations and definitions.
- Added `LirFunction::signature_is_variadic` so BIR/aarch64 can stop inferring
  `...` from `signature_text`.
- Added `LirFunction::signature_has_void_param_list` so consumers can
  distinguish `f(void)` from an empty fixed-parameter list without parsing the
  header.
- Threaded those fields from HIR-to-LIR alongside the existing return and
  parameter type-ref mirrors without changing final `signature_text` rendering.
- Extended `frontend_lir_function_signature_type_ref_test` to cover
  declaration/definition parity, aggregate and byval fixed parameters,
  variadic declarations/definitions, and void-parameter-list metadata.

## Suggested Next

Delegate Step 3 to convert BIR signature lowering to prefer
`signature_return_type_ref`, `signature_param_type_refs`, `signature_params`,
`signature_is_variadic`, `signature_has_void_param_list`, and
`is_declaration` over `signature_text` parsing.

## Watchouts

Do not switch current BIR to legacy definition `params` for aggregate/byval
signatures. A trial population of definition `params` made the existing BIR
aggregate path fail on `00204.c`/`fa_s17`; Step 3 should consume the new
`signature_params` carrier and normalize `signature_param_type_refs` byval
fragments deliberately. `signature_text` remains final spelling and render
compatibility data only.

## Proof

Targeted pre-proof check:
`cmake --build --preset default --target frontend_lir_function_signature_type_ref_test && ctest --test-dir build -j --output-on-failure -R '^frontend_lir_function_signature_type_ref$'`
passed.

Supervisor-selected proof:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log 2>&1`
passed. Proof log: `test_after.log`.
