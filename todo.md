# Execution State

Status: Active
Source Idea Path: ideas/open/81_convert_reviewed_x86_codegen_drafts_to_implementation_for_phoenix_rebuild.md
Source Plan Path: plan.md
Current Step ID: 4.3
Current Step Title: Prove The Live Replacement Dispatch Path Before Legacy Retirement
Plan Review Counter: 0 / 6
# Current Packet

## Just Finished

Extended step 4.3 proof so the focused x86 LIR handoff boundary test now
asserts the generic backend `dump_module(...)` front door matches the same
semantic-BIR, prepared-BIR, and target-local route-debug outputs produced by
the canonical x86 lowering and prepared-module path.

## Suggested Next

Decide whether the remaining step 4 proof should add a non-x86 compatibility
assertion around the thin target-wrapper shims, or whether step 5 should take
over with explicit legacy retirement/classification work.

## Watchouts

- The generic LIR dump path still uses `preserve_dynamic_alloca = true`, so
  dump proof must continue to compare against that exact lowering contract
  rather than the emit-path lowering defaults.
- `emit_target_lir_module(...)` and `assemble_target_lir_module(...)` remain
  compatibility wrappers; follow-on cleanup must preserve honest non-x86
  behavior while keeping x86 ownership explicit.

## Proof

Step 4.3 explicit x86 LIR dump-route proof packet on
2026-04-23:
`cmake --build --preset default`
`ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`
Backend subset passed (`106/106`). Canonical log paths: `test_before.log`,
`test_after.log`
