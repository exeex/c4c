Status: Active
Source Idea Path: ideas/open/102_aapcs64_va_arg_payload_shape_authority.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Existing Va Arg Shape Authority

# Current Packet

## Just Finished

Completed `plan.md` Step 1: inventoried existing scalar, aggregate, and HFA
AAPCS64 `va_arg` payload-shape authority without implementation changes.

### Step 1 Inventory

#### Producer Surfaces

| Fact family | Current producer | Current fact source | Classification |
| --- | --- | --- | --- |
| Runtime helper identity | `src/backend/bir/lir_to_bir/calling.cpp::lower_runtime_intrinsic_inst` creates `llvm.va_arg.*` and `llvm.va_arg.aggregate` placeholder `CallInst`s | Placeholder `callee` string plus runtime-placeholder shape; `callee_link_name_id` intentionally invalid | BIR placeholder metadata/name token |
| Scalar payload type | `lower_runtime_intrinsic_inst` scalar `lower_va_arg_call` path | `CallInst::return_type`, `result`, and `result_abi = compute_function_return_abi(...)` for scalar/fp/function-pointer values | BIR metadata |
| Scalar payload ABI | `src/backend/prealloc/variadic_entry_plans.cpp::make_aapcs64_scalar_va_arg_access_plan` | Recomputed with `infer_call_arg_abi(target_profile, call.return_type)` | Prealloc reconstruction from BIR scalar type |
| Aggregate payload size/alignment | `lower_runtime_intrinsic_inst` aggregate `lower_va_arg_call` path | `lower_byval_aggregate_layout(...)` then `CallInst::arg_abi[0]` as sret pointer with `size_bytes`, `align_bytes`, `primary_class = Memory` | BIR metadata |
| Aggregate destination payload home | `populate_aapcs64_variadic_entry_helper_operand_home_authority` | `prepared_home_for_named_value(...)` for `call->args[0]` on `llvm.va_arg.aggregate` | Prealloc value-home reconstruction |
| Aggregate overflow copy/progression | `make_aapcs64_aggregate_va_arg_access_plan` | `payload_abi.size_bytes`, `payload_abi.align_bytes`, AAPCS64 va_list fields, and overflow/register-save area layout | Legitimate prealloc placement decision from BIR size/align |
| HFA lane count and lane size | `infer_aapcs64_hfa_va_arg_shape` | Post-call `LoadLocalInst` sequence, `aggregate_slot_suffix_offset(load->slot_name, call.args.front().name)`, lane load `result.type`, and `payload_abi.size_bytes` consistency | Name/load-derived prealloc inference |
| HFA lane destination homes | `infer_aapcs64_hfa_va_arg_shape` | Prepared addressing lookup, frame-slot ids, byte offsets, frame-slot layout, and load result value names | Load/slot/frame-derived prealloc inference |
| Fixed-call HFA lane facts for comparison | BIR fixed-call lowering and `module.cpp::apply_aarch64_fixed_hfa_pressure` | `CallArgAbiInfo::aarch64_hfa_lane_count` and `aarch64_hfa_lane_index` | BIR metadata, but not currently published for variadic `va_arg` payload shape |

#### Consumer Surfaces

| Consumer | Consumed facts | Current behavior |
| --- | --- | --- |
| `prepared_variadic_entry_helper_kind_for_call` / `prepared_variadic_entry_helper_kind_for_callee` | Runtime placeholder shape and raw `llvm.va_arg.*` / `llvm.va_arg.aggregate` callee spellings | Classifies scalar vs aggregate helpers from placeholder name |
| `populate_aapcs64_variadic_entry_helper_operand_home_authority` | Helper kind, scalar result home, aggregate destination payload home, source `va_list` home | Builds `PreparedVariadicEntryHelperOperandHomes` per helper call |
| `make_aapcs64_scalar_va_arg_access_plan` | `call.return_type`, va_list layout, register-save slot sizes, operand homes | Reconstructs scalar payload ABI and selects GP, FP, or overflow source class |
| `make_aapcs64_aggregate_va_arg_access_plan` | `call.arg_abi[0]`, aggregate destination home, va_list layout, register-save/overflow layout | Builds overflow aggregate copy plan, then upgrades HFA to FP register-save area if inferred |
| `infer_aapcs64_hfa_va_arg_shape` | Aggregate destination name, subsequent local loads, aggregate slot suffixes, prepared addressing, frame slots | Infers HFA lane count, lane size, and per-lane destination homes from lowered load/slot/frame shape |
| `src/backend/prealloc/prepared_printer/variadic.cpp` | Scalar/aggregate access plans, including source class, payload size/align, lane count/size | Dumps consumer-visible plan facts; useful proof surface for a future contract test |
| AArch64 target instruction record tests | Prepared scalar/aggregate access plans | `backend_aarch64_target_instruction_records_test.cpp` verifies missing access plans fail closed and selected scalar/aggregate plans drive call instruction selection |
| BIR lowering/prepare liveness tests | Placeholder calls and helper homes | `backend_lir_to_bir_notes_test.cpp` checks HFA `va_arg` lowers through `llvm.va_arg.aggregate`; `backend_prepare_liveness_test.cpp` checks aggregate va_arg BIR `arg_abi` and helper operand homes |

#### Authority Split

- Scalar: BIR publishes the scalar result type and result ABI on the placeholder
  call, but prealloc recomputes the AAPCS64 argument-style payload ABI from
  `call.return_type`. The semantic source is type metadata from BIR; the
  payload access class/stride is currently prealloc reconstruction.
- Non-HFA aggregate: BIR publishes aggregate payload size/alignment as
  sret-style `arg_abi[0]`; prealloc legitimately owns va_list field selection,
  overflow stride, copy size, and destination-home placement.
- HFA aggregate: BIR publishes only aggregate size/alignment and lowered local
  aggregate storage shape. Prealloc currently infers lane count, lane size, and
  lane destination homes from load order, aggregate slot-name suffixes,
  prepared addressing, and frame slots. This is the main authority gap for
  Step 2.
- Name/load/slot/frame-derived inference currently exists only in the HFA
  aggregate path: raw helper names classify the placeholder family, and
  `infer_aapcs64_hfa_va_arg_shape` derives payload lane semantics from
  local-load and frame-placement artifacts.

## Suggested Next

Proceed to Step 2: decide whether scalar payload ABI and aggregate/HFA shape
facts should be explicit BIR placeholder metadata or a constrained AAPCS64
helper-local reconstruction contract.

## Watchouts

- Do not add or endorse slot-name parsing as payload-shape authority.
- Keep fixed-call HFA pressure and unrelated AArch64 ABI behavior out of scope.
- The strongest current gap is HFA lane-shape authority; scalar has BIR type
  metadata but prealloc recomputes ABI, while non-HFA aggregate has BIR
  size/align metadata and mostly legitimate prealloc placement decisions.
- Existing proof surfaces already include prepared-printer variadic dumps,
  `backend_aarch64_target_instruction_records_test.cpp`,
  `backend_lir_to_bir_notes_test.cpp`, and
  `backend_prepare_liveness_test.cpp`.

## Proof

Command:

```sh
git diff --quiet -- src/backend/bir src/backend/prealloc && printf 'analysis-only proof: no implementation diff under src/backend/bir or src/backend/prealloc\n' > test_after.log
```

Result: passed. `test_after.log` contains the delegated proof output, and the
command confirmed no implementation diff under `src/backend/bir` or
`src/backend/prealloc`.
