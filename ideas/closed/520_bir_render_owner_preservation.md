# BIR Render Owner Preservation

Status: Closed
Type: Behavior-preserving cleanup
Parent: `ideas/closed/518_bir_core_model_cleanup_umbrella.md`
Order: BIR cleanup follow-up 1 of 13, before route body extraction and before `ideas/open/519_rv64_object_emission_cleanup_umbrella.md`
Owning Layer: BIR printer/render helper ownership
Source Artifact: `docs/bir_core_cleanup/follow_up_ideas.md`

## Closure Note

Closed after moving `render_type`, `render_binary_opcode`, and
`render_cast_opcode` from central `bir.cpp` into a narrow public render
translation unit. The audit found mixed public/non-printer consumers, so the
helpers were not moved under `bir_printer.cpp`; public declarations and
render/diagnostic behavior were preserved.

Close proof used the accepted backend subset validation for the ownership move:
default build plus `ctest --test-dir build -j --output-on-failure -R
'^backend_'` passed, and the regression guard showed 345/345 backend tests
passing with no new failures.

## Goal

Preserve existing printer and validator ownership, then isolate only the public
render helper bodies from `src/backend/bir/bir.cpp` if consumer checks show a
safe destination.

## Why This Exists

Idea 518 found that printer and validator implementation ownership is already
mostly healthy, while public render helper bodies still live in the central
`bir.cpp`. This first cleanup slice should keep the obvious owners stable and
avoid mixing render movement with route extraction.

## In Scope

- Audit public render helper consumers.
- Move or preserve `render_type`, `render_binary_opcode`, and
  `render_cast_opcode` bodies according to the 518 destination map.
- Keep public declarations, names, signatures, spelling, and diagnostics
  unchanged.
- Use `.codex/skills/c4c-clang-tools/` before editing to confirm callers and
  dependency direction.

## Out Of Scope

- Do not move route analysis declarations or records.
- Do not change `print(Module, ...)` or `validate(Module, ...)` behavior.
- Do not fold route-index validation helpers into module validation.
- Do not start route body extraction in this slice.

## Acceptance Criteria

- Build proof passes.
- Focused BIR printer/validator proof passes, or the nearest repo-native
  backend subset passes if no narrower target exists.
- Render text and validation diagnostics remain unchanged.
- Any moved body has a clear owner and does not introduce printer-only
  dependencies into non-printer users.

## Reviewer Reject Signals

- Render text changes without explicit approval.
- New non-printer dependencies on printer-only implementation details.
- Route declarations, route validation, or public model records move under a
  printer or validator owner.
- Route extraction is combined with render cleanup.
