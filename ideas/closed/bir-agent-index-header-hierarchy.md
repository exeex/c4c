# BIR Agent Index Header Hierarchy

## Closure

Closed after completing the active five-step runbook. The final structure keeps
`src/backend/bir/bir.hpp` as the public BIR index,
`src/backend/bir/lir_to_bir.hpp` as the exported LIR-to-BIR entry API, and
`src/backend/bir/lir_to_bir/lowering.hpp` as the private lowering directory
index. Memory lowering sources live under
`src/backend/bir/lir_to_bir/memory/` without introducing one-off memory
headers; add `memory/memory.hpp` only if that subdomain later gains shared
memory-specific declarations.

Close-time proof used the available full-suite validation log: 3071 tests
passed, 0 failed, 12 disabled. The read-only regression guard passed against
that full-suite proof.

## Intent

Optimize `src/backend/bir` for LLM-agent programming by treating headers as
directory-level index surfaces rather than one-to-one implementation partners.

The goal is to reduce wasted context and file-search churn for agents working
on BIR lowering. A header should help an agent quickly understand the semantic
boundary of a directory and the shared declarations needed inside that
boundary. It should not exist only because a matching `.cpp` exists.

## Rationale

Traditional `src/` plus `include/` layouts and one-`.cpp`/one-`.hpp` pairings
are not the best fit for this codebase's agent workflow. The preprocessor
expands headers as a whole anyway, and LLM agents pay an additional cost when
they must inspect many small headers just to discover that most are thin or
irrelevant.

For this repo, consistent local style and clear directory-level indexes matter
more than enforcing narrow human-facing interface boundaries through many
headers. The important constraint is controlling what an agent needs to read in
one context window to understand or safely edit a semantic area.

## Design Direction

Use this rule:

```text
directory = semantic boundary
.hpp      = index entry for that boundary
```

Header visibility should follow path depth because that is the cheapest and
most natural signal for an LLM agent:

```text
src/backend/bir/*.hpp
  public to src/** callers

src/backend/bir/lir_to_bir/*.hpp
  private to the lir_to_bir implementation area

src/backend/bir/lir_to_bir/memory/*.hpp
  private to the memory-lowering subdomain
```

In other words, a deeply nested header is private by default because an agent
will only find it after entering that semantic area. A first-layer header is
public by default because it is visible from the directory's top-level index.
This makes both search behavior and responsibility boundaries align with the
file tree instead of relying on convention-heavy include discipline.

If an interface truly must be exported across a wider boundary, create or keep
an explicit top-level `.hpp` for that exported surface instead of expecting
agents to discover a nested implementation header.

Avoid creating one header per implementation file. Prefer one meaningful
header per semantic directory:

- `src/backend/bir/bir.hpp`
  - public BIR IR data model and small BIR-level APIs
- `src/backend/bir/lir_to_bir.hpp`
  - exported LIR-to-BIR entry API only
- `src/backend/bir/lir_to_bir/lowering.hpp`
  - private index for the LIR-to-BIR lowering implementation directory
- `src/backend/bir/lir_to_bir/memory/memory.hpp`
  - private index for the memory-lowering subdomain if memory is split into a
    subdirectory

Do not introduce headers such as `addressing.hpp`, `local_slots.hpp`, or
`provenance.hpp` unless a subdomain grows large enough to deserve its own
directory and directory-level index.

## Target Shape

Preferred long-term layout:

```text
src/backend/bir/
  bir.hpp
  bir.cpp
  bir_printer.cpp
  bir_validate.cpp

  lir_to_bir.hpp
  lir_to_bir.cpp

  lir_to_bir/
    lowering.hpp
    analysis.cpp
    aggregate.cpp
    calling.cpp
    cfg.cpp
    context.cpp
    globals.cpp
    module.cpp
    scalar.cpp
    types.cpp

    memory/
      memory.hpp
      coordinator.cpp
      addressing.cpp
      local_slots.cpp
      provenance.cpp
      value_materialization.cpp
```

## Refactoring Steps

1. Keep the current BIR public surface centralized in `bir.hpp`.
   - `bir_printer.hpp` and `bir_validate.hpp` should remain unnecessary once
     their declarations live in `bir.hpp`.
   - Avoid splitting BIR declarations again unless a whole semantic directory
     emerges.

2. Shrink `src/backend/bir/lir_to_bir.hpp` into a public entry header.
   - Keep only public options, notes, result types, and entry functions needed
     by external callers.
   - Move private lowering context, analysis structs, `lir_to_bir_detail`, and
     `BirFunctionLowerer` machinery into `src/backend/bir/lir_to_bir/lowering.hpp`.
   - Update `src/backend/bir/lir_to_bir/*.cpp` to include `lowering.hpp`.

3. Split the memory lowering directory only after the public/private header
   boundary is clean.
   - Move memory-family `.cpp` files under `src/backend/bir/lir_to_bir/memory/`.
   - Rename `memory.cpp` to `memory/coordinator.cpp` if it continues to act as
     the opcode dispatch/coordinator surface.
   - Add one `memory/memory.hpp` only if memory-specific shared declarations
     should be separated from the broader `lowering.hpp`.

4. Preserve source-level behavior.
   - This initiative is structural. It should not change lowering semantics,
     testcase expectations, or BIR output.
   - Any semantic cleanup discovered during the refactor should become a
     separate idea unless it is required to keep the structural change
     compiling.

## Acceptance Criteria

- External users can include `src/backend/bir/lir_to_bir.hpp` without pulling
  in the full private lowering implementation declarations.
- Agents working inside `src/backend/bir/lir_to_bir/` can open
  `lowering.hpp` as the directory index and avoid searching unrelated public
  headers.
- If memory lowering is moved into a subdirectory, memory agents can open one
  `memory.hpp` index instead of guessing among multiple one-off headers.
- The number of thin single-purpose headers in `src/backend/bir` decreases or
  stays flat.
- `c4c_backend` builds after each structural slice.
- Relevant backend tests continue to pass.

## Non-Goals

- Do not introduce a traditional separated `include/` tree for this work.
- Do not enforce one header per `.cpp`.
- Do not split headers merely for C++ interface purity.
- Do not change memory lowering semantics as part of the layout refactor.
