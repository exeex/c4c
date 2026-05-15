Status: Active
Source Idea Path: ideas/open/249_prepared_i128_helper_marshaling_abi_binding.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect Helper Marshaling And ABI Binding Gap

# Current Packet

## Just Finished

Step 1 inspected the prepared i128 helper authority, selected AArch64
helper-boundary records, existing call-boundary vocabulary, and the terminal
printer blocker.

Current surfaces:

- Prepared producer: `PreparedI128RuntimeHelper` records are produced for i128
  div/rem source operations in `regalloc.cpp`, then enriched in `prealloc.cpp`
  after canonical `PreparedI128Carrier` facts exist.
- Prepared helper facts currently include source block/instruction identity,
  opcode, helper kind/callee, result/lhs/rhs value identity, low/high carrier
  lane bindings, direct low/high result ownership, broad resource policy,
  broad ABI shape/banks, caller-saved clobbers, and missing-fact diagnostics.
- Selected consumer: `I128RuntimeHelperBoundaryRecord` preserves the prepared
  helper/callee identity, carrier-derived result/lhs/rhs low/high registers,
  resource policy, ABI shape/banks, clobbers, and source-helper provenance.
- Machine record effects currently model result carrier lanes as defs, source
  carrier lanes as uses, prepared clobbers as clobbers, and call as a side
  effect.
- Terminal printer: `print_i128_runtime_helper` validates helper/callee,
  direct result ownership, resource/clobber policy, and broad ABI shape, then
  fails closed with `i128 helper boundary printing requires structured helper
  marshaling and ABI register-binding facts`.
- Existing generic call-boundary move and ABI-binding records are tied to real
  `bir::CallInst` move bundles/ABI bindings. Helper-required i128 div/rem
  operations are not represented as fake retained calls and do not currently
  have helper-specific move bundles or ABI lane bindings.

Missing facts blocking executable helper-call printing:

- Per-lane ABI argument bindings for supported div/rem helpers: lhs low/high
  and rhs low/high must each carry argument index, lane role/index, ABI register
  bank/class/view, register name, occupied register set, contiguous width,
  lane width, and target ABI provenance.
- Per-lane direct-result ABI bindings: result low/high must carry the same
  structural register-binding facts for the helper return registers.
- Carrier-lane to ABI-register marshaling facts are missing for source lanes:
  each lhs/rhs carrier lane needs an explicit before-call move/binding from
  prepared carrier register authority to the helper ABI argument register.
- ABI-register to carrier-lane unmarshaling facts are missing for direct
  results: each helper ABI result register needs an explicit after-call
  move/binding to the prepared result carrier lane.
- Selected-call ownership is missing: no field currently states that a helper
  boundary record owns a terminal `bl <callee>` after all argument/result
  bindings, marshaling/unmarshaling moves, clobbers, and resources are present.
- Live preservation around the helper call is not yet explicit for values that
  must survive the helper boundary. Current clobber facts name caller-saved
  clobbers, but there is no helper-specific preserved-live lane policy or
  selected-call ownership check.

First implementation packet target for Step 2:

Add prepared/shared low/high ABI binding authority to
`PreparedI128RuntimeHelper` for supported direct-result div/rem helpers only.
The first packet should add a structured binding carrier family beside the
existing lane bindings, populated by the prepared producer from target ABI
policy, not by the AArch64 printer. It should expose:

- lhs/rhs argument low/high ABI register bindings with argument index, lane
  role/index, lane width, bank/class/view, register name, occupied registers,
  contiguous width, helper kind, callee, and source-operation identity.
- direct result low/high ABI register bindings with result lane role/index,
  lane width, bank/class/view, register name, occupied registers, contiguous
  width, helper kind, callee, and result value identity.
- fail-closed diagnostics for unsupported target ABI, missing target binding
  policy, incomplete direct-result ownership, non-register-pair carriers,
  memory-return helpers, conversion helpers, missing callee identity, and
  incomplete register binding state.
- prepared-printer output and focused prepared tests proving the binding facts
  exist independently of selected AArch64 printer output.

This packet should not add marshaling move records, selected helper-call
printing, fake `PreparedCallPlan` entries, target-local fixed-register
marshaling, scalar-i64 substitutes, conversion helper mapping, or memory-return
helper support.

Suggested Step 2 focused proof:

```sh
(cmake --build build -j2 && ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_liveness|backend_prepare_frame_stack_call_contract|backend_prepared_printer|backend_aarch64_target_instruction_records|backend_aarch64_instruction_dispatch|backend_aarch64_machine_printer)$') > test_after.log 2>&1
```

## Suggested Next

Execute Step 2: add low/high ABI argument and direct-result binding carriers in
prepared/shared state for supported i128 div/rem helpers. Keep AArch64 terminal
helper-call printing fail-closed until later packets add marshaling/unmarshaling
facts and selected-call ownership.

## Watchouts

- Do not reintroduce terminal `bl <callee>` output from
  `I128RuntimeHelperBoundaryRecord` lane facts alone.
- Do not hard-code helper ABI registers in the AArch64 printer or infer them
  from source opcodes, rendered names, numeric adjacency, or testcase-shaped
  `x0..x5` fixtures.
- Existing generic `PreparedMoveBundle` and `PreparedAbiBinding` facts are
  produced for real `bir::CallInst` call boundaries. Helper div/rem source
  operations need producer-owned helper-specific binding facts first.
- Clobber facts alone are not selected-call ownership. Later packets still need
  helper marshaling/unmarshaling records and live-preservation/selected-call
  authority before executable helper-call printing can resume.
- Float/i128 conversion helpers and memory-return helper families remain
  outside this prerequisite packet.

## Proof

Inspection-only packet. No build was required and no proof logs were modified.

Read-only inspection commands used `sed` and `rg` over prepared helper,
selected AArch64 helper-boundary, machine-printer, and focused test surfaces.
