Status: Active
Source Idea: ideas/open/12_prealloc_cpp_rs_convergence_and_activation.md
Source Plan: plan.md

# Current Packet

## Just Finished

Completed the next Step 2 stack-layout activation packet by tightening
rooted-only `bir::PhiInst` operand bookkeeping in
`alloca_coalescing.cpp`. The active `backend_prepare_stack_layout` test now
proves that pure rooted pointer phi merges conservatively mark the root local
slot `address_exposed == true`, `requires_home_slot == true`, and keep
dedicated frame-slot storage, removing the remaining active C++ `phi`
escape-model divergence from the retained Rust reference.

## Suggested Next

Continue Step 2 by comparing rooted-only `bir::BinaryInst` pointer
bookkeeping in `alloca_coalescing.cpp` against `alloca_coalescing.rs`, then
decide whether the active C++ rooted-pointer binary no-home-slot path should
remain a documented bounded divergence or be tightened so pure rooted pointer
binary uses also conservatively force exposure and home-slot storage.

## Watchouts

- `alloca_coalescing.cpp` now has three meaningful buckets for local-slot
  stack-layout decisions: rooted-pointer bookkeeping (`use_blocks` only), true
  address exposure (`addressed_slots`), and explicit dedicated-home consumers
  (`direct_access_slots`)
- rooted multi-block bookkeeping now stays elidable only while it remains pure
  bookkeeping; any direct slot access, addressed use, `is_address_taken`, or
  lowering-scratch requirement still forces dedicated-home storage
- rooted-only pointer `select` compare operands and rooted-only `select`
  true/false value merges are now aligned with the retained Rust escape model
  and both keep dedicated frame-slot storage in the active C++ route
- rooted-only `phi` operand bookkeeping is now aligned with the retained Rust
  escape model and no longer keeps a rooted-phi no-home-slot path in the
  active C++ route
- mixed rooted+unrooted `select` and `phi` paths are now both covered as
  conservative escape boundaries; they still force address exposure plus
  dedicated-home storage
- rooted-only pointer `binary` bookkeeping is still looser than the retained
  Rust model, which marks `BinOp` and `Cmp` operands as escape sites
  generally, so any future loosening needs an explicit reference comparison
  rather than a test-only expectation flip
- direct local-slot loads/stores and `CallInst::sret_storage_name` still keep
  dedicated-home requirements even when they are single-block
- `CallInst::sret_storage_name` remains a bounded host-framework divergence
  from the Rust reference surface and should stay documented as host adaptation
  during the eventual acceptance comparison
- `slot_assignment.cpp` is still a conservative bridge, so Rust-like slot
  tiering and interval sharing are not active yet
- keep `.rs` files as references until the final comparison pass is complete
- acceptance requires both `.cpp` vs `.rs` comparison and runtime proof in c4c
- do not let `liveness` or `regalloc` fall back to object identity for value
  tracking

## Proof

Ran the delegated proof command successfully:
`cmake --build --preset default -j4 && ctest --test-dir build -j
--output-on-failure -R '^backend_prepare_stack_layout$' > test_after.log 2>&1`
after tightening rooted-only `phi` operand escape handling and updating the
focused rooted-only phi activation coverage. Canonical proof log:
`test_after.log`.
