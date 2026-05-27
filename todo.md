# Current Packet

Status: Active
Source Idea Path: ideas/open/52_aarch64_calls_prepared_authority_repair.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Classify by-value aggregate call lane publication

## Just Finished

Lifecycle switched away from idea 56 because Step 5 classified the remaining
`00204` failure as AArch64 variadic by-value aggregate call argument
publication, which idea 56 explicitly treats as out of scope.

Idea 56 remains open but parked with a durable note. The active source idea is
now idea 52, which owns duplicate call authority in
`src/backend/mir/aarch64/codegen/calls.cpp`.

## Suggested Next

Delegate Step 1 as a classification-only executor packet. The packet should
inspect `calls.cpp` and prepared call plans for direct variadic calls carrying
7-byte and 9-byte by-value aggregate payloads, compare emitted GPR/overflow
stack lanes against the passing variadic aggregate byte-copy probe, and record
the exact missing or ignored prepared authority before implementation.

Proof command:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_codegen_route_aarch64_pointer_select_aggregate_byte_copy|backend_codegen_route_aarch64_variadic_aggregate_overflow_byte_copy|backend_codegen_route_aarch64_alu_unpublished_load_local_(after_call|call_boundary)|c_testsuite_aarch64_backend_src_00164_c|c_testsuite_aarch64_backend_src_00176_c|c_testsuite_aarch64_backend_src_00181_c|c_testsuite_aarch64_backend_src_00204_c)$'`

## Watchouts

Do not special-case `00204`, `myprintf`, `%7s`, `%9s`, specific temporary names,
or string literal contents.

Do not reopen idea 56's edge preservation, scalar cast source consumption,
`memory.cpp` va_list cursor writeback, or `dispatch_edge_copies.cpp` edge
source selection unless a later packet proves the call route classification was
wrong.

Treat existing dirty `memory.cpp`, `dispatch_edge_copies.cpp`, and transient
`review/*` files as external context for this lifecycle switch.

## Proof

No validation was run by the plan-owner lifecycle switch. The latest executor
proof before this switch was the focused eight-test subset with build passing,
`7/8` tests passing, and only `c_testsuite_aarch64_backend_src_00204_c`
failing with `[RUNTIME_MISMATCH]`.
