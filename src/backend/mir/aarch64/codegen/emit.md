# `emit.cpp` extraction

## Purpose and current responsibility

The removed `emit.cpp` was the historical AArch64 backend production surface.
It mixed public entry points, BIR and LIR shape recognition, direct assembly
text rendering, target-triple symbol spelling, and object-assembly handoff in a
single file.

The file was not a clean target API. It was a transition-era emitter that
recognized many narrow module shapes and returned GNU-style AArch64 assembly
when a shape matched. Unsupported shapes failed through `std::invalid_argument`
messages such as "aarch64 backend emitter does not support ...".

## Important API and contract surfaces

Exported surface from `emit.hpp`:

```cpp
std::string emit_direct_bir_module(const c4c::backend::bir::Module& module);
std::string emit_module(const c4c::backend::bir::Module& module);
std::string emit_prepared_lir_module(const c4c::codegen::lir::LirModule& module);
std::string emit_module(const c4c::codegen::lir::LirModule& module);
assembler::AssembleResult assemble_module(
    const c4c::codegen::lir::LirModule& module,
    const std::string& output_path);
```

Top-level behavior:

- `emit_module(const bir::Module&)` derived a `TargetProfile` from
  `module.target_triple` or the host default, then delegated to
  `emit_target_bir_module(...)`.
- `emit_direct_bir_module(...)` tried `render_bir_module_if_supported(...)`
  and otherwise rejected the direct BIR module.
- `emit_prepared_lir_module(...)` validated the LIR module, tried prepared
  slice renderers, tried a constant-fold single-block fallback for minimal
  types, then tried the general LIR renderer.
- `emit_module(const LirModule&)` derived a target profile from
  `module.target_profile.triple` or the host default, then delegated to
  `emit_target_lir_module(...)`.
- `assemble_module(...)` re-entered the target assembly seam through
  `assemble_target_lir_module(...)` and adapted the result to
  `assembler::AssembleResult`.

## Dependency direction and hidden inputs

Direct dependencies were broad:

- backend orchestration and target seams: `backend.hpp`, `generation.hpp`
- BIR and BIR/LIR conversion: `bir.hpp`, `lir_to_bir.hpp`
- LIR IR and call-argument metadata: `codegen/lir/ir.hpp`,
  `codegen/lir/call_args.hpp`
- stack-layout and regalloc helpers:
  `stack_layout/analysis.hpp`, `regalloc_helpers.hpp`,
  `slot_assignment.hpp`
- AArch64 assembler result types through `emit.hpp` and `assembler/mod.hpp`

Important hidden inputs:

- target triple controls Darwin symbol spelling and function prelude directives
- `LirModule::target_profile` and `bir::Module::target_triple` can be empty
  and were normalized at the entry points
- LIR textual type strings drove much of the general renderer: integer widths,
  arrays, structs, `fp128`, `alignstack(...)`, HFA detection, and sret handling
- global names, string-pool labels, extern declarations, stack slots, and
  ad-hoc type declaration strings were parsed locally
- shared BIR parser helpers supplied some direct-call slice views used by the
  minimal BIR emitter

## Responsibility buckets

### 1. Public AArch64 entry shim

The final functions preserved legacy `c4c::backend::aarch64` symbols and
connected direct BIR/LIR callers to common target-level generation helpers.
They also enforced the transition-era rejection policy for unsupported direct
modules.

### 2. Validation and type-gate helpers

Early helpers rejected unsupported LIR op families and detected whether a
module needed "nonminimal" lowering. These checks depended on LIR type strings
and backend helper predicates for nonminimal global, call, return, and
instruction types.

### 3. BIR minimal-slice recognizers

The file recognized many small BIR shapes before emitting assembly:

- immediate, add-immediate, sub-immediate, affine, and cast returns
- conditional scalar returns and conditional affine i8/i32 returns
- countdown loops
- scalar global load and store/reload
- extern scalar global load and extern array load
- several direct-call shapes supplied by shared BIR parse helpers

These recognizers were shape-specific. They inspected exact function counts,
block counts, terminator kinds, parameter positions, global counts, type kinds,
and immediate ranges.

### 4. LIR fallback recognizers

The old surface also recognized direct LIR-only patterns:

- minimal i32 return immediates and subtract-immediates
- local pointer return constants
- local/string/global scalar slices
- switch-return fallbacks
- a transitional constant-folding evaluator for bounded no-call functions

The constant-folding path simulated a limited memory/SSA environment with local
slots, arrays, structs, globals, and strings. It was explicitly described as a
transitional direct-LIR fallback rather than a real backend lowering strategy.

### 5. Minimal assembly text emitters

