# `prepared_param_zero_render.cpp`

## Purpose And Current Responsibility

This file is a specialized x86_64 backend renderer for "compare-driven" entry and branch shapes whose decision is derived from a single `i32` parameter or a prepared compare/short-circuit plan. It does not own generic codegen; it opportunistically recognizes a narrow family of prepared-control-flow patterns and turns them into full assembly snippets.

Its current responsibility is mixed across:

- extracting authoritative compare setup from prepared value-home metadata
- resolving prepared branch and short-circuit plans back to concrete BIR blocks
- rendering full branch functions for param-zero tests and compare-join returns
- reconstructing return values for materialized compare joins, including immediate arithmetic on top of a base value

The file acts as a bridge between prepared-analysis facts and final assembly text. Most helpers return `std::nullopt` when the shape is unsupported, but several paths escalate to `std::invalid_argument` when prepared metadata was expected to be authoritative and is missing or inconsistent.

## Important API And Contract Surfaces

Exported entry points in `x86_codegen.hpp`:

```cpp
std::optional<std::string> render_prepared_param_zero_branch_function(
    std::string_view asm_prefix,
    std::string_view function_name,
    std::string_view false_label,
    const char* false_branch_opcode,
    std::string_view compare_setup,
    std::string_view true_body,
    std::string_view false_body,
    std::string_view trailing_data = {});
```

This is the lowest-level string assembler here: it expects already-rendered compare setup and lane bodies, then emits the false-label split form.

```cpp
std::optional<std::string> render_prepared_compare_driven_entry_if_supported(...);
std::optional<CompareDrivenBranchRenderPlan> build_prepared_short_circuit_entry_render_plan(...);
std::optional<CompareDrivenBranchRenderPlan> build_prepared_plain_cond_entry_render_plan(...);
std::optional<CompareDrivenBranchRenderPlan> build_prepared_compare_join_entry_render_plan(...);
```

These are the shape recognizers. They consume prepared control-flow, optional value-location data, and callbacks for block lookup / lane rendering. They only succeed for narrow prepared branch families.

```cpp
std::optional<std::string> render_compare_driven_branch_plan(...);
std::optional<std::string> render_compare_driven_branch_plan_with_block_renderer(...);
```

These render a previously built branch plan. They assume `ShortCircuitPlan` is already semantically valid and focus on lane stitching, label naming, and continuation suppression.

```cpp
std::optional<std::string> render_prepared_materialized_compare_join_return_if_supported(...);
std::optional<std::string> render_prepared_supported_immediate_binary(...);
std::string render_prepared_return_body(...);
```

These form the return-value side of the feature: select a computed base, optionally apply immediate arithmetic/bitwise ops, then append `ret`.

## Dependency Direction And Hidden Inputs

Primary dependency direction is one-way into prepared-analysis products:

- `PreparedBirModule` provides names, stack layout, underlying module, and function-local prepared value locations
- `PreparedControlFlowFunction` provides authoritative branch conditions, target labels, join contracts, and short-circuit plans
- BIR `Function`, `Block`, `Param`, and `Global` objects provide the source graph and payloads to map back to
- renderer callbacks provide the final policy for return bodies, global-data emission, asm symbol naming, and block lookup

Hidden inputs that materially control behavior:

- x86_64-only gating; most public entry points immediately reject other architectures
- a single-parameter, `i32`, non-varargs/non-sret/non-byval function shape for param-zero entry fast paths
- `minimal_param_register` as fallback ABI knowledge when prepared value-home data is absent or not reused directly
- `module.stack_layout.frame_size_bytes` and prepared stack offsets for temporary `sub rsp` / `add rsp` sequences
- exact block-label naming contracts through `prepared_block_label(...)`
- authority assumptions: several helpers throw instead of returning `nullopt` once the prepared pipeline claims ownership of branch/join lowering facts

## Responsibility Buckets

### 1. Compare Setup Recovery

`find_prepared_param_zero_compare_setup(...)` translates the incoming parameter home into a `test` sequence.

- register home: narrow to `eax`-style register and emit `test reg, reg`
- stack-slot home: temporarily allocate frame space, load via prepared stack operand, then `test eax, eax`
- rematerializable immediate: materialize into `eax`, then `test eax, eax`

This is a low-level ABI and storage-policy concern embedded inside a control-flow recognizer.

### 2. Prepared Branch / Short-Circuit Plan Recovery

Several helpers recover branch intent from prepared metadata:

- authoritative branch condition lookup
- immediate-branch-condition lookup for named `i32` values
- direct-target resolution from prepared block labels
- short-circuit join and continuation reconstruction

This is the semantic matching layer. It decides whether a source block can be treated as a prepared compare-driven branch at all.

### 3. Entry-Shape Recognition

The file supports multiple overlapping entry families:

- minimal param-zero branch returning directly from the true/false arms
- materialized compare-join entry with authoritative return-arm contracts
- prepared short-circuit entry plans
- prepared plain conditional entry plans
- prepared compare-join continuation plans

Each path tries a prepared-authoritative route first, then may fall back to trailing compare inspection when allowed.

### 4. Branch Assembly Stitching

`render_prepared_param_zero_branch_function(...)` and `render_compare_driven_branch_plan(...)` assemble the final lane layout:

- emit compare setup
- branch to false lane with the selected opcode
- inline true lane first
- render false label and false lane body if not elided
- preserve optional continuation structure for short-circuit plans

### 5. Materialized Compare-Join Return Reconstruction

Return rendering is effectively a small expression evaluator over prepared contracts:

- choose a base value from immediate, parameter, global load, or pointer-backed global load
- recover authoritative return register and parameter home from before-return move bundles when the selected base is the parameter
- apply a limited set of immediate binary ops
- finish with `ret`

## Fast Paths, Compatibility Paths, And Overfit Pressure

### Fast Paths

- single `i32` parameter entry blocks on x86_64
- direct `test` generation from prepared register/stack/immediate homes
- direct true/false return-arm rendering without a generic CFG walk
- short-circuit lane omission when prepared continuations prove one lane is already rendered elsewhere

### Compatibility Paths

- fallback from prepared compare context to trailing guard-compare inspection
- fallback from authoritative param home to `minimal_param_register(...)` for some return-value cases
- support for both `I1` and `I32` compare results in trailing-compare detection

These keep old or partially prepared shapes working, but they also entangle detection logic with multiple levels of authority.

### Overfit Pressure

This file is under strong shape-specific pressure:

- it recognizes exact entry layouts such as "one param, one compare-like decision, conditional branch"
- it encodes a narrow opcode whitelist for guard compares
- it mixes prepared-authoritative contracts with local pattern matching against the trailing instruction in a block
- many error messages mention "canonical prepared-module handoff", which shows the file is compensating for missing clearer ownership boundaries upstream

The risk in further local edits is extending named-shape handling instead of moving responsibility into a cleaner prepared-lowering contract.

## Rebuild Ownership Boundary

This file should own, at most, final x86 text rendering for already-resolved compare-driven branch plans and return arms.

This file should not own:

- semantic recognition of many distinct prepared CFG shapes
- recovery of authoritative branch/join contracts from prepared analysis structures
- storage-policy decisions for parameter homes beyond consuming a normalized operand/register form
- ad hoc fallback matching against source-block trailing compares when a prepared contract should already exist
