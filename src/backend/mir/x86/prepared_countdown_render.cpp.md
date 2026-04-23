# `prepared_countdown_render.cpp`

## Purpose And Current Responsibility

This file is a shape recognizer plus direct assembly emitter for a very narrow family of prepared countdown loops in the x86 backend. It does not perform general MIR/BIR lowering. Instead, it tries to prove that a function already matches one of two countdown skeletons and, if the proof succeeds, returns fully rendered x86-64 assembly text.

Current ownership is split across two closely related routes:

- loop-carry/join-transfer countdowns that can stay entirely in `eax`
- local-slot countdown loops that spill the counter to stack memory and optionally chain a second segment

If any structural proof fails, the file declines the route with `std::nullopt`. Some mismatches are treated as hard invariants and throw because the prepared-control-flow metadata is considered authoritative.

## Main Surfaces

The directory-level public surface is declared in `x86_codegen.hpp`:

```cpp
std::optional<std::string> render_prepared_loop_join_countdown_if_supported(
    const c4c::backend::bir::Function& function,
    const c4c::backend::bir::Block& entry,
    const c4c::backend::prepare::PreparedNameTables& prepared_names,
    const c4c::backend::prepare::PreparedControlFlowFunction& function_control_flow,
    c4c::TargetArch prepared_arch,
    std::string_view asm_prefix);

std::optional<std::string> render_prepared_local_i32_countdown_loop_if_supported(
    const c4c::backend::bir::Function& function,
    const c4c::backend::bir::Block& entry,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedControlFlowFunction* function_control_flow,
    c4c::TargetArch prepared_arch,
    std::string_view asm_prefix,
    const PreparedModuleLocalSlotLayout& layout);
```

This file also exposes one dispatcher used by the surrounding backend:

```cpp
std::optional<std::string> render_prepared_countdown_entry_routes_if_supported(
    const PreparedX86FunctionDispatchContext& context);
```

Behavior contract:

- return rendered assembly only for x86-64, zero-parameter functions, and a matched countdown shape
- preserve `asm_prefix` and emit a complete function body
- reject unsupported structure with `std::nullopt`
- throw on contradictions between BIR blocks and prepared canonical metadata

## Dependency Direction And Hidden Inputs

Visible inputs:

- `bir::Function` and entry `bir::Block`
- prepared naming/control-flow tables
- `PreparedModuleLocalSlotLayout`
- target architecture and asm prefix

Hidden dependencies that materially control behavior:

- prepared block-label resolution and authoritative branch targets
- prepared join-transfer metadata, especially loop-carry semantics and edge transfers
- stack-layout decisions from `build_prepared_module_local_slot_layout`
- assumed calling convention result register (`eax`)
- implicit expectation that the whole function is essentially consumed by the recognized countdown route

The dependency direction runs from generic prepared-analysis data toward this emitter. This file does not derive its own truth for branches or joins when prepared metadata exists; it treats that metadata as the source of authority.

## Responsibility Buckets

### 1. Structural helpers

Small local helpers perform label lookup, stack operand rendering, and repeated type/name checks. These are support utilities for pattern recognition, not reusable backend abstractions.

### 2. Loop-carry countdown recognition

`render_prepared_loop_join_countdown_if_supported` matches a compact form:

- entry branches into a loop region
- a prepared join transfer carries an `i32` counter
- loop compares the carried value against zero
- body subtracts one
- exit returns either the counter or literal zero

When matched, it emits a register-only loop:

```cpp
mov eax, <init>
test eax, eax
je <exit>
sub eax, 1
jmp <loop>
ret
```

### 3. Entry-route dispatch

`render_prepared_countdown_entry_routes_if_supported` is the policy point:

- try the loop-join fast path first
- otherwise build a local-slot layout
- then try the stack-backed countdown renderer

This hardcodes the route ordering.

### 4. Local-slot countdown recognition and emission

`render_prepared_local_i32_countdown_loop_if_supported` recognizes a wider but still highly specific family:

- init block stores an immediate `i32` into one stack slot
- condition block loads that slot and compares `!= 0`
- body block reloads, subtracts one, and stores back
- return block reloads the same slot and returns it
- optional guard block can branch to an immediate return or to a second countdown segment

It then emits stack-based assembly around a single slot offset:

```cpp
mov DWORD PTR [rsp + off], <init>
mov eax, DWORD PTR [rsp + off]
test eax, eax
jne <body>
ret
```

## Important Contract Surfaces

- Prepared metadata is not advisory here. Several helpers throw if canonical prepared targets or join transfers do not line up with the visible BIR shape.
- The counter must be `i32`.
- The update must be `sub 1`.
- The zero test must be a compare against literal `0`.
- The storage path must use byte offset `0` within the chosen local slot and no address-form locals.
- For the local-slot route, every block in the function must be accounted for by the recognized segments; leftover blocks reject the route.
- For the guarded/two-segment route, the second segment must reuse the same slot and end cleanly without a third continuation.

## Fast Paths, Compatibility Paths, And Overfit Pressure

### Fast paths

- register-only loop-join emission avoids stack layout and stack traffic
- direct dispatch tries this route before any local-slot analysis

### Compatibility paths

- local-slot countdown route accepts a broader CFG shape
- guard/continuation handling allows a two-segment countdown pattern
- some branch-target resolution can fall back to raw terminator labels when prepared branch-condition metadata is absent

### Overfit pressure

This file is under heavy shape-specific pressure:

- it recognizes exact instruction counts and opcode sequences in blocks
- it encodes one semantic family as many local structural predicates
- it mixes proof of legality with literal asm printing
- it assumes countdown-specific storage/update patterns instead of expressing a generic loop-lowering rule
- the authoritative prepared-handoff exceptions are repeated and countdown-branded, which suggests the logic is compensating for brittle cross-layer coupling

The strongest overfit signal is that success depends on matching named block roles and narrow block contents rather than delegating to a reusable loop or recurrence lowering abstraction.

## Rebuild Ownership Boundary

### Should own

- countdown-route selection policy, if countdowns remain a distinct backend optimization
- the minimal countdown-specific facts needed by a future reusable renderer
- explicit documentation of which prepared-control-flow facts are mandatory

### Should not own

- generic block search and value-shape utilities that belong in shared helpers
- canonical truth for branch/join resolution that should live in prepared-analysis helpers
- full asm string construction mixed directly with brittle shape matching
- ad hoc support for additional countdown variants by adding more local predicates to this file
