Status: Active
Source Idea: ideas/open/12_prealloc_cpp_rs_convergence_and_activation.md
Source Plan: plan.md

# Current Packet

## Just Finished

Threaded lowering-owned helper call-argument ABI through
`src/backend/bir/lir_to_bir_calling.cpp`, so helper-built
runtime/intrinsic calls that bypass the shared direct/indirect call-lowering
handoff now publish explicit `bir::CallInst.arg_abi` metadata for active
scalar and pointer arguments instead of leaving prealloc/regalloc blind to the
call-site destination contract.

Focused `backend_prepare_liveness` coverage now proves two active helper-built
paths in the prepare pipeline: `llvm.fabs.float` preserves float argument ABI
metadata and publishes a concrete `call_arg_stack_to_register` move into
`fa0`, and `llvm.stackrestore` preserves pointer argument ABI metadata and
publishes a concrete `call_arg_register_to_register` move into `a0`.

## Suggested Next

Extend focused activation coverage across the remaining helper-built variadic
paths whose ABI shape is already known at lowering time, especially
`llvm.va_copy.*` and `llvm.va_arg.aggregate`, so pointer and memory/sret-style
helper arguments are proven through the active legalize/liveness/regalloc
route rather than only populated in lowering.

## Watchouts

- exact `definition_point` / `use_points` now capture true local activity,
  while `live_interval` still represents the CFG-extended range after
  live-through propagation; downstream consumers must not collapse those into
  one concept
- loop-depth weighting now depends on mapping liveness use points back through
  `PreparedLivenessBlock` ranges; if program-point numbering changes, regalloc
  weighting and spill-op locations need to stay in lockstep with that contract
- the new spill bookkeeping only records real eviction-to-stack fallout for
  values that actually lose a register and stay stack-backed afterward; reloads
  are now emitted per later use point, but this is still bounded bookkeeping
  rather than true split-interval placement
- `move_resolution` now covers phi joins plus result-producing
  binary/select/cast consumers plus call-site argument/result materialization,
  plus function return-site transfers, but it still does not model other
  non-result consumer sites
- consumer-keyed move records intentionally skip redundant entries when the
  source and produced result already share the same assigned register or stack
  slot; the dedupe key now also includes ABI destination metadata so repeated
  self-keyed ABI moves can stay distinguishable by destination kind/index
- call-argument, call-result, and return-site move records now publish
  destination kind, storage, and concrete register name when the target ABI
  makes that identity available, but ordinary value-destination moves must stay
  free of ABI-specific decoration
- `bir::Function.return_abi` is now the contract that return-site move
  resolution consumes; it is currently synthesized during legalization from the
  legalized function return type when the route has no better source ABI data
- shared direct/indirect call lowering now sets `bir::CallInst.result_abi`
  from `LoweredReturnInfo.abi`, and helper-built scalar/pointer runtime call
  sites in `lir_to_bir_calling.cpp` now also set concrete `arg_abi`/`result_abi`
  where the lowering already knows the active ABI class; variadic aggregate and
  other memory-shaped helper paths still need their own focused proof before
  assuming full helper call-site ABI coverage
- lowered aggregate/sret functions can now legitimately carry
  `return_abi={type=Void, primary_class=Memory, returned_in_memory=true}`;
  prepare legalize must preserve that metadata instead of treating all
  `Void` return ABI entries as empty
- the focused call-result proof currently uses an `F32` value on the active
  RISC-V target because the general-register seed pools are active while float
  register pools are still absent; if float allocation becomes active later,
  this test shape and the return/ABI destination proofs will need a different
  pressure source
- `PreparedAllocationConstraint.preferred_register_names` now carries the
  caller-vs-callee preference split, while `cannot_cross_call` is reserved for
  the stronger call-spanning prohibition; downstream consumers should keep
  those meanings distinct
- the active target pools are still small proof-oriented seeds, even though the
  callee-saved side now has bounded spillover headroom
- non-call spillover must not evict call-crossing callee assignments; the new
  replacement helper intentionally restricts callee spillover eviction to
  other non-call occupants
- Rust Tier 2 / Tier 3 shared-slot reuse is still reference-only for the
  active C++ route: `PreparedFrameSlot` remains a dedicated object-owned slot
  record, so do not fake value-level reuse with object names, source kinds, or
  cross-object slot alias shortcuts before a later packet consumes the new
  value-level contract semantically
- keep `.rs` files as references until the final comparison pass is complete
- acceptance requires both `.cpp` vs `.rs` comparison and runtime proof in c4c
- do not let `liveness` or `regalloc` fall back to object identity for value
  tracking

## Proof

Ran the delegated proof command successfully:
`cmake --build --preset default --target c4c_backend -j4 && ctest --test-dir
build -j --output-on-failure -R ^backend_prepare_liveness$ > test_after.log
2>&1`
after threading helper-built call-argument ABI metadata through
runtime/intrinsic lowering and proving active float and pointer helper-call ABI
move resolution in the prepare/regalloc route.
Canonical proof log: `test_after.log`.