`emit_minimal_*_asm` helpers emitted raw AArch64 text with manual stack
adjustments, link-register saves, call sequences, global data directives, and
small instruction sequences. They handled Darwin symbol spelling and function
preludes locally.

Representative output families included:

- `mov`/`sub` scalar returns
- direct helper calls and declared extern calls
- call-crossing regalloc demonstrations
- conditional branches and joins
- countdown loops
- global data emission and string-literal bytes

### 6. General LIR renderer

The largest region was `render_general_lir_asm_if_supported(...)` plus its
`gen_*` helpers. This was a handwritten string-driven renderer for a broad but
still incomplete LIR subset. It owned:

- frame sizing and slot maps for params, locals, globals, strings, variadic
  save areas, and aggregate returns
- integer and floating immediate materialization
- typed load/store chunks, aggregate copies, HFA and sret paths
- global and string data directives
- arithmetic, comparisons, casts, loads, stores, calls, GEPs, select,
  memcpy/memset, variadic ops, abs, stack save/restore, and simple aggregate
  value operations
- block labels, branch/conditional branch/switch terminators, phi edge copies,
  and return epilogues

The renderer returned `std::nullopt` for unsupported shapes, so dispatch order
and fallback selection were part of the implicit behavior.

## Dispatch order

Prepared LIR dispatch order was:

1. scalar global store/reload
2. global int-pointer roundtrip
3. string literal char slice
4. local pointer return immediate when nonminimal lowering was not needed
5. minimal LIR immediate return
6. minimal LIR subtract-immediate return
7. single-block constant fold for minimal types
8. general LIR renderer
9. unsupported direct-LIR failure

Direct BIR dispatch order started with shared direct-call slice views, then
global slices, immediate/affine/cast returns, conditional returns, and countdown
loops before failing the direct BIR module.

## Assumptions and invariants

- The emitter assumed AArch64 general-purpose argument/result registers start
  at `w0`/`x0` and used fixed scratch registers throughout.
- Stack adjustments were manually chunked and frame sizes were aligned locally.
- Many immediate paths only accepted values in narrow `mov`, `add`, or `sub`
  encodable ranges.
- Darwin targets require underscore-prefixed symbols and `.p2align`; other
  targets used `.type`.
- The general renderer treated LLVM/LIR type strings as parseable contracts.
  Struct and array support depended on local string parsing rather than a typed
  layout authority.
- Some paths intentionally bailed out with `std::nullopt` so a later fallback
  could try another route.

## Signature metadata notes

The old fast-path predicates had already moved away from parsing rendered
function signature text. The useful structured-signature evidence to preserve
for reconstruction is:

```cpp
lir_function_is_zero_arg_i32_definition(function)
lir_function_is_i32_definition(function)
lir_function_signature_uses_nonminimal_types(function)
function.signature_return_type_ref.has_value()
function.signature_params.size() == expected_count
function.signature_param_type_refs.size() ==
function.signature_is_variadic
function.signature_has_void_param_list
backend_lir_type_uses_nonminimal_types(type_ref.str())
```

Future AArch64 rebuild work should keep using structured signature fields and
typed aggregate refs instead of recreating rendered-signature parsing.

## Known failure risks and rebuild pressure

- The file concentrated too many responsibilities for reviewable backend work:
  API compatibility, target normalization, shape matching, ABI details, stack
  layout, type parsing, instruction selection, assembly rendering, and assembly
  handoff.
- Many successful paths were testcase-shaped recognizers rather than general
  semantic lowering rules. Rebuilding should avoid extending that pattern.
- Type handling was stringly and locally parsed, especially for aggregates,
  arrays, `fp128`, HFA, sret, and `alignstack(...)`.
- The constant-fold fallback could hide missing lowering capability by
  evaluating bounded functions instead of producing real target lowering.
- Global/string/data handling was interleaved with function rendering instead
  of being owned by a separate module-data surface.
- Error strings were effectively part of the public contract, which makes
  hidden behavior changes hard to detect without focused tests.
- The broad general renderer is the highest-risk source for accidental
  behavior loss during reconstruction, but it should be decomposed into typed
  ABI, frame, data, call, memory, scalar, control-flow, and module seams rather
  than restored as another monolith.

## Rebuild ownership suggestion

A replacement should keep the legacy entry surface thin and route into reviewed
target seams. Recommended extraction targets after this file are the smaller
codegen shards, starting with `src/backend/mir/aarch64/codegen/asm_emitter.cpp`
or `src/backend/mir/aarch64/codegen/prologue.cpp`, because they should help
separate raw assembly text conventions and function-frame policy before any
new AArch64 lowering is rebuilt.
