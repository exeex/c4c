# X86 Codegen Subsystem Extraction

## Purpose

This document compresses the current `src/backend/mir/x86/codegen/` subsystem
into one review target for Phoenix rebuild work. It treats the live code as
evidence, not as the target design.

## Subsystem Boundary

Owned surface:

- `src/backend/mir/x86/codegen/*.cpp`
- `src/backend/mir/x86/codegen/x86_codegen.hpp`

Important internal split today:

- canonical lowering families: `calls.cpp`, `returns.cpp`, `memory.cpp`,
  `alu.cpp`, `comparison.cpp`, `cast_ops.cpp`, `float_ops.cpp`, `atomics.cpp`,
  `intrinsics.cpp`, `variadic.cpp`, `prologue.cpp`
- top-level entry and module handoff: `emit.cpp`, `mod.cpp`,
  `asm_emitter.cpp`
- prepared-route specialization family:
  `prepared_module_emit.cpp`, `prepared_local_slot_render.cpp`,
  `prepared_param_zero_render.cpp`, `prepared_countdown_render.cpp`
- mixed shared contract surface: `x86_codegen.hpp`

## Entry Points And Handoffs

The public front door already routes canonical emission through the prepared
pipeline instead of keeping prepared logic as a thin optional layer:

```cpp
std::string emit_module(const c4c::backend::bir::Module& module) {
  c4c::TargetProfile target_profile;
  const auto prepared = c4c::backend::prepare::prepare_semantic_bir_module_with_options(
      module, resolve_direct_bir_target_profile(module, target_profile));
  return emit_prepared_module(prepared);
}
```

Observed contract:

- `emit.cpp` turns direct BIR/LIR emission into a prepared-module handoff
- `prepared_module_emit.cpp` becomes the real dispatcher for many current x86
  cases
- canonical non-prepared lowering files still exist, but prepared dispatch now
  owns an increasing share of route choice and rendering

## Shared Contract Surface In `x86_codegen.hpp`

`x86_codegen.hpp` is not a narrow header. It contains shared data structures,
dispatch contexts, helpers, and declarations for both canonical lowering and
prepared-route rendering.

Representative prepared dispatch surface:

```cpp
struct PreparedX86FunctionDispatchContext {
  const c4c::backend::prepare::PreparedBirModule* prepared_module = nullptr;
  const c4c::backend::bir::Module* module = nullptr;
  const c4c::backend::bir::Function* function = nullptr;
  const c4c::backend::prepare::PreparedStackLayout* stack_layout = nullptr;
  const c4c::backend::prepare::PreparedAddressingFunction* function_addressing = nullptr;
  const c4c::backend::prepare::PreparedValueLocationFunction* function_locations = nullptr;
  const c4c::backend::prepare::PreparedControlFlowFunction* function_control_flow = nullptr;
  std::function<const c4c::backend::bir::Block*(std::string_view)> find_block;
  std::function<std::optional<std::string>(const c4c::backend::bir::Param&)>
      minimal_param_register;
};
```

Observed contract pressure:

- the header mixes low-level machine helpers, ABI policies, render helpers,
  prepared dispatch contexts, and large numbers of `render_prepared_*`
  declarations
- prepared dispatch needs broad access to stack layout, addressing, control
  flow, names, globals, and helper-emission callbacks
- this suggests the seam is context-heavy and weakly partitioned

Representative declaration cluster near the bottom of the header:

```cpp
c4c::backend::RegAllocIntegrationResult run_shared_x86_regalloc(
    const c4c::backend::LivenessInput& liveness_input);

std::string emit_prepared_module(
    const c4c::backend::prepare::PreparedBirModule& module);
std::string summarize_prepared_module_routes(
    const c4c::backend::prepare::PreparedBirModule& module,
    std::optional<std::string_view> focus_function = std::nullopt,
    std::optional<std::string_view> focus_block = std::nullopt);
std::string trace_prepared_module_routes(
    const c4c::backend::prepare::PreparedBirModule& module,
    std::optional<std::string_view> focus_function = std::nullopt,
    std::optional<std::string_view> focus_block = std::nullopt);
```

This matters because the same header that exposes shared machine helpers also
publishes prepared-module entrypoints, route tracing, and a very large family
of prepared shape renderers. The contract is not split into "canonical
lowering interface" versus "prepared structural fast-path interface".

## Canonical Lowering Families

Current canonical files look closer to coherent lowering buckets:

- `calls.cpp`: ABI classification, stack-space computation, register argument
  placement, call instruction emission, return-value storage
