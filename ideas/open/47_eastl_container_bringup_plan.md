# EASTL Container Bring-Up Plan

Status: Open
Last Updated: 2026-04-03

## Goal

Shift the current library bring-up focus from direct `std::vector` pressure to a
smaller, staged EASTL-first path:

1. make `tests/cpp/eastl/*` reproducible and actionable
2. get the simpler EASTL cases reliably passing first
3. use EASTL container/header pressure to drive generic parser/sema/HIR fixes
4. expand gradually from `eastl::vector` to other EASTL container-adjacent
   surfaces before returning to broader STL pressure


## Why This Idea Exists

Direct `std::vector` bring-up is still valuable, but it is currently too heavy
as the primary day-to-day frontier:

- system `<vector>` pulls in a very large libstdc++ surface
- success-path throughput is still expensive to inspect
- failures are often mixed with unrelated standard-library complexity

By contrast, the repo already has a curated EASTL test area under
`tests/cpp/eastl/` with smaller, more targeted sources:

- `eastl_type_traits_simple.cpp`
- `eastl_utility_simple.cpp`
- `eastl_tuple_simple.cpp`
- `eastl_memory_simple.cpp`
- `eastl_vector_simple.cpp`

This makes EASTL a better intermediate frontier:

- still exercises real library-grade template code
- smaller and more reducible than system STL
- easier to turn into bounded parser/sema/HIR recipes


## Current EASTL Surface

Current files in `tests/cpp/eastl/`:

- `eastl_integer_sequence_simple.cpp`
- `eastl_memory_simple.cpp`
- `eastl_piecewise_construct_simple.cpp`
- `eastl_tuple_fwd_decls_simple.cpp`
- `eastl_tuple_simple.cpp`
- `eastl_type_traits_simple.cpp`
- `eastl_utility_simple.cpp`
- `eastl_vector_simple.cpp`

Existing helper recipes:

- `run_eastl_type_traits_simple_workflow.cmake`
- `run_eastl_vector_parse_recipe.cmake`

This means the repo already has the beginnings of an EASTL-oriented validation
track; the work now is to make that track the main bring-up path instead of
treating it as a side repro.


## Main Objective

Create a staged EASTL bring-up ladder:

1. non-container support utilities and traits
2. tuple / utility / memory support
3. `eastl::vector` parse-only acceptance
4. `eastl::vector` semantic/lowering viability
5. follow-on EASTL containers using the same infrastructure


## Proposed Bring-Up Order

### 1. Foundation headers and traits

Start with:

- `eastl_piecewise_construct_simple.cpp`
- `eastl_tuple_fwd_decls_simple.cpp`
- `eastl_integer_sequence_simple.cpp`
- `eastl_type_traits_simple.cpp`
- `eastl_utility_simple.cpp`

Why first:

- lower surface area
- high leverage for later containers
- likely to expose generic template/type-trait/parser issues without allocator
  and storage complexity


### 2. Object lifetime / utility layer

Next:

- `eastl_memory_simple.cpp`
- `eastl_tuple_simple.cpp`

Why next:

- introduces placement-new, alignment, tuple/application patterns
- still smaller than full container behavior


### 3. EASTL vector parse frontier

Then:

- `eastl_vector_simple.cpp`

First goal:

- stable, actionable parse-only behavior with bounded recipes and debug support

Second goal:

- progress toward sema/HIR/codegen viability only after parse stability exists


### 4. Expand to other containers

Only after the earlier ladder is healthy, add additional EASTL container tests.

Potential follow-ons:

- fixed-size/vector-like containers
- string-like containers
- associative containers with lighter template surfaces first

Rule:

- grow breadth gradually
- do not jump from one large failing header universe straight to all containers


## Guiding Principle

Do not special-case EASTL text if the underlying bug is generic.

EASTL is the pressure source, not the intended one-off dialect.

Every fix should be classified as one of:

- generic parser improvement
- generic sema improvement
- generic HIR / compile-time lowering improvement
- include-path / recipe / harness improvement

If the change is EASTL-only, justify why that is unavoidable.

Feature work should prefer real capability over workaround behavior.

That means:

- prefer implementing the missing parser/sema/HIR feature correctly
- prefer generic semantic support over testcase-shaped branching
- prefer reusable lowering/model improvements over one-off acceptance hacks

Avoid:

- header-name special cases
- EASTL-only parsing exceptions when the syntax is generic C++
- narrowly targeted hacks that make one testcase pass while leaving the
  underlying mechanism incorrect

Short-term narrowing is fine for debugging, but landed fixes should aim at the
real missing capability unless there is a clearly documented temporary reason
not to.

### Runtime/ABI exception

There is one explicit exception to the "prefer real feature work" rule for the
current bring-up phase:

- default `new` / `delete` / `new[]` / `delete[]`
- libc++abi integration
- libunwind integration
- adjacent runtime / ABI support that the project is not yet ready to own fully
- `virtual` dispatch machinery
- RTTI / `typeid` / dynamic-cast-style runtime type support

For these runtime-facing dependencies, temporary workarounds are acceptable.

Examples:

- testcase-local operator `new[]` shims such as the one in
  `tests/cpp/eastl/eastl_vector_simple.cpp`
- narrowly scoped runtime stubs used only to keep frontend/container bring-up
  moving

The intent is:

- frontend language support should still be implemented correctly
- runtime/ABI integration may be deferred with explicit workarounds until the
  project is ready to own that layer

