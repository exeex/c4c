# Current Packet

Status: Active
Source Idea Path: ideas/open/820_lir_directscalar_parameter_producer_verifier_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Trace the existing `ull` DirectScalar authority seam

## Just Finished

- Step 1 traced the existing `ull` seam. `init_fn_ctx` in
  `src/codegen/lir/hir_to_lir/hir_to_lir.cpp` (lines 1409-1441) allocates the
  parameter `LirValueId`, publishes it through `FnCtx::param_value_authorities`,
  and appends a `DirectScalar` `native_body_parameter_definitions` row with the
  current-function owner and `llvm_value_ty` type mirror. Declaration-reference
  emission consumes that identity in `expr/coordinator.cpp` (lines 660-663).
- The failing pre-import predicate is
  `verify_native_body_parameter_definitions` in `src/codegen/lir/verify.cpp`
  (lines 2135-2160): a `DirectScalar` row must be in range, be a plain fixed
  scalar in both logical and signature type facts, be non-byval, have matching
  logical/signature facts, exactly mirror the typed signature ref, and retain
  the same current-function owner/value. The focused external case reproduces
  this exact verifier failure before BIR.
- The narrowly scoped Step 2 seam is the DirectScalar admission/publication
  contract between `init_fn_ctx` and that verifier predicate (including focused
  malformed-authority coverage). Do not widen it to generic scalar receipt,
  presentation-text authority, other ABI classes, or Raw-BIR/importer work.

## Suggested Next

- Implement and prove the minimal typed DirectScalar producer/verifier contract
  repair for the existing `ull` shape, with positive and malformed-authority
  coverage only.

## Watchouts

- Stop if repairing `ull` requires broadening to a DirectScalar authority
  family; create no such expansion in this blocker.
- Do not touch Raw-BIR/importer code or use presentation text as authority.
- A later verifier (`LirBinOp.scalar_lhs_parameter_authority`, lines 2681-2721)
  separately requires a selected DirectScalar LHS binding when the emitted
  binary op uses the published parameter value; it is downstream of the current
  native-body-definition failure and must remain limited to the selected row.

## Proof

- Reproduced evidence only (no code change and no canonical log written):
  `ctest --test-dir build --output-on-failure -R '^llvm_gcc_c_torture_src_20041011_1_c$'`
  failed with `LirFunction.native_body_parameter_definitions: requires a native
  direct-pointer or direct-scalar current-function parameter identity and type`.
  This is pre-BIR/import; acceptance proof remains deferred to Step 2/3.
