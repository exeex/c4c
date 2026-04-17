Status: Active
Source Idea: ideas/open/13_x86_backend_pure_bir_handoff_and_legacy_guardrails.md
Source Plan: plan.md

# Current Packet

## Just Finished

Started Step 2 with a real x86 target-local handoff for the minimal supported
pure-BIR slice: `emit_target_bir_module(..., Target::X86_64)` now prepares BIR
and enters `src/backend/mir/x86/codegen/direct_bir.cpp`, where the x86 route
emits assembly for simple direct-return modules instead of returning prepared
semantic BIR text.

## Suggested Next

Expand the x86 prepared-BIR consumer beyond the direct-return subset so the
public X86_64 BIR route can handle real prepared backend metadata without
falling back to prepared semantic BIR text for the next supported module
family.

## Watchouts

- `backend_x86_handoff_boundary` now proves that the public X86_64 BIR entry
  reaches x86 assembly rather than prepared semantic BIR text; do not regress
  it into a prepared-BIR printer check
- the new target-local handoff is intentionally narrow and lives in
  `src/backend/mir/x86/codegen/direct_bir.cpp`; broadening it should keep being
  capability-shaped rather than growing testcase-specific text matchers
- only the X86_64 public BIR entry uses the new route today; non-x86 targets
  and wider x86 module shapes still stop at prepared semantic BIR text

## Proof

Ran `cmake --build --preset default -j4 && ctest --test-dir build -j
--output-on-failure -R '^backend_'` and wrote the passing proving output to
`test_after.log`.
