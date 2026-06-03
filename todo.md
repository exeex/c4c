Status: Active
Source Idea Path: ideas/open/102_aapcs64_va_arg_payload_shape_authority.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Decide The Narrow Authority Contract

# Current Packet

## Just Finished

Completed `plan.md` Step 2: chose the narrow authority contract for AAPCS64
`va_arg` payload ABI and aggregate/HFA shape facts without implementation or
test changes.

### Step 2 Contract Decision

Chosen route: BIR runtime `va_arg` placeholders should publish the semantic
payload facts that determine AAPCS64 payload shape; prealloc should own only
helper planning, va_list/register-save/overflow placement, value homes, and
target-facing access-plan assembly. Do not keep HFA shape as an anonymous
load/slot/frame-derived inference, and do not add more slot-name parsing.

#### Contract Surface

- Add or expose a named BIR/runtime-placeholder payload contract on
  `bir::CallInst` for `llvm.va_arg.*` and `llvm.va_arg.aggregate`.
- The contract should be populated in
  `src/backend/bir/lir_to_bir/calling.cpp::lower_runtime_intrinsic_inst`.
- The preferred implementation shape is to reuse/extend existing ABI metadata
  rather than introduce a broad new ABI subsystem:
  - scalar `va_arg`: publish a named scalar payload ABI authority derived at
    placeholder-lowering time from the scalar result type and target profile.
  - non-HFA aggregate `va_arg`: continue using `CallInst::arg_abi[0]` as the
    explicit payload size/alignment and memory/sret authority.
  - HFA aggregate `va_arg`: publish lane count and lane size as explicit
    placeholder payload facts. Existing `CallArgAbiInfo` already has
    `aarch64_hfa_lane_count` and `aarch64_hfa_lane_index` for fixed-call lanes;
    the implementation can either reuse that family on the aggregate payload
    ABI entry or add a narrowly named `va_arg` payload-shape field if reuse
    would make fixed-call lane semantics ambiguous.
- Prealloc should consume this contract in
  `make_aapcs64_scalar_va_arg_access_plan` and
  `make_aapcs64_aggregate_va_arg_access_plan`, producing the existing
  `PreparedVariadicScalarVaArgAccessPlan` and
  `PreparedVariadicAggregateVaArgAccessPlan` fields.

#### Explicit BIR Placeholder Metadata

- Runtime helper family identity: still based on runtime-placeholder shape plus
  the known `llvm.va_arg.*`/`llvm.va_arg.aggregate` helper spelling until the
  separate placeholder-identity idea replaces that name token. Step 2 does not
  require another identity route.
- Scalar payload facts: BIR must be the named semantic authority for payload
  type and ABI class/size/alignment. Prealloc may compute AAPCS64 source field
  selection from that published ABI plus va_list/register-save layout, but it
  should not silently recompute the payload ABI from `call.return_type` without
  a named contract.
- Non-HFA aggregate facts: BIR remains authority for payload size/alignment and
  memory/sret shape through `call.arg_abi[0]`.
- HFA aggregate facts: BIR must be authority for lane count and lane size for
  the aggregate `va_arg` payload. These facts should be visible through the
  placeholder metadata or through the resulting prepared dump/test surface.

#### Constrained Helper-Local Reconstruction

- Allowed: prealloc may reconstruct operand homes from prepared value
  locations, and it may compute va_list field offsets, source fields,
  progression strides, overflow strides, register-save slot sizes, and
  destination-home placement from target layout and prepared value homes.
- Allowed: aggregate lane destination homes may remain helper-local placement
  facts because they depend on the destination value homes and prepared
  addressing, not on source ABI classification.
- Rejected: inferring HFA lane count or lane size from
  `aggregate_slot_suffix_offset`, post-call load order, local slot names,
  prepared addressing, or frame-slot layout. Those can be retained only as a
  temporary compatibility fallback if Step 3 names the predicate, restricts it
  to `llvm.va_arg.aggregate` runtime placeholders, and fails closed when the new
  explicit BIR payload facts are absent for cases that should have them.
