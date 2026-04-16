Status: Active
Source Idea: ideas/open/12_prealloc_cpp_rs_convergence_and_activation.md
Source Plan: plan.md

# Current Packet

## Just Finished

Completed the next Step 2 stack-layout activation packet by adding focused
coverage for the conservative mixed rooted-plus-unrooted pointer `phi`
boundary. The active `backend_prepare_stack_layout` test now proves that a
rooted pointer alias merged with an external pointer through `bir::PhiInst`
keeps the root local slot `address_exposed == true`, `requires_home_slot ==
true`, and preserves dedicated frame-slot assignment in line with the retained
Rust escape-analysis intent.

## Suggested Next

Continue Step 2 by comparing the remaining conservative mixed-alias
stack-layout boundaries against `src/backend/prealloc/stack_layout/alloca_coalescing.rs`,
especially whether any non-escaping rooted-only `select` paths still retain a
dedicated-home requirement that the active C++ route can now safely elide
without diverging from the retained Rust contract.

## Watchouts

- `alloca_coalescing.cpp` now has three meaningful buckets for local-slot
  stack-layout decisions: rooted-pointer bookkeeping (`use_blocks` only), true
  address exposure (`addressed_slots`), and explicit dedicated-home consumers
  (`direct_access_slots`)
- rooted multi-block bookkeeping now stays elidable only while it remains pure
  bookkeeping; any direct slot access, addressed use, `is_address_taken`, or
  lowering-scratch requirement still forces dedicated-home storage
- mixed rooted+unrooted `select` and `phi` paths are now both covered as
  conservative escape boundaries; they still force address exposure plus
  dedicated-home storage
- the retained Rust `alloca_coalescing.rs` marks `Select` and `Phi` operands
  as escape sites generally, so any future loosening needs an explicit
  reference comparison rather than a test-only expectation flip
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
after landing the mixed rooted-plus-unrooted `phi` activation coverage update.
Canonical proof log: `test_after.log`.
