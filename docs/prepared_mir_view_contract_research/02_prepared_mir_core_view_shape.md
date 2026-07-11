# Prepared MIR Core View Shape

This document answers Step 2's core-view question: what the required
`PreparedMirCoreView` should contain, what it should exclude, how MIR should
traverse prepared input, and which invariants must hold before any target
lowering begins.

The shape below is intentionally a first adapter view over today's
`prepare::PreparedBirModule`, not a new permanent ABI around the whole
prepared module. The field inventory comes from
`src/backend/prealloc/module.hpp:32`, and the smallest live dependency set is
the Step 1 evidence in
`01_current_mir_dependencies_on_prepared_bir.md`.

## Proposed C++ Interface Shape

The first cut should live in a MIR-facing namespace, for example
`c4c::backend::mir::prepared`, and should expose read-only handles with stable
lifetimes for one lowering run:

```cpp
class PreparedMirCoreView {
 public:
  [[nodiscard]] const c4c::TargetProfile& target_profile() const;
  [[nodiscard]] std::string_view target_triple() const;

  [[nodiscard]] const bir::NameTables& bir_names() const;
  [[nodiscard]] const prepare::PreparedNameTables& prepared_names() const;

  [[nodiscard]] std::span<const bir::Function> functions() const;
  [[nodiscard]] std::span<const bir::Function> defined_functions() const;
  [[nodiscard]] std::span<const bir::Global> globals() const;
  [[nodiscard]] std::span<const bir::StringConstant> string_constants() const;

  [[nodiscard]] std::optional<FunctionNameId>
  resolve_function_name(std::string_view name) const;
  [[nodiscard]] std::string_view function_name(FunctionNameId id) const;
  [[nodiscard]] const bir::Function* bir_function(FunctionNameId id) const;
  [[nodiscard]] const prepare::PreparedControlFlowFunction*
  control_flow(FunctionNameId id) const;

  [[nodiscard]] std::optional<PreparedMirFunctionView>
  function_view(FunctionNameId id) const;
  [[nodiscard]] std::optional<PreparedMirFunctionView>
  function_view(std::string_view name) const;
};

class PreparedMirFunctionView {
 public:
  [[nodiscard]] FunctionNameId function_name() const;
  [[nodiscard]] std::string_view function_name_text() const;

  [[nodiscard]] const bir::Function& bir_function() const;
  [[nodiscard]] const prepare::PreparedControlFlowFunction& control_flow() const;
  [[nodiscard]] const prepare::PreparedValueLocationFunction& value_locations() const;
  [[nodiscard]] const prepare::PreparedStackLayout& stack_layout() const;
  [[nodiscard]] const prepare::PreparedAddressingFunction& addressing() const;
  [[nodiscard]] const prepare::PreparedFunctionLookups& prepared_lookups() const;

  [[nodiscard]] std::span<const PreparedMirBlockView> blocks() const;
  [[nodiscard]] std::optional<PreparedMirInstructionCursor>
  instruction(std::size_t block_index, std::size_t instruction_index) const;
};

struct PreparedMirInstructionCursor {
  FunctionNameId function_name = kInvalidFunctionName;
  BlockLabelId block_label = kInvalidBlockLabel;
  std::size_t block_index = 0;
  std::size_t instruction_index = 0;
  const bir::Function* function = nullptr;
  const bir::Block* block = nullptr;
  const bir::Inst* instruction = nullptr;
  const prepare::PreparedControlFlowBlock* prepared_block = nullptr;
};
```

The adapter may cache derived spans and `PreparedFunctionLookups`, but those
caches belong to the view. Targets should not rebuild shared lookup indexes by
walking the raw `PreparedBirModule`.

## Required Core Accessors

| Accessor family | Why MIR requires it |
| --- | --- |
| `target_profile()` and `target_triple()` | x86 resolves and gates the target profile before emitting, while data and symbol spelling need a stable target triple. This is MIR target identity, not a target-local convenience. |
| `bir_names()` | Existing data, function, and instruction code still consumes BIR spelling and structured BIR values. The view should expose the name table directly while the migration keeps BIR as the instruction carrier. |
| `prepared_names()` plus `resolve_function_name()` and `function_name()` | Prepared control-flow, value, block, slot, and link ids are durable prepared identities. Current x86 and RV64 paths resolve function names through `prepare::PreparedNameTables` before selecting a function. |
| `defined_functions()` and `bir_function(FunctionNameId)` | x86 currently iterates `module.module.functions` and skips declarations. A core view must expose defined BIR functions without exposing the entire mutable module. |
| `globals()` and `string_constants()` | x86 data emission uses BIR globals and string constants, and RV64/AArch64 object paths need module data identity. This is module-level MIR input, not optional debug state. |
| `control_flow(FunctionNameId)` | MIR branch lowering must consume prepared blocks, branch conditions, join transfers, and parallel-copy bundles from `prepare::PreparedControlFlowFunction` rather than reconstructing CFG obligations from raw BIR shape. |
| `value_locations()` | Targets need prepared value homes, block-entry publications, and value-home lookups to materialize values without guessing freshness or storage from BIR alone. |
| `stack_layout()` | Frame slots, stack objects, frame size, and object ids are required for stack-backed values and local-memory addressing. The layout is shared lowering input even when a target chooses its own final prologue spelling. |
| `addressing()` | Local/global memory access and address materialization depend on prepared addressing facts, including base kind, byte offsets, widths, alignment, and provenance. Targets should not rediscover these from BIR operands. |
| `prepared_lookups()` | AArch64 and RV64 already build `PreparedFunctionLookups` from a prepared function before lowering. The core view should own these derived indexes so every target sees the same call, address, memory, move-bundle, value-home, and edge-publication lookup semantics. |

