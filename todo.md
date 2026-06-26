Status: Active
Source Idea Path: ideas/open/386_rv64_object_route_same_module_byval_aggregate_call_args.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add Focused Call-Contract Coverage

# Current Packet

## Just Finished

Step 1: Audit Prepared Callsite Facts completed for `src/20030914-2.c`
`main` / `entry` / inst `36`.

Observed prepared BIR and callsite facts from existing
`build/rv64_gcc_c_torture_backend/src_20030914-2.c/prepared.log`:

- BIR instruction: `%t1 = bir.call i32 f(ptr byval(size=72, align=4) %t0, i32 4660)`.
- Source annotations: `call_arg_source index=0 encoding=frame_slot source_value=%t0 selection=frame_slot_address selection_value=%t0`; `index=1 encoding=immediate`.
- Wrapper/callee: `call block_index=0 inst_index=36 wrapper_kind=same_module variadic_fpr_arg_register_count=0 indirect=no callee=f`.
- Return placement: result is GPR, `source_storage=register` from `a0` with `source_placement=gpr:call_result#0/w1`, to `%t1` value id `38` in `t0` with `dest_placement=gpr:caller_saved#0/w1`; late result publication facts are present.
- Arg0: `value_bank=aggregate_address source_encoding=register source_value_id=39 source_placement=gpr:callee_saved#0/w1 source_reg=s1 source_bank=gpr dest_bank=none`; summary line is `arg0 bank=aggregate_address from=register:s1 to=none`.
- Arg1: `value_bank=gpr source_encoding=immediate source_literal=4660 source_bank=none dest_placement=gpr:call_argument#1/w1 dest_reg=a1 dest_bank=gpr`; summary line is `arg1 bank=gpr from=immediate:4660 to=a1`.
- Byval size/alignment: available on the BIR call operand as `byval(size=72, align=4)`; callee param stack object also records `object #18 func=f name=%p.pa source_kind=byval_param type=ptr size=72 align=4`. The prepared call argument plan itself does not publish byval size/alignment.
- Source placement for `%t0`: storage/value home value id `39` is register `s1`, with address materialization at inst `36` as `kind=frame_slot result=%t0 offset=0`; stack layout contains 18 four-byte local slots `%t0.0` through `%t0.68` at offsets `0..68`.
- Destination placement/outgoing stack: the before-call move bundle has `move from_value_id=39 to_value_id=39 destination_kind=call_argument_abi destination_storage=stack_slot abi_index=0 reason=call_arg_register_to_stack` plus ABI bindings for arg0 stack and arg1 `a1`. The call plan has no `outgoing_stack_argument_area=` line, because arg0 has `dest_bank=none` and no destination stack offset/size in the argument plan.
- Preserves/clobbers: `%t0` value id `39` is preserved by `callee_saved_register` in `s1`; clobbers are singleton reserved scratch `gpr:t0`, `fpr:ft0`, and `vreg:v0` through `v15`.

RV64 object-route call emission requirements observed in
`src/backend/mir/riscv/codegen/prepared_call_emit.cpp`:

- `emit_riscv_simple_call` requires an indexed prepared call plan, zero variadic FPR args, no memory return, no prepared outgoing stack argument area, matching argument/result counts, and direct same-module or direct-extern/indirect callee identity agreement.
- `emit_riscv_byval_aggregate_address_argument` currently accepts arg0 only when the call argument plan has `AggregateAddress`, register/GPR source with a source register name, no destination register, no destination stack offset/size, no aggregate transport, and a mappable RV64 argument register for the arg index.
- For the stack-copy path it additionally consumes `CallArgAbiInfo` from the BIR call (`byval_copy`, `size_bytes > 0`, `size_bytes <= 128`, `align_bytes <= 16`) and reconstructs payload fields by scanning prepared stack-layout objects whose names match the aggregate source suffix. It then allocates aligned outgoing payload space, copies 1/4/8-byte fields, moves the payload pointer to `a0`, lowers arg1 to `a1`, emits `call f`, restores the active stack adjustment, and moves result `a0` to `t0`.
- If the payload-field scan fails, it can fall back to a prepared frame-slot address for `source_value_id`; otherwise it can simply move the source register to the computed argument register.

Conclusion: the first concrete shape is mechanically lowerable by the current
RV64 object-route helper, but accepting it as this plan's semantic fix would
depend on facts not yet published as a narrow prepared call-contract: arg0's
byval size/alignment and the exact outgoing byval payload copy/source-lane
requirements are not carried by `PreparedCallArgumentPlan` or by an outgoing
stack area record for inst `36`. Step 2 should first add focused call-contract
coverage, or a small prepared contract extension, that publishes the selected
same-module byval aggregate/address payload contract explicitly instead of
letting the target reconstruct it from BIR arg ABI plus stack-object names.

## Suggested Next

Execute Step 2 from `plan.md`: add focused prepared call-contract coverage for
same-module byval aggregate/address arg0 with register source, stack payload
ABI destination, explicit byval size/alignment, payload source lanes, no
outgoing dynamic stack area, scalar arg1 in `a1`, and GPR result `a0 -> t0`.

## Watchouts

- Do not reopen prepared global-symbol publication; idea 384 closed that
  boundary.
- Do not treat closed byval parameter-home idea 370 as owning this callsite
  movement boundary.
- Keep aggregate `va_arg` and non-register parameter-home work routed to ideas
  371 and 374 respectively.
- Do not modify or rely on unrelated EV64 idea 385 for this lifecycle route.
- A target-only implementation that reads `CallArgAbiInfo` and stack-object
  name suffixes may pass the representative, but it is weaker than the plan's
  prepared-call authority rule unless Step 2 explicitly blesses or publishes
  those facts as the prepared contract.

## Proof

Audit-only packet; no build/test proof was required and no canonical proof logs
were created or overwritten.

```sh
git status --short
rg -n "20030914-2|inst 36|wrapper=same_module|aggregate_address|byval\\(size=72|from=register:s1|immediate:4660" -S .
rg -n "CallInst|callsite|prepared call|same_module|outgoing stack|clobber|return placement|ret.*placement|aggregate_address" src tests -S
which c4c-clang-tool && which c4c-clang-tool-ccdb && ls build/compile_commands.json
find build -path '*20030914-2.c*' -maxdepth 6 -type f | sort
rg -n "bir.func @main|callsite block=0 inst=36|call block_index=0 inst_index=36|arg0 bank=aggregate_address|arg index=0|result bank|move .*inst_index=36|clobber|outgoing_stack|%t1 = bir.call|storage %t0|storage %t1|block entry" build/rv64_gcc_c_torture_backend/src_20030914-2.c/prepared.log -C 4
sed -n '1,220p' build/rv64_gcc_c_torture_backend/src_20030914-2.c/case.log
c4c-clang-tool-ccdb find-definition /workspaces/c4c/src/backend/mir/riscv/codegen/prepared_call_emit.cpp emit_riscv_simple_call build/compile_commands.json
c4c-clang-tool-ccdb find-definition /workspaces/c4c/src/backend/mir/riscv/codegen/prepared_call_emit.cpp emit_riscv_byval_aggregate_address_argument build/compile_commands.json
rg -n "source_value_id=39|to_value_id=39|from_value_id=39|destination_kind=call_argument_abi|source_kind=byval|byval|outgoing_stack_argument|stack_argument|aggregate_transport|source_selection|frame_slot_address|%t0" build/rv64_gcc_c_torture_backend/src_20030914-2.c/prepared.log -C 3
```

Result: audit completed from existing dumps and source inspection; no
`test_after.log` proof applies to this packet.
