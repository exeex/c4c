Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 7.43
Current Step Title: Receive DirectScalar Fneg Parameter Authority

# Current Packet

## Just Finished

Repaired Step 7.43 receiver support for the closed-854 DirectScalar unary
`fneg` body-parameter authority row. Raw-BIR `FNeg` builder admission is now
limited to the selected DirectScalar authority tuple for the current-function
parameter used as `fneg` `lhs`: parameter `LirValueId`, owner, parameter index,
matching scalar type, DirectScalar ABI, explicit `Lhs` role, unary `fneg`
opcode, lhs parameter value, and empty rhs; the backend importer now passes an
invalid Raw-BIR RHS for that validated unary row instead of a synthetic
constant. Focused receiver coverage now checks the positive tuple and missing,
invalid, foreign, duplicate,
owner/index/type/ABI/role, definition scalar-type mismatch, signature
parameter-type mismatch, non-`fneg`, lhs mismatch, rhs-populated, and
duplicate-consumer failures.

## Suggested Next

Ask plan-owner to reassess source completion for Step 7.43/734 after the
fresh proof in `test_after.log`; if closure is rejected, request the exact
remaining in-scope receiver criterion or named successor.

## Watchouts

The builder admits unary Raw-BIR `FNeg` only through the direct-scalar fneg
authority path repaired here; the importer and foundation verifier retain the
owner/index/type/operand relation checks. No LIR producer/schema/frontend
files were changed.

## Proof

Final delegated proof passed and is preserved in `test_after.log`:
`( cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^backend_lir_to_bir_interface$' ) > test_after.log
2>&1 && git diff --check`.
