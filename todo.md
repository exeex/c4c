# Execution State

Status: Active
Source Idea Path: ideas/open/81_convert_reviewed_x86_codegen_drafts_to_implementation_for_phoenix_rebuild.md
Source Plan Path: plan.md
Current Step ID: 4.1
Current Step Title: Rewire X86-Only Backend Entry Surfaces To Explicit Replacement Owners
Plan Review Counter: 1 / 6
# Current Packet

## Just Finished

Completed step 4.1 by rewiring the generic x86 LIR backend emit route in
`emit_module(...)` to call the explicit
`emit_x86_lir_module_entry(...)` replacement-owned surface instead of lowering
inline and bypassing the x86 owner. Updated the focused x86 LIR handoff
boundary proof so the generic backend emit assertion now checks the explicit
x86 LIR entry directly.

## Suggested Next

Audit the remaining step 4 x86-only backend entrypoints and compatibility
wrappers for any generic names that still bypass explicit replacement-owned
surfaces, especially around assemble and dump front doors.

## Watchouts

- The generic LIR `emit_module(...)` path still keeps the
  `preserve_dynamic_alloca = true` lowering path for semantic-BIR output and
  non-x86 fallback routes; only the x86 final-emission lane moved to the
  explicit owner surface.
- `emit_target_lir_module(...)` remains the compatibility wrapper, so follow-on
  cleanup should preserve honest non-x86 behavior while continuing to expose
  explicit x86 ownership at the public boundary.

## Proof

Step 4.1 explicit x86 LIR entry-routing packet on
2026-04-23:
`cmake --build --preset default`
`ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`
Backend subset passed (`106/106`). Canonical log paths: `test_before.log`,
`test_after.log`
