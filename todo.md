# Current Packet

Status: Active
Source Idea Path: ideas/open/52_aarch64_calls_prepared_authority_repair.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Define the prepared call argument publication contract

## Just Finished

Step 2 defined the smallest prepared call argument publication contract for
small single-GPR direct variadic by-value aggregate payloads.

Selected contract:

- The owner is prepared call planning/source selection in
  `src/backend/prealloc/call_plans.cpp`, specifically
  `select_prepared_call_argument_source` and its
  `prepared_byval_lane_extent_bytes` decision.
- A small direct variadic by-value aggregate payload that occupies one GPR lane
  should receive the same semantic
  `PreparedCallArgumentSourceSelectionKind::ByvalRegisterLane` source
  selection as the existing multi-lane by-value aggregate path.
- The selection must be keyed by call argument identity and ABI binding:
  `PreparedCallPlan::wrapper_kind == DirectExternVariadic`,
  `PreparedCallArgumentPlan::arg_index`, the matching before-call ABI move,
  `bir::CallArgAbiInfo` with `byval_copy`, `passed_in_register`,
  `primary_class == Integer`, nonzero `size_bytes <= 16`, GPR destination
  binding, and a source value/home that can resolve payload bytes.
- Existing multi-lane behavior remains authoritative when the before-call move
  reason is `call_arg_byval_aggregate_register_lanes`; the new branch should
  only add the missing single-GPR direct variadic payload selection and should
  reuse the same `select_byval_payload_lane_source`/source-selection fields.

Rejected owners:

- Do not repair this in `calls.cpp` by scanning raw call operands, temporary
  names, string literal contents, `myprintf`, `%7s`, `%9s`, or testcase names.
  `calls.cpp` already has a prepared-selection consumer path for register
  moves through `selected_byval_lane_extent_bytes` and
  `make_byval_register_lane_prepared_source`.
- Do not add a shared `prepared_lookups.cpp` query for this packet. The missing
  fact is local to construction of each `PreparedCallArgumentPlan`; no separate
  indexed lookup is needed unless a later consumer outside call planning needs
  the same fact.
- Do not change boundary effects or move-bundle authority first. The existing
  before-call move still provides the ABI binding and destination register; the
  missing semantic payload-source fact is the call argument source selection.

Exact next implementation files:

- Primary: `src/backend/prealloc/call_plans.cpp`.
- Touch `src/backend/prealloc/calls.hpp` only if the implementation needs a
  small field/helper signature adjustment; no new source-selection kind is
  expected.
- Do not touch `src/backend/mir/aarch64/codegen/calls.cpp` unless the Step 3
  implementation proves the existing prepared-selection consumer rejects the
  newly selected single-GPR case.

## Suggested Next

Delegate Step 3 to implement the contract in `call_plans.cpp`. Extend the
by-value lane selection decision so a direct extern variadic, by-value,
integer-class aggregate argument with ABI register destination and `size_bytes
<= 16` gets `ByvalRegisterLane` source selection even when its before-call move
is currently classified as scalar `call_arg_register_to_register`.

The implementation should preserve the existing multi-lane
`call_arg_byval_aggregate_register_lanes` route unchanged, then run:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_codegen_route_aarch64_pointer_select_aggregate_byte_copy|backend_codegen_route_aarch64_variadic_aggregate_overflow_byte_copy|backend_codegen_route_aarch64_alu_unpublished_load_local_(after_call|call_boundary)|c_testsuite_aarch64_backend_src_00164_c|c_testsuite_aarch64_backend_src_00176_c|c_testsuite_aarch64_backend_src_00181_c|c_testsuite_aarch64_backend_src_00204_c)$'`

## Watchouts

Do not special-case `00204`, `myprintf`, `%7s`, `%9s`, temporary value names,
or string literal contents.

The 9-byte lanes already have and use `ByvalRegisterLane` prepared facts in
both `00204` and the passing overflow byte-copy probe. Step 3 should add only
the missing single-GPR direct variadic by-value aggregate payload fact, not
replace or weaken the multi-lane path.

`calls.cpp` has multiple by-value aggregate publication branches. The relevant
single-GPR register-source branch already checks
`selected_byval_lane_extent_bytes(*argument)` and rewrites the scalar
before-call move to `call_arg_byval_aggregate_register_lanes` after consuming
prepared source bytes. Prefer proving that path before widening.

Treat existing dirty `memory.cpp`, `dispatch_edge_copies.cpp`, and transient
`review/*` files as external context.

## Proof

Proof command:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_codegen_route_aarch64_pointer_select_aggregate_byte_copy|backend_codegen_route_aarch64_variadic_aggregate_overflow_byte_copy|backend_codegen_route_aarch64_alu_unpublished_load_local_(after_call|call_boundary)|c_testsuite_aarch64_backend_src_00164_c|c_testsuite_aarch64_backend_src_00176_c|c_testsuite_aarch64_backend_src_00181_c|c_testsuite_aarch64_backend_src_00204_c)$'`

Result: build passed, focused subset `7/8` passed, and only
`c_testsuite_aarch64_backend_src_00204_c` failed with `[RUNTIME_MISMATCH]`.
This matches the expected unresolved baseline for this contract-only packet.
Proof log path: `test_after.log`.
