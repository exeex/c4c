Status: Active
Source Idea Path: ideas/open/05_bir-memory-intrinsic-family-extraction.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Map the Intrinsic Family Boundary

# Current Packet

## Just Finished

Lifecycle activation created `plan.md` and this executor-compatible `todo.md`
from `ideas/open/05_bir-memory-intrinsic-family-extraction.md`. No
implementation work has started.

## Suggested Next

Begin Step 1 in `plan.md`: inspect the target memory intrinsic family in
`src/backend/bir/lir_to_bir/memory/local_slots.cpp`, classify helper
dependencies, identify build registration for a new memory `.cpp`, and choose
the narrow proof command for memcpy, memset, and direct runtime memory
intrinsic lowering.

## Watchouts

- Do not add new headers.
- Do not edit testcase expectations.
- Do not convert the moved functions away from `BirFunctionLowerer` members.
- Keep local-slot-only helpers in `local_slots.cpp`.

## Proof

Not run. Lifecycle activation only; no implementation code was changed.
