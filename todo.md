Status: Active
Source Idea: ideas/open/12_prealloc_cpp_rs_convergence_and_activation.md
Source Plan: plan.md

# Current Packet

## Just Finished

Completed a bounded Step 2 stack-layout contract packet in
`regalloc_helpers.cpp`: copy-coalescing candidates no longer get their
`PreparedStackObject.requires_home_slot` bit reasserted just because the source
slot is `LoweringScratch`. The active slot assigner was already eliding these
objects from dedicated frame-slot allocation by `source_kind`, but prepared
data still advertised the wrong dedicated-home requirement. Focused activation
coverage now proves the candidate keeps `source_kind ==
"copy_coalescing_candidate"` and also reports `requires_home_slot == false`.

## Suggested Next

Continue Step 2 by comparing the remaining active C++ `stack_layout` helper
contracts against the Rust references, with priority on places where prepared
metadata still overstates dedicated-home requirements or misses an active
object-level distinction. Do not pull Tier 2/Tier 3 SSA-value machinery into
the prepared object contract unless the current C++ pipeline first exposes that
state as real prepared data.

## Watchouts

- this slice only repaired the prepared dedicated-home contract for existing
  copy-coalescing candidates; it did not port Rust Tier 2/Tier 3 reuse,
  interval packing, or deferred slot finalization
- copy-coalescing candidates are still identified by provisional local-slot
  access patterns; this packet only made the prepared contract consistent with
  the active dedicated-slot elision
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

Ran `cmake --build --preset default -j4 && ctest --test-dir build -j
--output-on-failure -R '^backend_prepare_stack_layout$' > test_after.log 2>&1`
successfully after tightening the copy-coalescing home-slot contract; canonical
proof log: `test_after.log`. No broader checkpoint was run in this packet.
