# Current Packet

Status: Active
Source Idea Path: ideas/open/142_codegen_lir_aggregate_type_identity_metadata.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Probe TypeSpec Tag Removal Boundary

## Just Finished

Step 5 - Probe TypeSpec Tag Removal Boundary:
reran the temporary `TypeSpec::tag` deletion probe after the ABI migration. The
first remaining codegen/LIR compile boundary is now
`src/codegen/lir/verify.cpp:664`, where direct aggregate signature verification
still gates on `TypeSpec::tag`. The same probe also reaches many
`tests/frontend/frontend_parser_tests.cpp` `TypeSpec::tag` references after the
verifier failure; those test residuals are outside this codegen/LIR packet
family and should be classified after the verifier boundary is cleared.

## Suggested Next

Next coherent packet: migrate the LIR verifier boundary in
`src/codegen/lir/verify.cpp` so direct aggregate signature verification uses
structured `LirTypeRef`/`StructNameId` metadata or existing named compatibility
helpers instead of gating on `TypeSpec::tag`. After that, rerun the deletion
probe and classify whether the remaining frontend parser-test residuals belong
back to the parent idea rather than this codegen/LIR follow-up.

## Watchouts

- The probe edit was temporary: `const char* tag` in
  `src/frontend/parser/ast.hpp` was restored before the post-probe rebuild.
- `types.cpp` still writes `FieldStep::tag` for GEP text and observes
  `tag_text_id`; those are not `TypeSpec::tag` residuals.
- `src/codegen/llvm/calling_convention.cpp` no longer contains `.tag` reads;
  the remaining codegen-owned direct `.tag` read reported by the probe is in
  `src/codegen/lir/verify.cpp`.
- Do not modify frontend parser tests as part of the verifier packet.

## Proof

Probe command:
`cmake --build --preset default > test_after.log 2>&1` with `const char* tag`
temporarily disabled failed at the verifier boundary above.

After reverting the temporary probe edit, `cmake --build --preset default`
passed. The accepted focused baseline remains in `test_before.log`; the current
`test_after.log` is the failed deletion-probe artifact and should be
overwritten by the next executor proof.