- Rejected: testcase-shaped matching, additional slot-name parsing, or changing
  fixed-call HFA pressure to mask the variadic contract gap.

#### Implementation Targets

- `src/backend/bir/bir.hpp`: add or reuse narrow payload ABI/HFA shape fields
  on `CallInst`/`CallArgAbiInfo` with names that distinguish `va_arg` payload
  authority from ordinary fixed-call lanes if needed.
- `src/backend/bir/lir_to_bir/calling.cpp`: populate scalar payload ABI and
  aggregate/HFA shape facts in `lower_runtime_intrinsic_inst`, including the
  aggregate path that currently builds `llvm.va_arg.aggregate`.
- `src/backend/prealloc/variadic_entry_plans.cpp`: replace scalar silent
  `infer_call_arg_abi(target_profile, call.return_type)` authority and HFA
  `infer_aapcs64_hfa_va_arg_shape` lane-shape inference with consumption of the
  named BIR placeholder facts.
- `src/backend/prealloc/variadic.hpp`: adjust
  `PreparedVariadic*VaArgAccessPlan` only if the current fields are not enough
  to expose the selected contract; the current scalar and aggregate plan fields
  already carry source class, payload size/align, and HFA lane count/size.
- `src/backend/prealloc/prepared_printer/variadic.cpp`: keep or strengthen dump
  output so scalar source class, aggregate payload size/align, and HFA lane
  count/size are observable.

#### Focused Proof Targets

- Scalar proof: AAPCS64 `llvm.va_arg.i32` and `llvm.va_arg.f64` routes produce
  prepared scalar access plans from explicit payload ABI authority, covering GP
  and FP source classes plus overflow behavior where practical.
- Aggregate proof: non-HFA aggregate `llvm.va_arg.aggregate` uses BIR
  `arg_abi[0]` payload size/alignment and destination-home placement without
  HFA lane metadata.
- HFA proof: HFA aggregate `va_arg` publishes and consumes lane count/lane size
  without relying on load order or slot-name suffix parsing; prepared output
  should show `register_save_lanes` and `register_save_lane_size`.
- Contract/dump proof surfaces: use or extend
  `tests/backend/bir/backend_lir_to_bir_notes_test.cpp`,
  `tests/backend/bir/backend_prepare_liveness_test.cpp`,
  `tests/backend/mir/backend_aarch64_target_instruction_records_test.cpp`, and
  prepared-printer variadic dump assertions. No expectations should be weakened
  or marked unsupported.

## Suggested Next

Proceed to Step 3: implement the contract by publishing the selected BIR
runtime-placeholder payload facts and updating prealloc variadic entry planning
to consume them.

## Watchouts

- Do not add or endorse slot-name parsing as payload-shape authority.
- Keep fixed-call HFA pressure and unrelated AArch64 ABI behavior out of scope.
- Avoid making fixed-call `aarch64_hfa_lane_count/index` semantics ambiguous if
  those fields are reused for `va_arg` aggregate payload shape; use a narrowly
  named payload-shape field instead if the distinction matters.
- Runtime helper identity by raw `llvm.va_arg.*` spelling remains a separate
  placeholder-identity concern; this packet only decides payload ABI/shape
  authority.
- The implementation should fail closed or emit missing facts when expected
  payload metadata is absent, rather than falling back silently to old
  load/slot/frame inference.

## Proof

Command:

```sh
git diff --quiet -- src/backend/bir src/backend/prealloc tests && printf 'analysis-only proof: no implementation or test diff for Step 2 contract decision\n' > test_after.log
```

Result: passed. `test_after.log` contains the delegated proof output, and the
command confirmed no implementation or test diff under `src/backend/bir`,
`src/backend/prealloc`, or `tests`.
