Status: Active
Source Idea: ideas/open/12_prealloc_cpp_rs_convergence_and_activation.md
Source Plan: plan.md

# Current Packet

## Just Finished

Continued Step 2 in `stack_layout` by extending the C++ alloca/use bridge from
raw local-slot pointer names to simple derived pointer aliases in the current
BIR surface. `alloca_coalescing.cpp` now carries local-slot roots through
pointer-producing `PhiInst`, `SelectInst`, and pointer-shaped
`BinaryInst` values, then treats later call/store/terminator consumers of those
aliases as real addressed uses/escapes. `backend_prepare_stack_layout` now
proves this with a focused select-derived call-escape activation case alongside
the earlier dead-slot, copy-coalescing, addressed-local-slot, and direct
call-escaped local-slot coverage.

## Suggested Next

Continue Step 2 in `stack_layout`: keep comparing the C++ alloca/use-block
bridge to `alloca_coalescing.rs` and activate the next bounded gain around
remaining pointer escape sites that still are not modeled in the active BIR
bridge, especially alias-preserving merges or returns that mix local-slot roots
with non-root pointer values and therefore still need an explicit conservative
policy.

## Watchouts

- the active C++ route now elides dedicated frame slots for dead local-slot
  objects and for prepared `copy_coalescing_candidate` lowering-scratch objects,
  while both addressed-local-slot roots and call-escaped local-slot roots now
  correctly stay live and address-exposed
- derived pointer aliases now keep local-slot roots live through the current
  BIR `CastInst` / `PhiInst` / `SelectInst` / pointer-shaped `BinaryInst`
  bridge, but the C++ port still needs a deliberate policy for mixed-root or
  root-plus-nonroot merges beyond simple conservative unioning
- `regalloc_helpers.cpp` still keeps most live local-slot objects conservative,
  so broader Rust-like alloca tiering is not active yet
- keep `.rs` files as references until the final comparison pass is complete
- acceptance requires both `.cpp` vs `.rs` comparison and runtime proof in c4c
- do not let `liveness` or `regalloc` fall back to object identity for value
  tracking

## Proof

Ran `cmake --build --preset default -j4 && ctest --test-dir build -j
--output-on-failure -R '^backend_prepare_stack_layout$' > test_after.log 2>&1`
successfully; canonical proof log: `test_after.log`. Supervisor-side broader
checkpoint `ctest --test-dir build -j --output-on-failure -R
'^backend_prepare_'` also passed. The broader `-L 'backend'` label remains
non-green in this checkout because of existing `c_testsuite_x86_backend_*`
fallback-asm failures outside this packet.
