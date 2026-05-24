# AArch64 Calls Dispatch Bridge Helper Absorption

## Intent

Absorb the bounded helper glue in `calls_dispatch_bridge.*` into the durable
call-boundary adapter ownership without changing call ABI behavior or the
dispatch contract.

This is a behavior-preserving refactor idea only. It should make the bridge
smaller by moving, inlining, or consolidating thin wrappers whose only job is
to translate dispatch facts into call-lowering inputs.

## Target Files

- `src/backend/mir/aarch64/codegen/calls_dispatch_bridge.cpp`
- `src/backend/mir/aarch64/codegen/calls_dispatch_bridge.hpp`
- Adjacent call or dispatch adapter owner files only when a helper is moved to
  an already-durable owner.

Do not broaden this into `dispatch.cpp` or whole-call-family cleanup.

## Refactor Type

`Helper absorption`

Use exactly this slice type. Do not combine it with phase extraction, naming
repair, call ABI redesign, or dispatch route redesign.

## Durable Owner

Call-boundary adapter logic that translates selected dispatch facts into
call-lowering inputs.

The durable owner is not generic MIR lowering and not target-independent code.
Any target-specific register, instruction, or call ABI detail must remain in
the AArch64 backend layer that already owns that behavior.

## Scope

- Identify helpers in `calls_dispatch_bridge.*` that only forward, repackage,
  or lightly adapt already-selected dispatch facts.
- Absorb each accepted helper into the durable call/dispatch adapter owner
  while preserving any public entry point still needed by dispatch.
- Keep the implementation bounded to this bridge and the specific owner that
  receives absorbed helpers.
- Preserve existing data flow and error/reporting behavior.

## Behavior-Preservation Proof

Expected proof for a future implementation run:

- build proof for the AArch64 backend target
- focused AArch64 MIR/codegen tests covering:
  - normal calls
  - select-chain call arguments
  - call-result source registers
  - preserved-value materialization
  - local-load fallback call arguments

The proof should compare behavior, not claim progress through expectation
rewrites.

## Reject Signals

Reject the implementation route if it requires any of the following:

- expectation downgrades or unsupported-test conversions
- testcase-shaped shortcuts or named-case matching
- hidden semantic changes to call ABI, argument placement, return handling, or
  preserved-value behavior
- target-specific instruction or register logic moved into generic layers
- `dispatch.cpp` redesign or broad call-family cleanup
- giant multi-purpose refactors
- broad renames without durable concept proof
- reducing file count while increasing responsibility mixing

## Acceptance

- The bridge surface is smaller or clearer because bounded helper glue has a
  durable owner.
- No call ABI, dispatch selection, or generated assembly behavior changes.
- The slice is small enough for one focused future run.
