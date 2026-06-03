Status: Active
Source Idea Path: ideas/open/101_bir_prealloc_missing_call_abi_fallback_boundary.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Missing ABI Fallback Sites

# Current Packet

## Just Finished

Completed Step 1 inventory of missing call ABI fallback sites for call args,
function params, call results, and function returns.

AST-backed lookup used:

- `c4c-clang-tool-ccdb list-symbols` on `src/backend/prealloc/legalize.cpp`,
  `src/backend/prealloc/call_plans.cpp`,
  `src/backend/prealloc/regalloc/call_moves.cpp`, and
  `src/backend/bir/lir_to_bir/calling.cpp`.
- `c4c-clang-tool-ccdb function-callees` on `legalize_module`,
  `populate_call_plans`, `append_call_arg_move_resolution`, and
  `append_return_move_resolution`.

Fallback inventory and provisional status:

- `CallInst::arg_abi` active reconstruction:
  `src/backend/prealloc/legalize.cpp:91` `infer_call_arg_abi_impl` rebuilds
  scalar/pointer ABI from `TypeKind`; `legalize_module` fills missing call
  entries from `arg_types` at `src/backend/prealloc/legalize.cpp:369`.
  Status: `bir-carrier-required` for ordinary lowered calls, with
  `compat-bootstrap-repair` retained provisionally for direct BIR/test fixtures
  until Step 2 proves the fixture boundary.
- `Function::params[*].abi` active reconstruction:
  `src/backend/prealloc/legalize.cpp:304` fills missing parameter ABI from
  parameter type and reapplies `is_sret`/`is_byval` flags at
  `src/backend/prealloc/legalize.cpp:307` and
  `src/backend/prealloc/legalize.cpp:310`.
  Status: `bir-carrier-required` for ordinary lowered functions;
  `compat-bootstrap-repair` for legacy/direct BIR construction is plausible but
  needs Step 2 contract proof.
- `CallInst::result_abi` active reconstruction:
  `src/backend/prealloc/legalize.cpp:154` `infer_call_result_abi` rebuilds
  scalar return ABI from `return_type`; `legalize_module` fills missing named
  call results at `src/backend/prealloc/legalize.cpp:380`.
  Status: `bir-carrier-required` for ordinary lowered calls; provisional
  `compat-bootstrap-repair` only for direct BIR/test-fixture calls.
- `Function::return_abi` active reconstruction:
  `src/backend/prealloc/legalize.cpp:144` `infer_function_return_abi` delegates
  to scalar result inference; `legalize_module` fills missing function returns
  at `src/backend/prealloc/legalize.cpp:291`.
  Status: `bir-carrier-required` for ordinary lowered functions;
  `compat-bootstrap-repair` for direct BIR/bootstrap functions until Step 2.
- Shared call-argument ABI resolver:
  `src/backend/prealloc/regalloc/call_return_abi.cpp:296`
  `resolve_call_arg_abi` returns `call.arg_abi[arg_index]` when present, but
  falls back to `infer_call_arg_abi(target_profile, call.arg_types[arg_index])`
  at `src/backend/prealloc/regalloc/call_return_abi.cpp:306`.
  Consumers include `call_arg_abi_register_index` at
  `src/backend/prealloc/regalloc/call_return_abi.cpp:309`,
  `call_arg_storage_kind` at
  `src/backend/prealloc/regalloc/call_return_abi.cpp:362`, stack destination
  sizing, and `append_call_arg_move_resolution` in
  `src/backend/prealloc/regalloc/call_moves.cpp:250`.
  Status: `needs-contract-proof`; likely `bir-carrier-required` for ordinary
  calls, while register index, stack offset, byval lane, and storage placement
  decisions remain `prealloc-target-placement-authority` once the carrier exists.
- Call argument plan bank fallback:
  `src/backend/prealloc/call_plans.cpp:2559` chooses
  `register_bank_from_arg_abi(call->arg_abi[arg_index])` when available, else
  `register_bank_from_type(call->arg_types[arg_index])` at
  `src/backend/prealloc/call_plans.cpp:2561`.
  Status: `needs-contract-proof`; the missing-carrier branch should be treated
  as compatibility/bootstrap only, not ordinary ABI authority. The bank choice
  from an existing ABI remains `prealloc-target-placement-authority`.
- Call argument destination planning:
  `src/backend/prealloc/call_plans.cpp:1128`
  `plan_call_argument_destination` gets register index/storage decisions through
  `regalloc_detail::call_arg_abi_register_index`; placement helpers at
  `src/backend/prealloc/call_plans.cpp:1144`,
  `src/backend/prealloc/call_plans.cpp:1162`, and
  `src/backend/prealloc/call_plans.cpp:1180` only use `call.arg_abi` if present.
  Status: `prealloc-target-placement-authority` for placement from an ABI
  carrier; `needs-contract-proof` because its resolver dependency can still
  synthesize missing call arg ABI.
- Local aggregate address publication gate:
  `src/backend/prealloc/call_plans.cpp:1110` checks byval metadata when present
  and falls back to `arg_types`/argument type for pointer eligibility at
  `src/backend/prealloc/call_plans.cpp:1119` through
  `src/backend/prealloc/call_plans.cpp:1125`.
  Status: `no-action` for ABI reconstruction; this is publication eligibility,
  not ABI classification, but it depends on byval metadata when present.