- `returns.cpp`: epilogue, return register materialization, special handling
  for `i128`, `f32`, `f64`, `f128`
- `memory.cpp`: load/store/GEP/address materialization, direct vs indirect vs
  overaligned slot addressing
- `mod.cpp`: machine-level helpers, register-name conversion, target/ABI
  constants, caller/callee-saved policy, general support utilities

Representative canonical call surface:

```cpp
std::size_t X86Codegen::emit_call_compute_stack_space_impl(
    const std::vector<CallArgClass>& arg_classes,
    const std::vector<IrType>& /*arg_types*/) const {
  std::size_t pushed_bytes = 0;
  for (const auto& klass : arg_classes) {
    if (klass.is_stack()) {
      pushed_bytes += 8;
    }
  }
  return pushed_bytes + ((pushed_bytes % 16 != 0) ? 8 : 0);
}
```

Representative canonical memory surface:

```cpp
void X86Codegen::emit_store_with_const_offset_impl(
    const Operand& val,
    const Value& base,
    std::int64_t offset,
    IrType ty) {
  if (const auto addr = this->state.resolve_slot_addr(base.raw)) {
    switch (addr->kind) {
      case SlotAddr::Kind::OverAligned: /* ... */
      case SlotAddr::Kind::Direct: /* ... */
      case SlotAddr::Kind::Indirect: /* ... */
    }
  }
}
```

What the canonical buckets already own:

- `calls.cpp` centralizes SysV call policy through
  `call_abi_config_impl(...)`, stack-space sizing, argument register moves,
  direct versus indirect call emission, and result storage after the call
- `returns.cpp` owns function epilogue policy and return-register publication,
  including the mixed integer/SSE `i128` cases and x87-backed `f128` return
  handling
- `memory.cpp` owns stack-slot and indirect-memory access policy through
  `resolve_slot_addr(...)`, the `SlotAddr::Kind` split, typed load/store
  helpers, and GEP/address materialization

That ownership split is why prepared files are notable when they redo
addressing, register selection, return shaping, or direct assembly rendering
without flowing through these seams.

## Prepared Route Family

The prepared family currently looks like a parallel subsystem layered beside
the canonical lowering files rather than a thin client of them.

### `prepared_module_emit.cpp`

Observed responsibility:

- prepared-module entry selection
- target validation and symbol rendering policy
- same-module global and string data handling
- multi-function dispatch context construction
- shape-based routing into specialized prepared render helpers
- same-module helper discovery, private data label rendering, and direct data
  section emission

Representative metadata pressure inside the dispatcher:

- target resolution and target-specific symbol naming
- function enumeration and entry selection
- string constant lookup and `.rodata` emission
- same-module global lookup and `.data` / `.bss` emission
- prepared name tables, value-location tables, addressing tables, and
  control-flow tables
- construction of `PreparedX86FunctionDispatchContext`, which is then handed to
  many `render_prepared_*if_supported(...)` helpers

Representative surface:

```cpp
std::string emit_prepared_module(
    const c4c::backend::prepare::PreparedBirModule& module) {
  // validates target, chooses entry, resolves data/symbol rendering,
  // then dispatches into prepared render families
}
```

### `prepared_param_zero_render.cpp`

Observed responsibility:

- compares against zero or immediate
- prepares compare setup from `PreparedValueHome`
- renders branch prefixes and compare-driven return paths
- depends on prepared names, locations, control flow, and specific value-home
  shapes

This is not just rendering syntax; it is doing shape recognition plus value
materialization policy.

Concrete dependency story:

- looks up `PreparedValueHome` records to decide whether a value lives in a
  register, stack slot, or rematerializable immediate
- narrows architectural register names itself before emitting compare setup
- synthesizes compare and branch assembly directly instead of asking a shared
  branch-lowering seam to own the decision

### `prepared_countdown_render.cpp`

Observed responsibility:

- identifies a loop-join countdown control-flow shape
- reconstructs authoritative branch targets through prepared control-flow data
- emits the specialized assembly sequence for that recognized pattern

This is pattern-driven control-flow lowering embedded in a prepared-specialized
file instead of flowing through a shared branch-lowering seam.

Concrete dependency story:

- walks `PreparedControlFlowFunction::join_transfers` and prepared branch
  conditions directly
- resolves authoritative branch targets through prepared labels rather than
  a shared block-lowering abstraction
- rebuilds stack-layout assumptions for the local countdown case before
  rendering assembly text

### `prepared_local_slot_render.cpp`

Observed responsibility:

- by far the largest prepared file and the de facto home for prepared local
  slot, addressing, byval payload, HFA, call-lane, and assorted render helpers
