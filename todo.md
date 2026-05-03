# Current Packet

Status: Active
Source Idea Path: ideas/open/142_codegen_lir_aggregate_type_identity_metadata.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Probe TypeSpec Tag Removal Boundary

## Just Finished

Step 5 - Probe TypeSpec Tag Removal Boundary:
reran the temporary `TypeSpec::tag` deletion probe after the `types.cpp`
field-chain migration. The first remaining compile boundary is now
`src/codegen/llvm/calling_convention.cpp:116-117`, where the ABI lookup helper
still falls back to `mod.struct_defs.find(ts.tag)`. Same-wave residual remains
in `src/codegen/lir/verify.cpp:664`, where direct aggregate signature
verification still gates on `TypeSpec::tag`.

## Suggested Next

Next coherent packet: migrate the ABI fallback in
`src/codegen/llvm/calling_convention.cpp` so aggregate layout lookup uses
structured owner metadata or existing named compatibility helpers instead of
direct `TypeSpec::tag`. Keep `src/codegen/lir/verify.cpp` as the following
packet unless a tiny shared helper naturally covers both.

## Watchouts

- The probe edit was temporary: `const char* tag` in
  `src/frontend/parser/ast.hpp` was restored before the post-probe rebuild.
- `types.cpp` still writes `FieldStep::tag` for GEP text and observes
  `tag_text_id`; those are not `TypeSpec::tag` residuals.
- Keep the ABI patch focused; verifier checks are LIR-owned and can be handled
  separately after another probe.

## Proof

Probe command:
`cmake --build --preset default > test_after.log 2>&1` with `const char* tag`
temporarily disabled failed at the ABI boundary above.

After reverting the temporary probe edit, `cmake --build --preset default`
passed. The accepted focused baseline remains in `test_before.log`; the current
`test_after.log` is the failed deletion-probe artifact and should be
overwritten by the next executor proof.
