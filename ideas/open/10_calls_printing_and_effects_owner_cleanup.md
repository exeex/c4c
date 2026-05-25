# Calls Printing And Effects Owner Cleanup

## Goal

Move call machine-node printing and effect spelling out of call lowering when
they are not true lowering decisions.

## Why This Exists

`calls_printing.cpp` suggests call lowering ownership, but much of this kind of
logic belongs to machine-node printing, shared MIR effects, or target printer
hooks. Keeping printing/effects inside calls makes the calls family look larger
and encourages semantic decisions to hide in display code.

## In Scope

- Audit call printing/effect helper responsibilities.
- Move generic machine effect spelling to shared MIR or printer owners.
- Keep AArch64 target spelling hooks local only when they are target-specific.
- Delete call-lowering dependencies on printer-only helpers.
- Preserve existing assembly and diagnostic output.

## Out Of Scope

- Changing call argument selection.
- Rewriting the whole machine printer.
- Dispatch cleanup.

## Acceptance Criteria

- Printing/effect ownership is clearly separate from call semantic lowering.
- `calls_printing.cpp` is retired or renamed/split into accurate owners.
- Existing printer and backend tests pass.
- No behavior is changed except through ownership cleanup.

## Reviewer Reject Signals

- A patch changes call semantics while presented as printing cleanup.
- A patch moves printing code into another misleading calls helper.
- A patch removes effects used by diagnostics or regression tests.
- A patch reduces output coverage instead of preserving it.
