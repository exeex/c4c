# Execution State

Status: Active
Source Idea Path: ideas/open/81_convert_reviewed_x86_codegen_drafts_to_implementation_for_phoenix_rebuild.md
Source Plan Path: plan.md
Current Step ID: 4.3
Current Step Title: Prove The Live Replacement Dispatch Path Before Legacy Retirement
Plan Review Counter: 1 / 6
# Current Packet

## Just Finished

Extended step 4.3 proof so the focused x86 LIR handoff boundary test now
asserts the thin non-x86 `emit_target_lir_module(...)` and
`assemble_target_lir_module(...)` compatibility wrappers still preserve the
generic prepared-BIR and bootstrap-assemble contracts while x86 ownership
stays explicit on the replacement path.

## Suggested Next

Decide whether step 4.3 still needs any front-door proof beyond the current
x86 emit/assemble/dump and non-x86 compatibility coverage, or whether step 5
should now start explicit residual legacy classification and broader
validation.

## Watchouts

- The generic LIR dump path still uses `preserve_dynamic_alloca = true`, so
  dump proof must continue to compare against that exact lowering contract
  rather than the emit-path lowering defaults.
- `emit_target_lir_module(...)` and `assemble_target_lir_module(...)` still
  owe honest generic behavior for non-x86 targets, so step 5 retirement work
  must not collapse those wrappers into x86-specific ownership.

## Proof

Step 4.3 non-x86 compatibility-wrapper proof packet on
2026-04-23:
`cmake --build --preset default`
`ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`
Backend subset passed (`106/106`). Canonical log paths: `test_before.log`,
`test_after.log`
