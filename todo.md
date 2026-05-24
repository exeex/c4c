Status: Active
Source Idea Path: ideas/open/prepared-consumer-missing-fact-diagnostics.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Adapt AArch64 To Use Shared Builders

# Current Packet

## Just Finished

Step 3 adapted AArch64 operand prepared-consumer diagnostics to use the shared
prealloc diagnostic builder.

`src/backend/mir/aarch64/codegen/operands.cpp` now calls
`prepare::build_prepared_decoded_home_storage_diagnostic(...)` for decoded
home/storage missing-fact failures and maps the returned target-neutral
category back to the existing AArch64 `ModuleLoweringDiagnosticKind`.

Preserved AArch64 diagnostic behavior:

- `MissingValueAuthority`, `MissingTypedRegisterAuthority`,
  `UnsupportedStoragePlan`, and `UnsupportedValueHome` kind mapping remains
  AArch64-local.
- Message specificity remains source-specific through the shared builder,
  including distinct regalloc, storage-plan, and value-home register-placement
  messages.
- Missing function context and AArch64 register-conversion diagnostics remain
  local and unchanged.

This was a diagnostics-only extraction; no functional lowering behavior was
changed or claimed.

## Suggested Next

Step 4 should prove an x86 prepared-consumer reuse path for the shared
diagnostic builders without rewriting x86 lowering or moving x86-specific
emission/ABI diagnostics into prealloc.

## Watchouts

Do not broaden the next packet into function/block/instruction context
diagnostics, AArch64 register conversion, ABI/register spelling, operand
construction, block mapping, instruction mapping, or call/variadic-specific
diagnostics. Those remain target-local or shared-MIR candidates for later
slices.

## Proof

Ran the delegated proof exactly:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'; } > test_after.log 2>&1`

Result: passed, 155/155 backend tests. Proof log: `test_after.log`.