These are core because MIR cannot lower the current prepared input safely
without them. They also describe the shared old-BIR/new-BIR output contract:
future producers must provide the same identities, traversal bindings, and
lookup answers even if their internal preparation pipeline is different.

## Excluded Current Prepared Fields

The core view should not expose `prepare::PreparedBirModule` as a field or
inheritance base. It also should not include these current fields as core
accessors:

| Current `PreparedBirModule` field | Core decision |
| --- | --- |
| raw `module` | Excluded as raw authority. The core view exposes only BIR names, defined functions, globals, string constants, and per-function BIR handles. |
| `invariants` | Excluded as a raw vector. Successful invariant verification is a precondition, not codegen input. |
| `liveness` | Not part of the smallest MIR input set; keep it producer/verifier-local unless a later feature view proves a target needs it directly. |
| `register_group_overrides` | Not core. Current uses are grouped-authority diagnostics or register-allocation detail. |
| `regalloc` | Optional storage/register-home feature input. Core value locations name homes; full allocation records are target-dependent. |
| `frame_plan` | Optional frame-boundary feature input. Core stack layout is required; callee-save and frame-boundary plans should be requested only by targets that need them. |
| `dynamic_stack_plan` | Optional dynamic-stack feature input, required only when a function contains dynamic allocation or VLA-like stack behavior. |
| `call_plans` | Optional call feature view, required only for call lowering and call-boundary publication. |
| `store_source_publications` and `call_argument_value_publications` | Optional/verifier feature facts, not core traversal state. They should not be silently used as semantic authority outside a named feature view. |
| `variadic_entry_plans` | Optional variadic feature view. |
| `storage_plans` | Optional value-storage feature view. Core lowering should request it explicitly when decoding storage beyond `PreparedValueLocations`. |
| `object_data` | Optional module data/object-emission view. Assembly text paths may use core globals; relocatable object emission must request object data explicitly. |
| `i128_carriers`, `f128_carriers` | Optional special-carrier views. Required only for functions/instructions that use those carrier families. |
| `atomic_operations` | Optional atomic feature view. Required only for atomic instructions on targets that lower them. |
| `intrinsic_carriers` | Optional intrinsic feature view. Required only for intrinsic instructions. |
| `inline_asm_carriers` | Optional inline-asm feature view. Required only for inline asm instructions. |
| `f128_runtime_helpers`, `i128_runtime_helpers` | Optional runtime-helper feature views attached to the special-carrier contracts, not core. |
| `completed_phases` and `notes` | Diagnostic/provenance metadata. They may appear in a diagnostic view, never in core MIR semantic input. |
| private `route_` | Producer provenance. It may be reported diagnostically but must not change target lowering decisions. |

This split leaves the core view narrow enough for a future BIR producer to
implement without cloning every current prealloc artifact.

## Traversal Strategy

The core view should expose both read-only BIR traversal and a prepared
instruction cursor abstraction.

Direct BIR traversal is necessary for the initial migration because current
target code still pattern-matches `bir::Inst` variants and uses BIR
`Function`, `Block`, `Value`, globals, and string constants as the semantic
instruction carrier. Hiding BIR immediately would force an implementation
rewrite, which is out of scope for this research.

The cursor should be the preferred new target-facing entry point. It binds a
BIR instruction to the prepared function id, block label, block index, and
instruction index used by prepared call plans, variadic helper homes, special
carriers, atomics, intrinsics, inline asm, memory accesses, and object
diagnostics. This prevents consumers from mixing a BIR instruction from one
function with prepared facts from another, and it gives optional feature views
a common lookup key.

The view should not expose mutable traversal or whole-module ownership.
Targets may read BIR through the view, but all prepared facts must be reached
through view accessors or named optional feature views.

## Required Invariants

Before any MIR target runs, a `PreparedMirCoreView` must guarantee:

- target identity is resolved: `target_profile().arch` is not unknown, and
  `target_triple()` is the exact triple used for ABI and symbol spelling;
- all exposed references and spans remain valid for the entire lowering call;
- every defined BIR function that is eligible for MIR lowering has a stable
  `FunctionNameId`, and every exposed `FunctionNameId` resolves through
  `prepared_names()`;
- declarations are either omitted from `defined_functions()` or clearly marked
  so targets do not emit bodies for them;
- every `PreparedControlFlowFunction` exposed by `control_flow()` maps to at
  most one defined BIR function with the same durable prepared function id;
- prepared control-flow block indexes, block labels, branch labels,
  join-transfer labels, and parallel-copy bundle labels all resolve inside the
  same function view;
- instruction cursors never cross functions and never pair an instruction
  index with a different prepared block label;
- `value_locations()`, `stack_layout()`, and `addressing()` contain no invalid
  ids for values, slots, objects, blocks, or links referenced by the function
  view;
- `prepared_lookups()` is derived from the same core view inputs and agrees
  with the exposed control-flow, value-location, addressing, and publication
  facts;
- optional feature absence is explicit. A missing feature view means the
  feature is unavailable for lowering unless the target can prove the function
  does not need it;
- diagnostic notes, completed phases, route names, and verifier reports are
  observational only and cannot change emitted instructions through the core
  view.

These invariants make the core view a stable MIR input contract rather than a
renamed `PreparedBirModule`.
