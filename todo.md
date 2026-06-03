Status: Active
Source Idea Path: ideas/open/101_bir_prealloc_missing_call_abi_fallback_boundary.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Decide Required Carriers And Compatibility Paths

# Current Packet

## Just Finished

Completed Step 2 by selecting the missing-ABI fallback boundary for every
audited site. Ordinary lowered BIR must publish ABI carriers for scalar and
aggregate call args, formal params, named call results, and function returns;
prealloc may legalize existing carrier types and keep only explicitly named
direct-BIR/bootstrap repair paths.

Selected contracts:

- `legalize_module` call arg fill:
  `CallInst::arg_abi.size() == arg_types.size()` is `bir-carrier-required` for
  ordinary lowered calls. The `infer_call_arg_abi_impl` loop may remain only as
  `compat-bootstrap-repair: direct-bir-call-arg-abi-repair` for direct BIR
  fixtures or bootstrap modules that construct scalar calls without full
  lowering. Step 3 should add a named helper/guard around this repair and make
  the ordinary-lowered path observable as a required carrier, not silent
  prealloc authority.
- `legalize_module` param fill:
  every ordinary lowered non-void formal must carry `Param::abi`, including
  sret and byval metadata. The existing fill may remain only as
  `compat-bootstrap-repair: direct-bir-param-abi-repair`, preserving the
  existing `is_sret`/`is_byval` compatibility translation for direct BIR
  fixtures. Step 3 should name the compatibility repair and keep it out of the
  ordinary source-lowered proof path.
- `legalize_module` call result fill:
  named ordinary lowered call results must carry `CallInst::result_abi`.
  Type-only inference from `return_type` may remain only as
  `compat-bootstrap-repair: direct-bir-call-result-abi-repair` for direct BIR
  fixtures that model scalar named results. Void/no-result calls are excluded
  from this requirement.
- `legalize_module` function return fill:
  ordinary lowered functions and declarations with non-void returns must carry
  `Function::return_abi`. The scalar fallback may remain only as
  `compat-bootstrap-repair: direct-bir-function-return-abi-repair` for direct
  BIR/bootstrap functions. Step 3 should keep return type legalization, but make
  ABI synthesis explicitly compatibility-scoped.
- Shared call-arg resolver fallback:
  `regalloc_detail::resolve_call_arg_abi` should stop acting as an ordinary
  type-only ABI authority. Step 3 should split the resolver into an
  existing-carrier path plus a named compatibility helper, or otherwise require
  callers that need ordinary placement to fail/skip without `arg_abi`. Register
  index, stack offset, byval lane width, register names, and move placement
  remain `prealloc-target-placement-authority` after the ABI carrier exists.
- Call arg plan bank fallback:
  `PreparedCallArgumentPlan::value_bank` should prefer
  `register_bank_from_arg_abi`. The `register_bank_from_type` branch may remain
  only as `compat-bootstrap-repair: direct-bir-call-arg-bank-display` for
  direct BIR plans that lack ABI metadata; it must not be the ordinary scalar
  call ABI classifier.
- Call result plan bank fallback:
  `PreparedCallResultPlan::value_bank` should prefer
  `register_bank_from_result_abi`. The `register_bank_from_type` branch may
  remain only as `compat-bootstrap-repair: direct-bir-call-result-bank-display`
  for direct BIR/display planning without ABI metadata; result storage and move
  authority still require `result_abi`.
- Function-return scalar fallback:
  `function_return_storage_kind` and `append_return_move_resolution` currently
  infer scalar return ABI when `Function::return_abi` is absent. Step 3 should
  route this through `compat-bootstrap-repair:
  direct-bir-function-return-move-repair` or require an existing carrier for
  ordinary lowered functions. The repair may cover direct BIR scalar return
  fixtures, but ordinary lowered return moves must be proved from
  `Function::return_abi`.
- No-action consumers:
  `call_result_storage_kind`, `append_call_result_move_resolution`, memory
  return planning, formal value-home indexing, and local aggregate address
  publication are retained. They do not synthesize missing result/param ABI
  facts from scalar type alone. Local aggregate address publication remains a
  publication eligibility rule, not an ABI classifier.

Rejected alternatives:

- Do not delete all fallbacks immediately; direct BIR tests and bootstrap
  modules still need narrow compatibility repairs unless Step 4 proves no
  such fixture path exists.
- Do not move register allocation, stack slot, register-name, byval lane, or
  move-placement decisions into BIR.
- Do not keep unnamed type-only inference in shared regalloc helpers, because
  that lets prealloc silently become primary ABI authority.
- Do not add new enums or broad ABI redesign for this slice; the smallest
  acceptable implementation is named helper/guard surfaces around existing
  carriers and compatibility repairs.

Proof expectations for Step 4:

- Args: at least one ordinary lowered call must prove `arg_abi` is complete
  before prealloc planning, including a scalar case and a metadata-sensitive
  pointer/byval or sret-adjacent case if available.
- Params: ordinary lowered function params must prove `Param::abi` is present,
  including retained sret/byval flag propagation where applicable.
- Results: ordinary lowered named call results must prove `result_abi` is
  present and result moves/storage do not use type-only reconstruction.
- Returns: ordinary lowered non-void functions and declarations must prove
  `return_abi` is present and return move planning consumes that carrier rather
  than the scalar fallback.
- Compatibility: any retained direct-BIR/bootstrap fallback should have an
  explicitly named test or assertion surface so the ordinary lowered proof
  cannot pass through the repair branch by accident.

## Suggested Next

Execute Step 3: implement named compatibility/helper boundaries in
`legalize_module`, the shared call-arg resolver, call/result plan bank selection,
and scalar function-return move planning. Preserve target-placement logic, but
make ordinary lowered paths require existing ABI carriers.

## Watchouts

- Keep the compatibility names stable in Step 3 so tests can assert the intended
  branch instead of relying on implicit type inference.
- Missing ABI in ordinary lowered code should be treated as a producer/contract
  failure. Only direct BIR/bootstrap fixture paths should use the retained
  repairs.
- Existing no-action consumers should not be churned unless Step 3 needs a
  small assertion to document that they already require carriers.

## Proof

Delegated proof command:

```sh
git diff --quiet -- src/backend/bir src/backend/prealloc && printf 'analysis-only Step 2 proof: no implementation changes under src/backend/bir or src/backend/prealloc\n' > test_after.log
```

Result: passed. `test_after.log` contains the delegated Step 2 output and
`git diff --quiet -- src/backend/bir src/backend/prealloc` confirmed no
implementation changes under the audited source directories.
