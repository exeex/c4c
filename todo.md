Status: Active
Source Idea Path: ideas/open/06_bir-call-lowering-exit-from-memory-coordinator.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Declare Call Lowering Member

# Current Packet

## Just Finished

Lifecycle activation created `plan.md` and this executor-compatible `todo.md`
for Step 1.

## Suggested Next

Start with Step 1 in `plan.md`: declare `lower_call_inst(...)` as a
`BirFunctionLowerer` member in `src/backend/bir/lir_to_bir/lowering.hpp`.

## Watchouts

Keep this as a behavior-preserving placement cleanup. Do not change call ABI,
pointer provenance, memory intrinsic behavior, or test expectations.

## Proof

No code validation was run during lifecycle activation.
