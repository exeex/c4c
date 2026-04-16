Status: Active
Source Idea: ideas/open/12_prealloc_cpp_rs_convergence_and_activation.md
Source Plan: plan.md

# Current Packet

## Just Finished

Completed the widened Step 2 `alloca_coalescing.cpp` packet: rooted pointer
bookkeeping is now distinct from true address exposure, so casts/phi/select/
binary rooted-pointer transforms only add root use-block bookkeeping unless a
later operation actually escapes the pointer. Pointer-based memory addresses,
call/return/store/branch escapes, and mixed rooted+unrooted select/phi paths
still force `address_exposed`. Phi incoming rooted uses now attribute their
use-blocks to predecessor blocks, and focused activation coverage proves a
non-escaping rooted phi path can keep `address_exposed == false`,
`requires_home_slot == false`, and skip dedicated frame-slot assignment. The
cast-only rooted-pointer coverage was updated to match the same active contract.

## Suggested Next

Continue Step 2 by comparing other active C++ stack-layout helper boundaries
against the Rust references, especially places where the current bridge still
uses coarse dedicated-home decisions for multi-block or mixed-alias paths that
could now be expressed more precisely in prepared data without widening into
`liveness` or `regalloc`.

## Watchouts

- `alloca_coalescing.cpp` now has three meaningful buckets for local-slot
  stack-layout decisions: rooted-pointer bookkeeping (`use_blocks` only), true
  address exposure (`addressed_slots`), and explicit dedicated-home consumers
  (`direct_access_slots`)
- the current widening intentionally keeps mixed rooted+unrooted `select` /
  `phi` handling conservative; those paths still force address exposure
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
after landing the rooted-pointer bookkeeping split, phi predecessor attribution,
and focused backend coverage updates. Canonical proof log: `test_after.log`.
