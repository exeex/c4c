# Liveness C++ vs Rust Comparison

This note starts Step 5 acceptance for the active prealloc migration by
comparing the live C++ liveness phase in `liveness.cpp` against the retained
Rust reference in `liveness.rs`.

## Active Match Points

### Dense value set and bitset dataflow

- C++ `BitSet`, `run_backward_dataflow(...)`, and the `ProgramPointState`
  scaffolding keep the same compact bitset plus backward fixed-point shape as
  Rust `BitSet`, `run_backward_dataflow(...)`, and `ProgramPointState`.
- Both paths build a dense value space before dataflow so live-in/live-out work
  is done on compact bitsets instead of sparse hash sets.

### Program-point numbering and CFG-aware interval growth

- C++ `assign_program_points(...)` numbers each instruction and terminator,
  records per-value definition and use points, and collects call points for
  later regalloc consumption.
- Rust `assign_program_points(...)` provides the same core phase ordering:
  program-point numbering first, then live-set propagation, then interval
  construction.
- C++ adds explicit phi-predecessor edge uses through
  `collect_phi_predecessor_uses(...)`, which is now part of the active prepared
  contract and is covered by `backend_prepare_liveness_test.cpp`.

### CFG shape, loop depth, and interval extension

- C++ `build_successors(...)`, `build_predecessors(...)`,
  `compute_loop_depth(...)`, and `extend_intervals_from_liveness(...)` map to
  the same responsibilities as Rust successor construction, loop-depth
  computation, and interval extension from live-in/live-out sets.
- Both routes extend intervals across live-through blocks instead of treating
  local def/use points as the whole live range.

### Published outputs for downstream regalloc

- Rust publishes `intervals`, `call_points`, and `block_loop_depth` through
  `LivenessResult`.
- C++ publishes the same semantic outputs, then projects them into the active
  prepared contract through `PreparedLivenessFunction`,
  `PreparedLivenessBlock`, and `PreparedLivenessValue`.
- The active C++ route is runtime-proven by `backend_prepare_liveness`, which
  checks value-level phi uses, call-crossing detection, loop-depth weighting,
  and regalloc seeding from prepared liveness output.

## Bounded Divergences

### Intentional host-framework adaptations

- C++ works on named BIR values plus prepared stack-layout context instead of
  Rust's raw IR value IDs. This is intentional because the active c4c backend
  publishes prepared records keyed by `PreparedValueId` and may link values
  back to `PreparedStackObject` metadata when that object identity is unique.
- C++ emits richer prepared artifacts than Rust: blocks carry predecessor and
  successor indices, values publish exact `definition_point` / `use_points`,
  and each value records prepared-contract flags such as `address_taken`,
  `requires_home_slot`, and `crosses_call`.

### Still-reference-only Rust behavior

- Rust still has `extend_gep_base_liveness(...)`,
  `extend_f128_source_liveness(...)`, and `extend_intervals_for_setjmp(...)`.
  The active C++ liveness path does not port those extensions yet.
- This omission is currently bounded rather than hidden: the active C++ tests
  prove the core named-value, phi-edge, call-point, and loop-depth route, but
  Step 5 is not complete until the remaining Rust-only extensions are either
  ported or explicitly retired with justification.

## Runtime Proof

- Focused proof command:
  `cmake --build --preset default -j4 && ctest --test-dir build -j --output-on-failure -R '^backend_prepare_liveness$'`
- Current proof surface covers:
  phi predecessor-edge uses, value-level intervals, call-crossing constraints,
  loop-weighted priorities, spill/reload bookkeeping, and move-resolution
  records derived from prepared liveness output.
