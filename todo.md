Status: Active
Source Idea Path: ideas/open/prepared-consumer-missing-fact-diagnostics.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Prove Reuse Path For x86 Prepared Consumers

# Current Packet

## Just Finished

Step 4 proved the x86 prepared-consumer reuse path for the shared prealloc
diagnostic builder.

`x86::prepared::Query` now exposes `diagnose_home_storage(...)`, a narrow
wrapper that decodes x86 prepared home/storage facts and passes them through
`prepare::build_prepared_decoded_home_storage_diagnostic(...)`.

The existing x86 prepared internal test now proves reuse for multiple shared
diagnostic categories:

- missing value authority for absent decoded facts
- unsupported storage-plan authority for a present empty storage-plan record
- missing typed register authority for a diagnostic-only value-home register

The test verifies category, message specificity, and preservation of decoded
source/status/value facts. x86 lowering was not rewritten, and x86
target-specific emission/ABI diagnostics remain local.

## Suggested Next

Step 5 should validate diagnostics and anti-overfit coverage for the active
plan.

## Watchouts

Do not broaden validation into implementation changes unless a real blocker is
found. Function/block/instruction context diagnostics, AArch64 register
conversion, ABI/register spelling, operand construction, block mapping,
instruction mapping, call/variadic-specific diagnostics, and x86
emission/ABI diagnostics remain target-local or shared-MIR candidates for later
slices.

## Proof

Ran the delegated proof exactly:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'; } > test_after.log 2>&1`

Result: passed, 155/155 backend tests. Proof log: `test_after.log`.
