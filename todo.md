Status: Active
Source Idea Path: ideas/open/191_bir_function_signature_byval_metadata_text_retirement.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Prove Stale Signature Text Cannot Override Metadata

# Current Packet

## Just Finished

Completed Step 3 by routing incoming aggregate parameter collection through
structured byval metadata when structured signature params exist.

Changes:
- Extended `ParsedFunctionSignatureParam` with an explicit `is_byval` fact.
- Made `structured_signature_params` populate that fact from
  `LirSignatureParam::is_byval`.
- Removed the metadata-rich `collect_aggregate_params` parse of
  `function_.signature_text` for byval detection.
- Kept `parse_function_signature_params(function_.signature_text)` as the
  no-structured-metadata fallback, where parsed text still supplies legacy
  byval state.
- Added backend coverage proving stale byval spelling in `signature_text` does
  not rescue structured metadata when the structured byval flag is absent.

## Suggested Next

Execute Step 4: add or review focused stale-text evidence around structured
function-signature byval authority, including positive structured byval and
legacy no-metadata fallback behavior.

## Watchouts

- `lower_function_params_with_layouts` now uses `param.is_byval` for the
  structured aggregate-layout requirement, but valid structured byval layout
  still depends on the existing `signature_param_type_refs` type text and
  `StructNameId` boundary.
- The no-metadata compatibility path still parses `signature_text`; that is
  intentional and should remain isolated to `!has_structured_signature_params`.
- The new backend failure case reports through the local-memory semantic family
  because aggregate-param alias materialization is where the stale structured
  byval fact becomes fatal.

## Proof

Ran:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_lir_function_signature_type_ref|backend_lir_to_bir_notes)$' > test_after.log`

Result: passed, `2/2` tests green.

Proof log: `test_after.log`.

Supervisor also ran:

`ctest --test-dir build -j --output-on-failure -R '^backend_'`

Result: passed, `109/109` runnable tests green; `12` matching tests disabled.
