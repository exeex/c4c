# Current Packet

Status: Active
Source Idea Path: ideas/open/52_aarch64_calls_prepared_authority_repair.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Classify by-value aggregate call lane publication

## Just Finished

Step 1 classified the remaining by-value aggregate variadic call lane
publication failure without implementation edits.

Relevant `calls.cpp` owners:

- `lower_call_instruction` requires the authoritative `PreparedCallPlan` and
  routes direct calls into `lower_prepared_call_instruction`.
- `lower_prepared_call_instruction` copies `call_plan.arguments` into the
  `CallInstructionRecord`; the final call node itself is prepared-aware.
- `lower_before_call_moves` lowers the `BeforeCall` move bundle by calling
  `lower_before_call_move`.
- `lower_before_call_move` is the publication owner for prepared call argument
  ABI lanes. Its by-value aggregate register/stack lane path is guarded by
  `is_aarch64_byval_register_lane_move`, `selected_byval_lane_extent_bytes`,
  `has_selected_byval_register_lane_source`, and
  `make_byval_register_lane_prepared_source`.

Prepared facts for `stdarg` in `00204`:

- First direct variadic `myprintf` at `block_index=0 inst_index=169`
  (`"%9s %9s %9s %9s %9s %9s"`): args 1-6 are all
  `arg.source_selection=byval_register_lane`. Args 1-3 publish to GPR pairs
  `x1/x2`, `x3/x4`, `x5/x6`; args 4-6 publish to outgoing stack offsets
  `0`, `16`, `32`. Source selections identify payload homes at stack offsets
  `0`, `16`, `32`, `48`, `64`, `80` with
  `selection_byval_lane_extent=16`.
- Second direct variadic `myprintf` at `block_index=0 inst_index=333`
  (`"%7s %9s %9s %9s %9s %9s"`): the leading 7-byte aggregate is
  `arg index=1 source_value_id=2348 source_encoding=register source_reg=x13`
  with before-call move `reason=call_arg_register_to_register`, no
  `ByvalRegisterLane` source selection, and no prepared payload-source fields.
  The following 9-byte aggregates have `ByvalRegisterLane` selections and
  publish to `x2/x3`, `x4/x5`, `x6/x7`, then stack offsets `0`, `16`.
- Passing probe
  `backend_codegen_route_aarch64_variadic_aggregate_overflow_byte_copy` has
  the expected shape for 9-byte overflow publication: the `pick_ninth` call
  records `ByvalRegisterLane` for every by-value aggregate, with register
  pairs `x1/x2`, `x3/x4`, `x5/x6` followed by stack offsets `0`, `16`, `32`,
  `48`, `64`, `80`.

Emitted lane mismatch:

- The `00204` proof still fails in `stdarg`. Expected output begins
  `ABCDEFGHI ABCDEFGHI ABCDEFGHI ABCDEFGHI ABCDEFGHI ABCDEFGHI` then
  `lmnopqr ABCDEFGHI ABCDEFGHI ABCDEFGHI ABCDEFGHI ABCDEFGHI`.
- Actual output begins `ABCDEFGHI ABCDEFGHI stdarg:` and then corrupts the
  `%7s` lane before later `%9s` lanes. Generated `stdarg` publishes the first
  call as `x1=[sp]`, `x2=[sp,#8]`, `x3=[sp,#16]`, `x4=[sp,#24]`,
  `x5=[sp,#32]`, `x6=[sp,#40]`, with overflow bytes copied through `x16`.
  For the second call it emits `mov x1, x13` for the leading 7-byte aggregate
  while the 9-byte arguments use prepared byval lane loads/copies.

Classification: `calls.cpp` is not ignoring an existing `ByvalRegisterLane`
fact for the 7-byte lane; that fact is missing from the prepared call plan
because `call_plans.cpp::prepared_byval_lane_extent_bytes` only selects
`ByvalRegisterLane` when the before-call move reason is
`call_arg_byval_aggregate_register_lanes`. The 7-byte direct variadic lane is
currently planned as scalar `call_arg_register_to_register`, so `calls.cpp`
falls back to scalar register publication. The next repair should define a
shared prepared query/plan fact for small direct variadic by-value aggregate
payloads that occupy one GPR lane, rather than patching `calls.cpp` to infer it
from value spelling or the test case.

## Suggested Next

Delegate Step 2 to define the prepared call argument publication contract.
Small single-GPR by-value aggregates used by direct variadic calls need the
same semantic source-selection authority as the existing multi-lane
`ByvalRegisterLane` path. The contract should be keyed by argument identity and
ABI binding, not `00204`, `myprintf`, `%7s`, or temporary spelling.

Proof command:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_codegen_route_aarch64_pointer_select_aggregate_byte_copy|backend_codegen_route_aarch64_variadic_aggregate_overflow_byte_copy|backend_codegen_route_aarch64_alu_unpublished_load_local_(after_call|call_boundary)|c_testsuite_aarch64_backend_src_00164_c|c_testsuite_aarch64_backend_src_00176_c|c_testsuite_aarch64_backend_src_00181_c|c_testsuite_aarch64_backend_src_00204_c)$'`

## Watchouts

Do not special-case `00204`, `myprintf`, `%7s`, `%9s`, specific temporary names,
or string literal contents.

Do not reopen idea 56's edge preservation, scalar cast source consumption,
`memory.cpp` va_list cursor writeback, or `dispatch_edge_copies.cpp` edge
source selection unless a later packet proves the call route classification was
wrong.

The 9-byte lanes already have prepared `ByvalRegisterLane` facts in both
`00204` and the passing overflow byte-copy probe. The gap is the 7-byte
single-lane direct variadic aggregate planned as `call_arg_register_to_register`
with no payload-source selection.

Treat existing dirty `memory.cpp`, `dispatch_edge_copies.cpp`, and transient
`review/*` files as external context.

## Proof

Proof command:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_codegen_route_aarch64_pointer_select_aggregate_byte_copy|backend_codegen_route_aarch64_variadic_aggregate_overflow_byte_copy|backend_codegen_route_aarch64_alu_unpublished_load_local_(after_call|call_boundary)|c_testsuite_aarch64_backend_src_00164_c|c_testsuite_aarch64_backend_src_00176_c|c_testsuite_aarch64_backend_src_00181_c|c_testsuite_aarch64_backend_src_00204_c)$'`

Result: build passed, focused subset `7/8` passed, and only
`c_testsuite_aarch64_backend_src_00204_c` failed with `[RUNTIME_MISMATCH]`.
Proof log path: `test_after.log`.
