# Stack Layout C++ vs Rust Comparison

This note finishes the per-phase Step 5 comparison coverage for the active
prealloc migration by summarizing the live C++ stack-layout route in
`stack_layout/coordinator.cpp` plus `stack_layout/*.cpp` against the historical
Rust design. The prealloc-local `stack_layout/*.rs` files have been removed;
use `ref/claudes-c-compiler/` only for explicit archaeology.

## Active Match Points

### Phase split and orchestrated helper boundaries

- Rust `calculate_stack_space_common(...)` drives stack-layout work through
  explicit helper phases for analysis, coalescing, inline-asm/regalloc
  integration, and slot assignment.
- C++ `BirPreAlloc::run_stack_layout()` now keeps the same broad phase split:
  `collect_function_stack_objects(...)`, `apply_alloca_coalescing_hints(...)`,
  `apply_copy_coalescing_hints(...)`, `summarize_inline_asm(...)`,
  `apply_regalloc_hints(...)`, and `assign_frame_slots(...)`.
- Both routes treat stack layout as an ordered pipeline instead of a single
  monolithic pass, and the active C++ tests exercise those helpers directly
  as well as through the prepared-module entrypoint.

### Prepared object discovery and explicit home-slot contracts

- The historical analysis and early slot-classification stages distinguish
  address-exposed or parameter-owned storage from values that may be reordered
  or coalesced.
- C++ `analysis.cpp` publishes the same core contract into
  `PreparedStackObject`: byval-copy and phi local slots keep explicit
  permanent-home metadata, address-taken locals stay address-exposed, and
  byval/sret parameters become first-class prepared stack objects.
- `backend_prepare_stack_layout` proves this active path with focused checks
  for analysis-object collection, parameter object publication, regalloc-hint
  preservation, and fixed-tier frame-slot creation for byval/sret and
  addressed locals.

### Copy-coalescing and frame-slot ordering behavior

- The historical copy-coalescing and slot-assignment stages avoid unnecessary
  stack homes for short-lived or aliasable values, then keep fixed-location
  storage ahead of reorderable storage during slot assignment.
- C++ `copy_coalescing.cpp` marks single-store/single-load lowering-scratch
  locals as `copy_coalescing_candidate`, and `slot_assignment.cpp` excludes
  those candidates from dedicated frame-slot allocation while still anchoring
  permanent-home objects in a fixed-location tier ahead of reorderable ones.
- The active C++ route is runtime-proven by `backend_prepare_stack_layout`,
  which checks copy-coalescing activation, fixed-tier ordering, byval/sret
  anchoring, and dedicated storage for explicit permanent-home objects such as
  sret-storage locals.

### Regalloc- and inline-asm-aware stack-object preservation

- The historical design keeps stack layout aware of regalloc and inline asm so
  stack-slot decisions respect register pressure and asm clobber constraints.
- C++ carries the same intent at the prepared-contract level: it summarizes
  inline-asm presence, then `apply_regalloc_hints(...)` preserves or strengthens
  home-slot requirements for address-exposed objects and parameter-owned
  storage before slots are assigned.
- `backend_prepare_stack_layout` already proves that these hints are active by
  checking regalloc-hint preservation for byval-copy, phi, address-taken,
  byval-param, and sret-param objects instead of leaving the metadata path
  unexercised.

## Bounded Divergences

### The active C++ route publishes prepared metadata, not Rust's full codegen-time allocator

- Rust `calculate_stack_space_common(...)` consumes raw IR, regalloc results,
  and optional cached liveness to drive the full three-tier stack-space
  allocator used by backend codegen.
- The current C++ route stops earlier on purpose: it materializes prepared
  stack objects plus frame slots for the shared prealloc contract, but it does
  not yet port Rust's full Tier 2 liveness packing, Tier 3 block-local reuse,
  or backend-state slot propagation machinery one-to-one.
- This difference is visible rather than hidden: the active C++ tests prove
  prepared object and frame-slot behavior, not a full backend codegen-time
  stack-space allocator replacement.

### Alloca escape analysis and dead-param elision remain unimplemented locally

- The historical alloca-coalescing route performs real alloca escape analysis,
  tracks GEP-derived aliases, and can classify dead or single-block allocas for
  stronger slot reuse decisions.
- The historical analysis route also removes dead parameter allocas when
  regalloc proves the ABI value can live safely in a callee-saved register.
- C++ currently keeps aggregate byval/sret parameter objects materialized and
  uses lighter-weight metadata hints instead of porting the full alloca and
  dead-param analysis. The comment in `analysis.cpp` makes that limitation
  explicit because the current prepared contract does not yet expose the Rust
  inputs needed for a one-to-one port.

### Inline-asm and regalloc helper scope is deliberately narrower in C++

- The historical inline-asm route scans explicit asm constraints and clobbers
  to mark callee-saved-register pressure precisely, and its regalloc-helper
  route filters available registers plus merges asm clobbers into backend
  regalloc state.
- C++ currently reduces that scope to a prepared `FunctionInlineAsmSummary`
  and a home-slot-preservation pass in `apply_regalloc_hints(...)`; it does
  not yet port Rust's explicit callee-saved clobber accounting or register-pool
  filtering inside stack layout.
- That gap is currently bounded: the active C++ path still reacts to address
  exposure and inline-asm side effects when deciding permanent-home metadata,
  but Step 5 acceptance should not claim one-to-one parity with Rust's
  register-clobber-aware helper surface yet.

## Runtime Proof

- Focused proof command:
  `cmake --build --preset default -j4 && ctest --test-dir build -j --output-on-failure -R '^backend_prepare_stack_layout$'`
- Current proof surface covers:
  prepared stack-object collection, byval/sret parameter publication,
  copy-coalescing candidates, regalloc-hint preservation, fixed-location
  frame-slot ordering, and dedicated storage for explicit permanent-home
  objects including sret-storage locals.
