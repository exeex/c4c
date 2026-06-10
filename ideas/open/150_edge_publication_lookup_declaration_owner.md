# 150 Edge publication lookup declaration owner

## Goal

Give `make_prepared_edge_publication_lookups` a narrow declaration owner so
AArch64 dispatch producer code no longer needs the broad
`prepared_lookups.hpp` facade just to call that helper.

## Why This Exists

Idea 149 cleaned residual removable `prepared_lookups.hpp` includes from
AArch64 and prealloc consumers. One AArch64 target remained:
`src/backend/mir/aarch64/codegen/dispatch_producers.cpp` still includes the
facade because `make_prepared_edge_publication_lookups` is declared only in
`src/backend/prealloc/prepared_lookups.hpp`.

That remaining include is a declaration-owner problem, not routine include
cleanup. It should be handled as a separate source idea because idea 149 was
explicitly include-only and did not move declarations.

## In Scope

- Identify the narrow owner header that should declare
  `make_prepared_edge_publication_lookups`.
- Move or expose the declaration through that narrow owner without changing the
  helper's behavior.
- Update `dispatch_producers.cpp` to include the narrow owner header instead
  of `prepared_lookups.hpp` if it no longer needs the facade.
- Keep the owning implementation and any remaining true facade users compiling
  with direct, intentional includes.

## Out Of Scope

- Changing edge-publication lookup semantics.
- Renaming the helper as a substitute for moving declaration ownership.
- Removing unrelated `PreparedFunctionLookups`, return-chain, RISC-V, x86, or
  owning implementation facade includes.
- Introducing a broad umbrella compatibility header under a new name.

## Acceptance Criteria

- `make_prepared_edge_publication_lookups` has a narrow declaration owner
  separate from `prepared_lookups.hpp`.
- `dispatch_producers.cpp` no longer includes `prepared_lookups.hpp` solely
  for `make_prepared_edge_publication_lookups`.
- Remaining direct `prepared_lookups.hpp` includes are still justified by
  aggregate facade use, owning implementation, or a separately recorded
  blocker.
- Proof includes `cmake --build --preset default` and
  `ctest --test-dir build -R '^backend_' --output-on-failure`.

## Reviewer Reject Signals

- The patch keeps the helper declared only by `prepared_lookups.hpp` while
  claiming the declaration-owner issue is fixed.
- The patch adds a new broad umbrella header or transitive include path that
  hides the same facade dependency under a different name.
- `dispatch_producers.cpp` drops the facade include only because another
  unrelated header happens to include it transitively.
- The helper is renamed, wrapped, or duplicated without reducing declaration
  ownership pressure.
- Backend lowering behavior, test expectations, or unsupported markers change
  while the route is claimed as declaration ownership cleanup.
- Broad rewrites of prepared lookup construction, return-chain lookup handling,
  RISC-V, or x86 are presented as necessary for this narrow AArch64 blocker.