So the rule becomes:

- parser / sema / HIR / compile-time semantics: prefer the real feature
- runtime / ABI glue not yet in scope: temporary workaround is acceptable
- `virtual` / RTTI support is currently out of scope for this bring-up track and
  should be treated as an explicit exclusion unless a separate idea activates it


## Scheduling And Spin-Out Rules

Bring-up work is explicitly allowed to discover missing prerequisites.

That means:

- it is allowed to create new `ideas/open/*.md` files during bring-up
- it is allowed to reschedule open ideas by renaming them with ordered prefixes
  such as `01_xxx.md`, `02_xxx.md`
- it is allowed to move the active bring-up idea later in the queue when a new
  prerequisite idea must land first

In practice, this means EASTL bring-up is not required to absorb every newly
discovered blocker into one ever-growing plan.

Instead:

1. if the blocker is small and clearly required, keep it in the current bring-up
   step
2. if the blocker is larger, reusable, or belongs to a different subsystem,
   spin it out into a new idea
3. if that new idea must land first, renumber/reorder the open ideas and let
   the bring-up plan move later

This is an intentional policy, not a failure mode.

The goal is to keep bring-up pressure productive without forcing one active plan
to become an unbounded catch-all.


## Immediate Workstreams

### 1. Turn EASTL tests into stable recipes

For each selected EASTL testcase, establish:

- required include flags
- bounded timeout behavior
- whether the current expected state is pass, parse fail, sema fail, or runtime
  fail
- one reproducible command line


### 2. Separate parser blockers from later-stage blockers

For each EASTL testcase:

- first verify `--parse-only`
- then `--dump-canonical` if parse succeeds
- then `--dump-hir-summary` if semantic validation progresses
- only then full compile/runtime workflows

## Execution Notes

- Step 1 and Step 2 of the staged bring-up ladder are complete.
- Step 3 was active when the plan was switched out in favor of the broader
  reference-semantics initiative under
  `ideas/open/46_rvalue_reference_completeness_plan.md`.
- The latest reduced generic blockers cleared from the Step 3 tuple path were:
  `subrange(_Rng&&) -> ...` deduction-guide parsing and
  `operator in_in_result<_IIter1, _IIter2>() &&` template-id conversion
  operator parsing in libstdc++ `ranges_util.h`.
- The next resumable EASTL blocker is the later block-scope
  declaration/expression split now reached around
  `/usr/include/c++/14/bits/ranges_util.h:740` from
  `tests/cpp/eastl/eastl_tuple_simple.cpp`.

This keeps parser debugging from being mixed with later failures.


### 3. Use EASTL to drive focused generic fixes

Likely themes:

- template argument parsing
- dependent qualified names
- member typedef / alias lookup
- reference/cv transformation correctness
- tuple / utility helper lowering
- compile-time trait evaluation


### 4. Keep std::vector as a later validation target

`std::vector` should remain a useful pressure test, but not the primary active
frontier until EASTL coverage is healthier.

Use it as:

- a follow-up benchmark
- a throughput/stress validation case
- a check that generic fixes generalize beyond EASTL


## Non-Goals

- do not reopen closed `std::vector`-specific follow-on plans as the main work
- do not block EASTL progress on solving the whole standard library at once
- do not treat every EASTL failure as a request for a new large refactor
- do not jump to additional containers before the simpler EASTL ladder is
  stable


## Execution Plan

### 1. Establish the EASTL baseline matrix

Record for each file in `tests/cpp/eastl/`:

- parse-only result
- canonical/sema result
- HIR result
- full compile/run result (when practical)


### 2. Pick the simplest failing frontier

Work the smallest EASTL testcase that still exposes the next missing generic
 capability.

Do not default to `eastl_vector_simple.cpp` if a smaller EASTL testcase already
 exposes the same missing mechanism.


### 3. Fix one mechanism family at a time

Examples:

- one template parsing family
- one type-trait / canonicalization family
- one tuple/memory lowering family
- one vector/member access family


### 4. Revalidate the EASTL ladder after each step

After each landed fix:

- rerun the directly affected EASTL testcase
- rerun any earlier simpler cases to prevent regressions
- only then move to the next rung of the ladder


### 5. Expand container breadth only after vector becomes tractable

Once `eastl::vector` is no longer the active frontier:

- add the next simplest container family
- repeat the same staged validation model


## Acceptance Criteria

This idea is successful when:

1. EASTL, not raw `std::vector`, becomes the primary active bring-up track
2. the simpler `tests/cpp/eastl/*.cpp` cases have a clear validation matrix
3. `eastl::vector` has a bounded, reproducible bring-up workflow
4. fixes discovered under EASTL are expressed as generic frontend improvements
5. new container expansion happens incrementally instead of all at once


## Notes To Future Agents

- Start from the smallest EASTL testcase that exposes the next missing
  mechanism.
- Prefer recipe-driven bring-up over one-off shell commands.
- Use parser/sema/HIR stage separation aggressively when triaging failures.
- Treat `std::vector` as a later generalization check, not the first active
  battlefield.
- If bring-up exposes a broader prerequisite, create a new idea and reorder the
  queue instead of silently bloating this plan.
- Bias toward real feature implementation, not workaround acceptance.
- Exception: runtime/ABI glue such as default `new/delete` and libc++abi /
  libunwind integration may use temporary workarounds during this phase.
- Also excluded for this phase: `virtual` and RTTI support.
