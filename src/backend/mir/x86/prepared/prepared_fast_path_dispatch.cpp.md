# `prepared_fast_path_dispatch.cpp`

## Purpose and current responsibility

This file is a branch-dispatch synthesizer for the prepared x86 backend. It does not lower arbitrary MIR instructions. Its current job is narrower and more fragile:

- recognize a small set of compare shapes that can drive direct branch emission
- recover enough prepared-control-flow facts to turn those compares into `compare_setup + false_branch_opcode + target plan`
- bridge between local block evidence and authoritative prepared metadata when the raw block is not sufficient
- render compare setup for named values by consulting prepared value homes

In practice this makes the file the decision point for "can this branch use a prepared fast path?" and the assembly-snippet producer for that decision.

## Important APIs and contract surfaces

Public surface from the directory header:

```cpp
std::optional<std::pair<std::string, std::string>> render_prepared_guard_false_branch_compare(
    const c4c::backend::bir::BinaryInst& compare,
    const std::optional<std::string_view>& current_i8_name,
    const std::optional<MaterializedI32Compare>& current_materialized_compare,
    const std::optional<std::string_view>& current_i32_name,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations);

std::optional<CompareDrivenBranchRenderPlan>
build_prepared_short_circuit_cond_branch_render_plan_if_supported(...);

std::optional<CompareDrivenBranchRenderPlan>
build_prepared_compare_join_fallback_render_plan_if_supported(...);
```

Key output contracts:

- `render_prepared_guard_false_branch_compare(...)` returns only enough text to set flags and the jump mnemonic for the false arm. It does not emit labels or full branch bodies.
- `build_prepared_*_render_plan_if_supported(...)` packages compare rendering with a `ShortCircuitPlan`; callers are expected to emit the final branch sequence elsewhere.
- `select_prepared_i32_named_immediate_compare_*` is the shape filter for the most common supported i32 fast path: named value against immediate.
- `render_prepared_i32_eax_immediate_compare_setup(...)` encodes the local convention that zero compares become `test eax, eax`, otherwise `cmp eax, imm32`.

Shared structs this file depends on:

```cpp
struct MaterializedI32Compare {
  std::string_view i1_name;
  std::optional<std::string_view> i32_name;
  c4c::backend::bir::BinaryOpcode opcode;
  std::string compare_setup;
};

struct CompareDrivenBranchRenderPlan {
  ShortCircuitPlan branch_plan;
  std::string compare_setup;
  std::string false_branch_opcode;
};
```

These structs show the real contract: callers bring current-register/materialized-compare knowledge, and this file either upgrades that into a branch-ready plan or declines.

## Dependency direction and hidden inputs

Dependency direction is inward from prepared metadata and outward to string-based assembly plans:

- consumes BIR blocks, values, and terminators as local evidence
- consumes prepared control-flow facts such as `PreparedBranchCondition`, compare target labels, short-circuit join context, and continuation labels
- consumes prepared value-home lookup to materialize compares for named values
- consumes x86 memory/register helpers from `../lowering/memory_lowering.hpp`
- delegates final short-circuit target assembly to `build_prepared_short_circuit_plan(...)`

Hidden inputs that strongly affect behavior:

- `PreparedX86FunctionDispatchContext` and `PreparedX86BlockDispatchContext` carry many nullable pointers; several paths silently become unsupported when `prepared_names`, `function_locations`, or `function_control_flow` are missing.
- `current_i8_name`, `current_i32_name`, and `current_materialized_compare` act like implicit register-cache state. Support is wider when callers happen to have current values already materialized.
- `find_block` is required to validate continuation targets and reconstruct RHS-entry plans.
- prepared value homes decide whether compare setup comes from a register, stack slot, or rematerialized immediate; this file does not own those placement decisions but is tightly coupled to them.

## Responsibility buckets

### 1. Compare-shape recognition

- extract result names from selected BIR instruction kinds
- detect whether a block defines a named value needed by a join
- admit only a small opcode family: `Eq`, `Ne`, signed ordered comparisons
- admit only selected operand/result type combinations, mostly `I32`, `I8`, and a narrower `I64` path

### 2. False-branch compare rendering

- choose the false-branch jump mnemonic for supported predicates
- build assembly setup for:
  - current `i8` in `al`
  - materialized i32 compare reused from existing state
  - named `i32` vs immediate
  - named `i8` vs immediate
  - named `i64` vs immediate
  - constant-vs-constant i32 compare

This bucket mixes semantic selection with concrete x86 text emission.

### 3. Prepared short-circuit admissibility

- inspect whether a branch/continuation can be rendered through prepared short-circuit plumbing
- confirm that continuation targets can be resolved and that the RHS entry block exposes a usable compare, or that the join has a join-defined named incoming

This is a route validator, not an emitter.

### 4. Branch-plan synthesis

- convert compare context plus prepared short-circuit target information into `CompareDrivenBranchRenderPlan`
- support the direct conditional-branch case
- support a compare-join fallback path that trusts prepared branch-condition metadata over the raw trailing compare when available

### 5. Named-value home materialization

- locate prepared value homes
- choose register narrowing or stack operands
- emit `mov/cmp` or `test` setup in the register convention expected by callers

This is effectively a small operand-lowering island embedded inside dispatch logic.

## Notable fast paths, compatibility paths, and overfit pressure

### Fast paths

- current-register compare reuse for `i8` and `i32`
- zero-immediate i32 compares lowered to `test eax, eax`
- prepared short-circuit branch-plan construction when control-flow metadata and block layout line up
- named-value immediate compares that can be satisfied directly from prepared value homes

### Compatibility paths

- fallback to authoritative `PreparedBranchCondition` instead of trusting the observed block compare
- compare-join fallback that reconstructs branch behavior even when the local block shape is not the preferred direct path
- rematerializable-immediate value homes, which let the file rebuild a compare even when no register or stack home exists

These paths are preserving older or less direct prepared layouts, not expressing a clean core design.

### Overfit pressure

- support is hard-coded around a few exact compare shapes and type combinations rather than a general compare/liveness model
- the file is sensitive to whether the compare sits at the end of the block or can be recovered from continuation metadata
- register assumptions leak into policy: `eax`, `al`, and `rax` are baked into rendering decisions
- the join admissibility logic includes special handling for join-defined named incoming values, which is a concrete compatibility carve-out rather than a general branch abstraction
- string-assembly generation and route selection live in the same file, making it easy to add another narrow special case instead of improving shared compare planning

## Rebuild ownership boundary

In a rebuild, this unit should own:

- recognition of supported compare-driven branch patterns
- translation from prepared control-flow facts into a compact branch-render plan
- explicit declaration of why a fast path is supported or rejected

It should not own:

- ad hoc operand lowering details for register/stack/immediate homes
- scattered register-convention knowledge beyond a small documented interface
- compatibility rescue logic that depends on block-shape accidents
- final assembly-string stitching for whole branch bodies or unrelated block dispatch concerns
