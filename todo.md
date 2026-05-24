Status: Active
Source Idea Path: ideas/open/prealloc-stack-layout-contract-boundary.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Align Prepared Frame Printing And Close

# Current Packet

## Just Finished

Step 5 - Align Prepared Frame Printing And Close completed the final
prepared-printer and closure-readiness audit with no code changes.

Audited `src/backend/prealloc/prepared_printer/frame.cpp` against the completed
stack-layout boundary cleanup:
- `append_stack_layout(...)` still prints the public stack-layout object and
  frame-slot families without hiding or reclassifying facts.
- `append_frame_plan(...)` still prints frame sizing, alignment, saved
  register slots, and frame-slot ordering with the same field meanings.
- `append_dynamic_stack_plan(...)` still prints dynamic stack operation facts
  under the existing dynamic-stack family.
- No public data-family names changed during this plan, so printer terminology
  does not need alignment.

Changed files:
- `todo.md`

Behavior notes:
- Accumulated plan changes moved stack-layout hook declarations and
  `FunctionInlineAsmSummary` to `stack_layout/stack_layout.hpp`, then extracted
  file-local object-planning and addressing-publication helpers in
  `stack_layout/coordinator.cpp`.
- The prepared dump section names and field meanings remain aligned with the
  durable data contracts: stack layout, frame plan, and dynamic stack plan.
- No frame/stack facts were hidden, dropped, or renamed.
- The source idea appears closure-ready: stack-layout ownership is clearer from
  the contract header and helper names, `module.hpp` remains intentionally
  aggregate, backend proof has stayed green, and prepared frame dump meaning is
  preserved.

## Suggested Next

Ask the plan owner to close
`ideas/open/prealloc-stack-layout-contract-boundary.md`.

## Watchouts

- Deferred follow-up candidates, if desired as separate ideas: further
  `run_stack_layout()` decomposition around slot assignment orchestration, and
  any broader frame-plan naming cleanup. Neither is required to close this
  source idea.
- Do not fold target instruction emission or register spelling into prealloc;
  that stayed outside the stack-layout contract boundary throughout this plan.

## Proof

Ran:

```sh
git diff --check
```

Result: passed.

Proof log path: none. This was a no-code audit packet, so no `test_after.log`
was created.
