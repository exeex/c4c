Status: Active
Source Idea Path: ideas/open/542_rv64_object_function_traversal_facade_cleanup.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Map Function Traversal Facade Ownership

# Current Packet

## Just Finished

Lifecycle activation created `plan.md` and this executor-compatible `todo.md` for Step 1.

## Suggested Next

Execute Step 1 from `plan.md`: map the `prepared_function_to_object_function` facade boundary, name the first safe extraction target, record parked object-side dependencies, and record the exact proof command for Step 2.

## Watchouts

- Keep this run behavior-preserving.
- Do not change admission semantics, diagnostics, block traversal order, function name matching, prepared lookup construction, unsupported markers, runtime expectations, or object bytes.
- Keep `fragment_for_prepared_instruction`, final object module assembly, public ELF entrypoints, data object emission, symbol/fixup ownership, relocation mapping, section emission, and module layout parked unless a later plan-owner/reviewer decision explicitly changes scope.
- Do not hide dependency sets behind a new catch-all facade or second all-purpose object emission module.

## Proof

Activation is lifecycle-only; no build or test proof was run.
