# `module_emit.cpp` contract

## Legacy Evidence

The extraction below captures how the old `module_emit.cpp` mixed orchestration,
policy, and many narrow structural lanes in one file.

## Purpose and current responsibility

This file is the top-level x86 module text emitter for a prepared BIR module. It accepts a `PreparedBirModule`, resolves target ABI/profile state, inventories defined functions, constructs data-emission helpers, and returns final assembly text for the whole module.

In practice it owns much more than orchestration:

- module-level function inventory and entry selection
- ABI narrowing for parameter and return registers
- construction of `PreparedX86FunctionDispatchContext`
- direct lowering of a narrow family of trivial scalar-return functions
- fallback dispatch into prepared-control-flow based helper renderers
- multi-defined-function helper/data coordination
- assembly prefix/prologue/epilogue shaping
- contract enforcement via many shape-specific `invalid_argument` errors

The file is therefore both coordinator and policy engine for several unrelated lowering lanes.

## Important API and contract surfaces

Exported surface:

```cpp
[[nodiscard]] std::string emit_prepared_module_text(
    const c4c::backend::prepare::PreparedBirModule& module);
```

Top-level flow:

```cpp
const auto resolved_target_profile = c4c::backend::x86::abi::resolve_target_profile(module);
const auto prepared_arch = resolved_target_profile.arch;
const ModuleAssemblySupport module_assembly_support{ ... };
return module_assembly_support.render_module_text();
```

Internal assembly contract:

- only `X86_64` and `I686` are accepted
- module must contain at least one defined function
- single-function modules are the normal path
- multi-defined modules are accepted only through a bounded same-module helper/dispatch lane
- non-void returns require prepared return ABI metadata
- most successful paths require canonical prepared-module handoff data: prepared names, value homes, move bundles, control-flow metadata, stack layout, and same-module data lookup support

Characteristic failure mode:

- unsupported shapes are rejected by throwing `std::invalid_argument`
- error strings are effectively part of the file's contract language; they describe which prepared handoff or structural family was expected

## Dependency direction and hidden inputs

Direct includes point outward into ABI resolution, prepared-query helpers, lowering helpers, generic x86 codegen helpers, and module data support. Dependency direction is mostly:

`PreparedBirModule` -> ABI/profile resolution -> prepared query lookup -> x86 rendering helpers -> module data finalization

Hidden inputs the entry API does not expose explicitly:

- target triple/profile inferred from the prepared module
- `module.names` for name-to-id and value-home lookup
- `module.stack_layout`
- `module.value_locations`
- `module.control_flow`
- prepared addressing metadata for frame sizing and local-slot addressing
- same-module global inventory and string constant inventory
- helper-prefix and helper-name sets returned by multi-defined dispatch preparation

This means the public function looks simple, but correctness depends on a large, precomputed prepared-module ecosystem.

## Responsibility buckets

### 1. Module inventory and entry selection

`ModuleAssemblyInventorySupport` gathers all non-declaration functions, picks `main` when present, and rejects modules with no defined entry.

### 2. Minimal ABI adaptation

`ModuleMinimalAbiSupport` narrows wide ABI register names to i32-capable names, resolves argument/result registers from prepared ABI metadata, and emits the textual function prefix.

### 3. Prepared-query stitching

`ModuleFunctionPreparedQuerySupport` translates a BIR function into the prepared addressing/value-location/control-flow pointers and packages them into `PreparedX86FunctionDispatchContext`.

Essential context surface:

```cpp
PreparedX86FunctionDispatchContext{
    .prepared_module = &module,
    .function = &function,
    .stack_layout = &module.stack_layout,
    .function_addressing = prepared_queries.function_addressing,
    .function_locations = prepared_queries.function_locations,
    .function_control_flow = prepared_queries.function_control_flow,
    ...
};
```

### 4. Minimal scalar return lowering

`ModuleFunctionReturnSupport` contains the most direct handwritten lowering in the file. It computes frame size, reads prepared value homes and move bundles, and emits a tiny family of i32 return bodies:

- direct parameter passthrough
- one-instruction param/immediate arithmetic or bitwise transforms
- named-return materialization using authoritative return bundles
- some compare-join return arms delegated to prepared helper renderers

This is not generic instruction selection; it is a narrowly curated set of supported shapes.

### 5. Fallback structural dispatch

`ModuleFunctionDispatchFallbackSupport` probes prepared helper renderers in a fixed order:

- local i32 arithmetic guard
- countdown entry routes
- local i16 arithmetic guard
- local slot guard chain
- compare-driven entry

If those fail, it escalates to contract errors keyed to specific prepared-control-flow expectations.

### 6. Multi-defined module compatibility lane

`ModuleMultiDefinedSupport` and `build_multi_defined_dispatch()` coordinate helper prefixes, helper/global usage sets, same-module symbol calls, private data labels, string/global emission, and stricter error behavior when bounded helpers are active.

### 7. Final text assembly

`ModuleRenderSupport` decides among:

- pre-rendered whole-module text from multi-defined dispatch
- selected defined functions plus deferred data finalization
- single-function direct render

## Notable fast paths, compatibility paths, and overfit pressure

### Fast paths

- single-function direct render without deferred module-data bookkeeping
- pre-rendered trivial defined function support for bounded multi-defined modules
- one-block i32 return lanes that bypass broader structural dispatch

### Compatibility paths

- bounded multi-defined-function support with same-module helper prefix insertion
- compare-driven entry rendering through canonical prepared-control-flow handoff
- local-slot and guard-chain helpers that preserve older supported shapes if prepared metadata matches

### Overfit pressure

This file shows strong pressure toward shape-matching rather than durable lowering rules:

- support is encoded as a list of accepted structural families with long explanatory error strings
- the scalar-return lane recognizes a very specific "single param plus immediate op" family
- fallback dispatch order is hardcoded and feature-shaped
- multi-defined handling mixes orchestration with compatibility exceptions
- local-slot errors are rewritten differently when bounded helpers are active

The risk is continued growth by adding another recognized family instead of moving policy into clearer lowering stages with stable ownership.

## Design Contract

Current rebuild role:

- accept a fully prepared module from `api/`
- resolve target/module services
- emit whole-module assembly text through dedicated module and data helpers
- remain the only owner of top-level x86 module text assembly

Owned inputs:

- `PreparedBirModule`
- target profile and target triple resolved through `abi/`
- module data helpers from `module_data_emit.*`

Owned outputs:

- whole-module assembly text

Invariants:

- module emission only accepts x86 target profiles
- function-body lowering is delegated to narrower seams over time
- contract-first stubs are acceptable until behavior-recovery packets land

Must not own:

- deep prepared-query archaeology
- shape-enumeration dispatch catalogs
- assembler or linker orchestration
- hidden ABI policy beyond invoking dedicated helpers

Deferred gaps:

- real function-body lowering and multi-function dispatch recovery remain later
  packets

## Rebuild ownership boundary

This file should own only module-level orchestration in a rebuild:

- accept a prepared module
- resolve target/module services
- invoke dedicated function/module/data emitters
- combine their outputs into final module text

This file should not own in a rebuild:

- handwritten lowering for specific return-expression families
- prepared move/value-home interpretation details
- compare-driven or local-slot structural dispatch policy
- bounded multi-defined compatibility policy internals
- data/helper emission rules beyond invoking a dedicated module-data surface
- the growing catalog of shape-specific contract strings