- Call result consumers:
  `src/backend/prealloc/regalloc/call_return_abi.cpp:449`
  `call_result_storage_kind` returns `None` when `result_abi` is absent; it does
  not infer a result ABI. `append_call_result_move_resolution` only derives
  destination register placement when `call->result_abi.has_value()` at
  `src/backend/prealloc/regalloc/call_moves.cpp:372` and
  `src/backend/prealloc/regalloc/call_moves.cpp:403`.
  Status: `ordinary-path-unreachable` if BIR lowering published result ABI;
  `no-action` for type-only reconstruction in these consumers.
- Call result plan bank fallback:
  `src/backend/prealloc/call_plans.cpp:1004` chooses
  `register_bank_from_result_abi(*call.result_abi)` when present, else
  `register_bank_from_type(call.result->type)` at
  `src/backend/prealloc/call_plans.cpp:1006`.
  Status: `needs-contract-proof`; ordinary lowered named results should carry
  `result_abi`, while the type branch may remain as narrow fixture/bootstrap
  display planning if Step 2 keeps it.
- Memory-return planning:
  `src/backend/prealloc/call_plans.cpp:859` requires `call.result_abi` with
  `returned_in_memory` and scans `call.arg_abi` for the sret pointer at
  `src/backend/prealloc/call_plans.cpp:880`.
  Status: `ordinary-path-unreachable` for missing metadata; no fallback
  reconstruction occurs here.
- Function return move fallback:
  `src/backend/prealloc/regalloc/call_return_abi.cpp:463`
  `infer_scalar_function_return_abi` rebuilds scalar return ABI from
  `Function::return_type`; `function_return_storage_kind` falls back to it at
  `src/backend/prealloc/regalloc/call_return_abi.cpp:497`, and
  `append_return_move_resolution` repeats the fallback at
  `src/backend/prealloc/regalloc/call_moves.cpp:469`.
  Status: `bir-carrier-required` for ordinary lowered functions;
  `needs-contract-proof` for whether this scalar fallback remains as
  `compat-bootstrap-repair`.
- Function parameter consumers:
  `src/backend/prealloc/regalloc/value_homes.cpp:33`
  `fixed_formal_abi_register_index` requires `param.abi` and does not infer
  missing parameter ABI; missing ABI skips register index assignment. The
  fallback for parameters is therefore concentrated in
  `legalize_module`, not value-home indexing.
  Status: `no-action` for reconstruction at this consumer;
  `ordinary-path-unreachable` for missing ABI on lowered params.

BIR carrier producer evidence:

- Carrier structs live in `src/backend/bir/bir.hpp`: `Param::abi` at line 496,
  `CallInst::arg_abi` at line 780, `CallInst::result_abi` at line 786, and
  `Function::return_abi` at line 952.
- BIR lowering computes ABI facts through
  `src/backend/bir/lir_to_bir/call_abi.cpp:790` and
  `src/backend/bir/lir_to_bir/call_abi.cpp:796`.
- Ordinary lowered function declarations publish return ABI at
  `src/backend/bir/lir_to_bir/calling.cpp:689`; lowered function params publish
  ABI on scalar, sret, and byval paths in
  `src/backend/bir/lir_to_bir/call_abi.cpp:527`,
  `src/backend/bir/lir_to_bir/call_abi.cpp:619`,
  `src/backend/bir/lir_to_bir/call_abi.cpp:644`,
  `src/backend/bir/lir_to_bir/call_abi.cpp:673`, and
  `src/backend/bir/lir_to_bir/call_abi.cpp:728`.
- Ordinary lowered direct/indirect calls populate `arg_abi` and `result_abi` in
  `src/backend/bir/lir_to_bir/calling.cpp:1362` and
  `src/backend/bir/lir_to_bir/calling.cpp:1376`; indirect scalar/byval paths
  push argument ABI at `src/backend/bir/lir_to_bir/calling.cpp:1265`,
  `src/backend/bir/lir_to_bir/calling.cpp:1289`, and
  `src/backend/bir/lir_to_bir/calling.cpp:1321`.
  Runtime/compatibility placeholders also populate available ABI metadata, for
  example variadic helpers at `src/backend/bir/lir_to_bir/calling.cpp:1423`,
  `src/backend/bir/lir_to_bir/calling.cpp:1462`, and result ABI at
  `src/backend/bir/lir_to_bir/calling.cpp:1484`.

## Suggested Next

Execute Step 2: decide the retained compatibility/bootstrap boundary for
`legalize_module` and the shared regalloc call-argument/function-return fallback
helpers, then identify which missing carriers should become assertions versus
which fixture paths need explicit compatibility naming.

## Watchouts

- The strongest ordinary-path requirements are the four active reconstruction
  sites in `legalize_module` and the shared regalloc helpers that can still
  synthesize call arg or function return ABI.
- Do not collapse target placement into BIR: register index, stack offset,
  byval lane width, destination register names, and move placement remain
  prealloc target-placement authority after ABI carriers exist.
- Direct BIR tests may be the real reason the `legalize_module` fallback exists;
  Step 2 should name those as compatibility/bootstrap if retained.
- `call_result_storage_kind`, memory-return planning, and formal value-home
  indexing already avoid type-only reconstruction when ABI is absent.

## Proof

Delegated proof command:

```sh
git diff --quiet -- src/backend/bir src/backend/prealloc && printf 'analysis-only Step 1 proof: no implementation changes under src/backend/bir or src/backend/prealloc\n' > test_after.log
```

Result: passed. `test_after.log` contains the delegated proof output and
`git diff --quiet -- src/backend/bir src/backend/prealloc` confirmed no
implementation changes under the audited source directories.
