# `prepared_local_slot_render.cpp`

## Purpose and current responsibility

This file is the x86 prepared-route fallback renderer for functions that cannot be emitted by the tiny direct-return routes but still fit a bounded set of prepared BIR shapes. In practice it is not just "local slot render"; it is the structural dispatcher for:

- local-slot stack/frame memory access rendering
- straight-line scalar, pointer, float, and selected `f128` instruction lowering
- block and branch rendering for prepared control-flow handoff
- single-block return fast paths
- bounded same-module helper and multi-defined-function rendering

The file treats prepared metadata as authoritative. When local structural rendering reaches a shape that should have been described by canonical prepared control-flow, addressing, or return metadata, it throws contract errors instead of silently guessing.

## Important APIs and contract surfaces

Primary externally-used surfaces are declared in `prepared/prepared_query_context.hpp` and implemented here:

```cpp
std::optional<std::string> render_prepared_local_slot_guard_chain_if_supported(
    const PreparedX86FunctionDispatchContext& context);

std::optional<std::string> render_prepared_single_block_return_dispatch_if_supported(
    const PreparedX86FunctionDispatchContext& context);

std::optional<std::string> render_prepared_trivial_defined_function_if_supported(
    const c4c::backend::bir::Function& candidate,
    c4c::TargetArch prepared_arch,
    ...);

PreparedModuleMultiDefinedDispatchState build_prepared_module_multi_defined_dispatch_state(
    const c4c::backend::prepare::PreparedBirModule& module,
    const std::vector<const c4c::backend::bir::Function*>& defined_functions,
    ...);
```

Context contract is large and mostly hidden behind `PreparedX86FunctionDispatchContext`:

```cpp
struct PreparedX86FunctionDispatchContext {
  const c4c::backend::prepare::PreparedBirModule* prepared_module;
  const c4c::backend::bir::Module* module;
  const c4c::backend::bir::Function* function;
  const c4c::backend::bir::Block* entry;
  const c4c::backend::prepare::PreparedStackLayout* stack_layout;
  const c4c::backend::prepare::PreparedAddressingFunction* function_addressing;
  const c4c::backend::prepare::PreparedNameTables* prepared_names;
  const c4c::backend::prepare::PreparedValueLocationFunction* function_locations;
  const c4c::backend::prepare::PreparedControlFlowFunction* function_control_flow;
  std::function<...> find_block;
  std::function<...> find_string_constant;
  std::function<...> find_same_module_global;
  std::function<...> emit_string_constant_data;
  std::function<...> emit_same_module_global_data;
  ...
};
```

Observed contract rules:

- Only `TargetArch::X86_64` is supported by the real routes.
- Prepared names, value homes, addressing, and sometimes control-flow must line up with the BIR function body.
- Many helpers return `std::nullopt` for unsupported shapes; some paths escalate to `std::invalid_argument` when canonical prepared handoff is required.
- Callers are expected to provide symbol/data emission hooks, block lookup, ABI register selection, and same-module-global capability queries.

## Dependency direction and hidden inputs

Direct includes show the file leaning on broader codegen infrastructure rather than owning it:

- `x86_codegen.hpp` for shared x86 helpers and declarations
- `prepared/prepared_fast_path_dispatch.hpp` and `prepared/prepared_fast_path_operands.hpp` for prepared-route utility surfaces
- `lowering/frame_lowering.hpp` and `lowering/memory_lowering.hpp` for frame and memory operand lowering

Hidden inputs are heavier than the filename suggests:

- prepared-module name tables map user-visible labels and names to canonical ids
- prepared value-location tables decide whether a value lives in a register or stack slot
- prepared addressing tables decide whether a local/global access can use authoritative memory operands
- prepared control-flow tables decide whether branch targets, joins, and short-circuit continuations are authoritative
- module-level callbacks decide symbol spelling, private-data labels, string/global data emission, and same-module-global legality
- ABI knowledge enters through minimal return-register and parameter-register selectors

Dependency direction is inward: this file consumes prepared analyses and emits assembly text. It should not be the place that discovers or repairs prepared facts.

## Responsibility buckets

### 1. Memory/render primitives

Small helpers select prepared memory accesses, build stack operands, and resolve whether a local/global/symbol/frame reference can be rendered from authoritative prepared metadata or from a fallback stack-layout calculation.

### 2. Instruction-family renderers

Large groups of `render_*_if_supported` helpers lower bounded shapes:

- float loads, stores, and `FPExt`
- `f128` local copies
- `i32`, `i64`, and pointer binary ops
- casts and selects
- scalar loads/stores to locals and globals
- same-module helper calls, same-module calls, and direct extern calls

Most of these helpers also mutate ad hoc per-block state such as current named `i32`, current pointer in `rax`, current float carrier register, and materialized compare state.

### 3. Block/control-flow renderer

`render_prepared_local_slot_guard_chain_if_supported` is the center of gravity. It:

- builds local-slot layout
- decides callee-saved preservation for same-module call participation
- walks authoritative control-flow to identify cycles/reentry labels
- renders each block instruction-by-instruction through a long ordered cascade of specialized helpers
- renders return, branch, and conditional-branch terminators
- throws when the route reaches a shape that should have been supplied by authoritative prepared local-slot/control-flow handoff

### 4. Single-block return fast-path family

`render_prepared_single_block_return_dispatch_if_supported` chooses among narrower routes before falling back to the general guard-chain renderer:

- direct extern-call return sequence
- minimal local-slot return
- constant-folded single-block return
- local `i16`/`i64` sub-return specialty route
- immediate-or-param return
- final fallback to local-slot guard chain

### 5. Multi-defined-function/module rendering

The tail of the file assembles bounded multi-function modules:

- emit trivial helper functions
- optionally synthesize same-module helper prefixes
- render one bounded multi-defined call lane
- collect referenced strings and globals
- return `PreparedModuleMultiDefinedDispatchState` for module assembly code to consume

This is materially broader than local-slot rendering.

## Notable fast paths, compatibility paths, and overfit pressure

### Fast paths

- trivial defined functions with no locals and immediate/void returns
- single-block immediate or one-parameter derived returns
- minimal local-slot return sequences
- direct extern-call return sequences
- bounded helper-function renderers that bypass the heavier block renderer

### Compatibility paths

- fallback stack-memory rendering when authoritative prepared access is absent but layout math still works
- same-module helper-call and multi-defined-function support layered into the same file as local-slot rendering
- explicit callee-saved preservation planning for same-module call participation
- support for special float and `f128` transfer cases that keep older execution families alive

### Overfit pressure

This file shows sustained pressure from testcase-shaped growth:

- many narrowly named specialty routes (`constant_folded`, `minimal_*`, `local_i16_i64_sub_return`, bounded helper lanes)
- large ordered cascades where new families are added by inserting another `if_supported` probe
- mixed ownership of general structural rendering and highly specific bounded shapes in one translation unit
- reliance on transient state variables (`current_i32_name`, `current_ptr_name`, compare materialization, alias maps) that couple unrelated instruction families

The route is still attempting to guard against silent overreach by returning `nullopt` or throwing authoritative-handoff errors, but the file has become the compatibility umbrella for too many exceptional paths.

## Rebuild ownership boundary

This file should own only a thin prepared-to-asm structural dispatch seam for bounded x86 rendering, with explicit subcomponents for memory operands, per-instruction lowering, block/terminator rendering, and module helper assembly.

This file should not own prepared-analysis interpretation, same-module helper policy, multi-defined module assembly, or an expanding pile of named shape-specific fast paths. Those belong in clearer route-specific modules with smaller contracts.