- contains helpers such as
  `resolve_prepared_named_ptr_published_payload_frame_offset_if_supported(...)`
  and
  `append_prepared_small_byval_payload_move_into_register_if_supported(...)`
- repeatedly resolves `PreparedValueHome`, `PreparedAddressingFunction`, stack
  slots, and local layout decisions inside renderer-local logic

This file is currently mixing:

- local-slot addressing resolution
- prepared value-home consumption
- byval payload lane formation
- specialized call lowering
- special-case runtime/copy ordering
- shape-based fast paths

Concrete dependency story:

- computes `PreparedModuleLocalSlotLayout` from stack-layout, addressing, name,
  and value-location metadata
- resolves frame-slot and global-symbol memory accesses directly from prepared
  addressing tables
- contains call-lane helpers that select argument registers and stack offsets
  from prepared value-location data instead of going through `calls.cpp`

## Responsibility Mixing

Current false coupling signals:

- prepared dispatch needs broad knowledge of names, blocks, globals, strings,
  stack layout, value homes, control flow, and symbol emission
- prepared files do not look like clients of `calls.cpp`, `returns.cpp`, or
  `memory.cpp`; they often re-decide addressing or move formation locally
- `x86_codegen.hpp` acts as a shared dumping ground for both subsystem-wide
  helpers and prepared-route-specific contracts

## Dependency Direction

Current likely dependency story:

1. `emit.cpp` normalizes entry into prepared-module handoff
2. `prepared_module_emit.cpp` decides whether a prepared-specialized route can
   render the function/module
3. specialized prepared files consume prepared metadata directly and often
   produce final assembly strings themselves
4. canonical lowering files still define reusable machine/ABI logic, but the
   prepared family does not consistently depend on them as its primary seams

## Prepared Divergence Map

Representative ownership overlap versus the canonical families:

- branch and compare shaping:
  `prepared_param_zero_render.cpp` and `prepared_countdown_render.cpp` decide
  predicates, block targets, and compare setup locally instead of using one
  shared terminator-lowering owner
- call-lane and argument placement:
  `prepared_local_slot_render.cpp` selects ABI registers and stack offsets from
  prepared metadata even though `calls.cpp` already owns canonical call-policy
  machinery
- memory and address formation:
  prepared local-slot helpers reconstruct frame-slot, local-slot, and symbol
  memory operands from prepared metadata instead of treating `memory.cpp` as
  the sole owner
- dispatcher and data emission:
  `prepared_module_emit.cpp` combines route choice, symbol naming, data-section
  emission, and helper-family invocation in one file rather than stopping at a
  narrow dispatch seam

## Special-Case Classification

Provisional classification from the current extraction:

- `core lowering`
  `calls.cpp`, `returns.cpp`, `memory.cpp`, parts of `mod.cpp`
- `dispatch`
  `emit.cpp`, `prepared_module_emit.cpp`, `mod.cpp` dispatcher/helper surfaces
- `optional fast path`
  bounded compare/param-zero/countdown prepared routes when they are truly
  thin adapters over canonical seams
- `legacy compatibility`
  prepared-specialized routes that remain necessary only because canonical
  seams do not yet accept the needed prepared contracts
- `overfit to reject`
  any new prepared matcher that exists only to encode one testcase shape or to
  bypass an already-correct canonical seam

## Proof Surfaces

Current proof surfaces already tied to this subsystem:

- x86 backend c-testsuite focused failures such as `00204.c`
- regenerated assembly under `build/c_testsuite_x86_backend/src/*.s`
- lifecycle ideas 75 to 77 showing repeated downstream collisions around the
  prepared/x86 handoff

## Initial Rebuild Pressure

The subsystem-level rebuild is justified because:

- the prepared family has become a parallel lowering stack
- ownership between dispatch, addressing, call lowering, and special-case
  rendering is not cleanly split
- `x86_codegen.hpp` carries too much mixed contract surface
- structural fixes now require deciding which `.cpp` owns which family instead
  of adding one more local repair in `prepared_local_slot_render.cpp`

## Open Questions For Stage 2 Review

- which parts of `prepared_module_emit.cpp` are real dispatch and which parts
  are accidental rendering ownership
- which prepared routes can be reduced to thin adapters over canonical
  `calls.cpp` / `returns.cpp` / `memory.cpp` seams
- which `x86_codegen.hpp` declarations should become narrower headers or local
  implementation-only contracts
- which prepared-specialized routes still represent durable capability and
  which are only temporary compatibility paths
