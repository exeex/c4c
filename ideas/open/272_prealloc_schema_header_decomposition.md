# Prealloc Schema Header Decomposition

Status: Open
Created: 2026-05-17

## Intent

Split `src/backend/prealloc/prealloc.hpp` into focused public schema headers
without changing prepared BIR behavior.

The current header has become a monolithic `PreparedBirModule` schema dump
because AArch64 bring-up filled many prepared facts in one place. That was a
reasonable bridge while capability was still being patched in, but the
prepared contract is now large enough that the public schema needs ownership
boundaries.

## Why This Exists

`prealloc.hpp` currently contains names, stack/frame facts, addressing,
liveness, regalloc, value locations, calls, variadic entry plans, i128/f128
carriers, atomics, intrinsics, inline asm, runtime helpers, and control-flow
facts in one translation-facing header.

That hurts readability and compile parallelism:

- every consumer pays for every prepared schema family;
- agents must scan unrelated contracts to edit one family;
- later implementation splits stay tied to one giant include;
- enum-to-string helpers and inline query helpers are scattered through the
  same file as core data definitions.

## In Scope

- Create focused public headers under `src/backend/prealloc/`, using names such
  as:
  - `names.hpp`
  - `module.hpp`
  - `frame.hpp`
  - `addressing.hpp`
  - `liveness.hpp`
  - `regalloc.hpp`
  - `value_locations.hpp`
  - `calls.hpp`
  - `variadic.hpp`
  - `runtime_helpers.hpp`
  - `special_carriers.hpp`
  - `control_flow.hpp`
- Keep `prealloc.hpp` as a compatibility umbrella while the split lands.
- Move schema structs, enums, small constexpr name helpers, and inline lookup
  helpers into their natural headers.
- Keep include direction acyclic and lightweight.
- Update implementation files and tests only as needed to compile against the
  new header layout.
- Preserve all public type names and behavior.

## Out of Scope

- Changing prepared BIR semantics, field meanings, or output.
- Splitting `prealloc.cpp`, `regalloc.cpp`, or `prepared_printer.cpp` bodies in
  this idea except for include adjustments needed by the header split.
- Renaming public prepared structs for aesthetics.
- Removing the compatibility umbrella before downstream consumers are ready.
- Reworking stack layout or register allocation algorithms.

## Completion Criteria

- `prealloc.hpp` is no longer the only owner of all prepared schema families;
  it mainly includes focused headers and exposes the existing API surface.
- Focused headers exist for the major prepared fact families.
- Existing backend/prealloc tests compile and pass with no behavior changes.
- Include churn is limited to the minimum needed for the schema split.
- The split improves parallel compile potential by allowing family-specific
  implementation files to include narrower headers later.

## Reviewer Reject Signals

Reject the route or slice if it:

- changes prepared BIR output, field semantics, or test expectations while
  claiming a header-only decomposition;
- creates circular includes or makes most focused headers include the entire
  umbrella again;
- renames public prepared contracts in a way that churns downstream code
  without reducing header ownership;
- hides implementation changes inside the schema split;
- deletes the umbrella before consumers are migrated;
- treats this as permission to redesign regalloc or stack layout behavior.
