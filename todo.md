Status: Active
Source Idea: ideas/open/12_prealloc_cpp_rs_convergence_and_activation.md
Source Plan: plan.md

# Current Packet

## Just Finished

Completed the next Step 2 `alloca_coalescing.cpp` packet: rooted multi-block
pointer bookkeeping no longer forces a dedicated home slot when the root local
slot is neither directly accessed nor truly address-exposed. The active
stack-layout route now treats predecessor-attributed rooted phi uses in
multiple blocks the same way it already treated the single-block case, and
focused activation coverage proves a non-escaping multi-block rooted phi path
can keep `address_exposed == false`, `requires_home_slot == false`, and skip
dedicated frame-slot assignment.

## Suggested Next

Continue Step 2 by comparing the remaining conservative mixed-alias stack-layout
boundaries against the Rust references, especially `select` / `phi` paths that
still combine rooted and unrooted pointer inputs and therefore conservatively
force address exposure plus dedicated-home storage.

## Watchouts

- `alloca_coalescing.cpp` now has three meaningful buckets for local-slot
  stack-layout decisions: rooted-pointer bookkeeping (`use_blocks` only), true
  address exposure (`addressed_slots`), and explicit dedicated-home consumers
  (`direct_access_slots`)
- rooted multi-block bookkeeping now stays elidable only while it remains pure
  bookkeeping; any direct slot access, addressed use, `is_address_taken`, or
  lowering-scratch requirement still forces dedicated-home storage
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
after landing the multi-block rooted-phi no-home-slot refinement and focused
backend coverage update. Canonical proof log: `test_after.log`.
