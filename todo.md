Status: Active
Source Idea Path: ideas/open/prepared-consumer-missing-fact-diagnostics.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Add Shared Prepared Diagnostic Builders

# Current Packet

## Just Finished

Step 2 added shared prealloc diagnostic builders for decoded prepared
home/storage operand authority failures.

`src/backend/prealloc/decoded_home_storage.hpp/.cpp` now exposes
`PreparedDecodedHomeStorageDiagnosticCategory`,
`PreparedDecodedHomeStorageDiagnostic`, and
`build_prepared_decoded_home_storage_diagnostic(...)`.

The builder preserves decoded source, kind, status, function/value identity,
and emits target-neutral category/message pairs for:

- missing value authority
- missing typed register authority
- unsupported storage-plan authority
- unsupported value-home authority

Messages preserve the existing specificity from the AArch64-local inventory,
including separate register-placement messages for regalloc, storage-plan, and
value-home sources. The API does not depend on AArch64 registers, MIR
operands, ABI conversion, instruction mapping, or emission.

Direct coverage was added to `backend_prealloc_decoded_home_storage` for each
selected category plus decoded fact preservation and category-name spelling.

## Suggested Next

Step 3 should adapt AArch64 operand prepared-consumer diagnostics to call the
shared builders for the selected decoded home/storage missing-fact categories,
while preserving diagnostic kind mapping and target-local register conversion
diagnostics.

## Watchouts

Do not broaden Step 3 into function/block/instruction context diagnostics,
AArch64 register conversion, ABI/register spelling, operand construction,
block mapping, instruction mapping, or call/variadic-specific diagnostics.
Those remain target-local or shared-MIR candidates for later slices.

## Proof

Ran the delegated proof exactly:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'; } > test_after.log 2>&1`

Result: passed, 155/155 backend tests. Proof log: `test_after.log`.
