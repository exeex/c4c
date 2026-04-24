# BIR Memory Entrypoints And Helper Boundaries

## Intent

Tighten the BIR memory lowering shape after the first memory header/helper and
coordinator split work.

The next step should stay small: keep memory lowering behavior owned by
`BirFunctionLowerer` member entrypoints, and make helper boundaries explicit.
Do not convert memory lowering into broad free functions that take
`BirFunctionLowerer& self`.

## Rationale

`lowering.hpp` is intentionally the full private index for LIR-to-BIR lowering.
That is useful for LLM-agent context: an agent can open one file and see the
lowerer state, entrypoints, and family declarations.

The recent memory work created `memory_types.hpp` and `memory_helpers.hpp`, and
reduced the coordinator, but the boundary is still fuzzy:

- stateful lowering operations should remain `BirFunctionLowerer` members
- pure layout/projection facts should live behind `memory_helpers.hpp`
- file-private glue should stay in anonymous namespaces
- `memory_helpers.hpp` should not behave like a macro-included class fragment

This initiative is about clarifying that boundary, not redesigning memory
semantics.

## Design Direction

Use this rule:

```text
member entrypoint owns stateful lowering flow
free helper owns pure reusable reasoning
anonymous helper owns file-private glue
```

Preferred shape:

```cpp
bool BirFunctionLowerer::lower_memory_gep_inst(...) {
  const auto projection = resolve_aggregate_byte_offset_projection(...);
  ...
}
```

Avoid converting stateful lowering to this style:

```cpp
bool lower_memory_gep_inst(BirFunctionLowerer& self, ...);
```

That form still depends on the whole lowerer state, but hides the ownership
boundary and weakens `lowering.hpp` as the private index.

## Scope

1. Keep these as explicit `BirFunctionLowerer` member entrypoints.
   - `lower_local_memory_alloca_inst`
   - `lower_memory_gep_inst`
   - `lower_memory_load_inst`
   - `lower_memory_store_inst`
   - `lower_memory_memcpy_inst`
   - `lower_memory_memset_inst`
   - any call-memory entrypoint introduced during this work, if needed

2. Make `memory_helpers.hpp` a pure helper declaration surface.
   - It may declare helper result structs.
   - It may declare pure helpers for layout facts, byte-offset projection,
     scalar leaf lookup, scalar subobject reasoning, byte-storage
     reinterpretation, repeated extents, and pointer-array extents.
   - It should not inject class member declarations through preprocessor
     modes.

3. Keep member declarations visible in `lowering.hpp`.
   - If a helper is still a `BirFunctionLowerer` member because it owns
     lowerer behavior or uses lowerer vocabulary tightly, declare it directly
     in `lowering.hpp`.
   - Do not hide member declarations behind `#define` / `#include` fragments.

4. Contain `local_slots.cpp` growth.
   - Do not let `local_slots.cpp` become the new dumping ground.
   - If a clearly non-local-slot family is already isolated enough to move
     without semantic risk, move only that family.
   - Good candidates are memory intrinsic glue or another narrow memory family,
     but only if the move is mechanical and compile-proven.

## Refactoring Steps

1. Remove macro-included member declarations.
   - Replace the `memory_helpers.hpp` member-fragment include pattern with
     explicit declarations in `lowering.hpp`.
   - Keep the same declarations and behavior.

2. Simplify `memory_helpers.hpp`.
   - Keep it as a normal `#pragma once` header.
   - Remove macro mode guards.
   - Ensure it exposes only pure helper structs/functions.

3. Review memory entrypoint list.
   - Ensure the stateful memory flows are visible as members in
     `lowering.hpp`.
   - Keep the number of entrypoints small and semantic.

4. Review helper placement.
   - Promote reusable pure reasoning to `memory_helpers.hpp`.
   - Keep one-off glue local to the `.cpp` that uses it.
   - Do not create new `.hpp` files.

5. Compile and prove no behavior change.
   - Build `c4c_codegen`.
   - Run the relevant BIR/LIR-to-BIR narrow tests.
   - Do not rewrite expectations.

## Acceptance Criteria

- `memory_helpers.hpp` is a normal helper header with no macro-injection mode.
- `lowering.hpp` explicitly lists memory member entrypoints and member helpers.
- Stateful memory lowering remains owned by `BirFunctionLowerer` members.
- Pure helper logic is available through `memory_helpers.hpp`.
- No new `.hpp` files are added.
- No broad `foo(BirFunctionLowerer& self, ...)` rewrite is introduced.
- `local_slots.cpp` does not grow as part of this work; if anything moves, it is
  a narrow mechanical family move.
- `c4c_codegen` builds.
- Relevant BIR/LIR-to-BIR tests pass with no expectation rewrites.

## Non-Goals

- Do not redesign BIR memory semantics.
- Do not introduce `MemoryLoweringState`.
- Do not split every memory `.cpp` into matching headers.
- Do not convert member lowering operations into functional-style free
  functions over the whole lowerer.
- Do not do broad coordinator or call-lowering redesign in this idea.
