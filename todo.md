Status: Active
Source Idea: ideas/open/12_prealloc_cpp_rs_convergence_and_activation.md
Source Plan: plan.md

# Current Packet

## Just Finished

Completed the bounded Step 2 `.cpp` vs `.rs` packet for
`stack_layout/slot_assignment`: the active C++ bridge now packs required
home-slot objects by normalized alignment and size before assigning offsets, so
mixed-width local slots no longer keep discovery-order padding that inflated
`PreparedStackLayout.frame_size_bytes`. Added focused stack-layout activation
coverage proving a `4,8,4` local-slot shape now packs to offsets `0,8,12` with
`frame_size_bytes == 16` instead of the previous sequential-layout waste.

## Suggested Next

Continue Step 2 in `stack_layout` by comparing the remaining Rust
`slot_assignment.rs` responsibilities around block-local reuse and deferred
slot/finalization behavior, then take the next bounded packet only if one of
those responsibilities maps to an active current-pipeline prepared-data gap
without pulling in Step 3 liveness or Step 4 regalloc semantics.

## Watchouts

- this slice only improved ordering among already-required home slots; it did
  not port Rust Tier 2/Tier 3 reuse, interval packing, or deferred slot
  finalization
- the new ordering is stable by normalized alignment, then normalized size,
  then `object_id`; later packets should preserve deterministic prepared-data
  output if they extend slot packing further
- current focused proof now covers one real mixed-alignment packing case inside
  the active c4c stack-layout route
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
successfully after the bounded `slot_assignment` packing change; canonical
proof log: `test_after.log`. No broader checkpoint was run in this executor
packet.
