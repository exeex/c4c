Status: Active
Source Idea: ideas/open/12_prealloc_cpp_rs_convergence_and_activation.md
Source Plan: plan.md

# Current Packet

## Just Finished

Checkpointed the helper-call ABI lane with the deferred broader backend pause
and repaired the stack-layout test compile blocker that surfaced during that
checkpoint. `backend_prepare_stack_layout_test.cpp` now references
`prepare::stack_layout::FunctionInlineAsmSummary` at the active public
namespace boundary again, the focused `backend_prepare_stack_layout` proof
passes, and the broader `^backend_` checkpoint passes on the repaired tree.

## Suggested Next

Choose the next packet from the active prealloc runbook rather than extending
this helper-call ABI lane by inertia. The route review says the plan and source
idea still align, broader proof is now refreshed, and the next bounded packet
should either advance a real convergence gap or start the explicit Step 5
acceptance/comparison work instead of adding more helper-only proofs.

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
  where the lowering already knows the active ABI class; the focused coverage
  now includes helper-built `va_copy` pointer arguments, aggregate
  `va_arg` sret/ap arguments, and helper-built aggregate call byval/sret
  destinations, but future packets should prefer broader checkpointing over
  adding more helper-only shapes
- `BirFunctionLowerer::lower_param_type` now treats pointer/array-shaped
  function params as canonical `Ptr` inputs before the byval aggregate fallback;
  if pointer-param lowering rules change again, helper-built runtime calls will
  regress first
- ABI move-resolution consumers should distinguish redundant same-register
  handoffs from real ABI-surface transfers: identical source/destination
  registers still skip records, but stack-backed ABI destinations and
  non-matching register ABI destinations now legitimately produce move records
- lowered aggregate/sret functions can now legitimately carry
  `return_abi={type=Void, primary_class=Memory, returned_in_memory=true}`;
  prepare legalize must preserve that metadata instead of treating all
  `Void` return ABI entries as empty
- the broader backend pause exposed a build-sensitive namespace contract in
  `backend_prepare_stack_layout_test.cpp`: callers must use
  `prepare::stack_layout::FunctionInlineAsmSummary`, not the old
  `prepare::FunctionInlineAsmSummary` spelling
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

Focused repair proof passed:
`cmake --build --preset default -j4 && ctest --test-dir build -j
--output-on-failure -R '^backend_prepare_stack_layout$' > test_after.log 2>&1`

Broader checkpoint also passed afterward:
`cmake --build --preset default -j4 && ctest --test-dir build -j
--output-on-failure -R '^backend_' > test_after.log 2>&1`

The active canonical proof log is `test_after.log` from the broader backend
checkpoint. The older `test_before.log` came from a narrower
`backend_prepare_liveness` run, so it is not a matching regression-guard pair
for this broader scope and should not be treated as one.
