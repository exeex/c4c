# Current Packet

Status: Active
Source Idea Path: ideas/open/52_aarch64_calls_prepared_authority_repair.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Audit neighboring call authority fallbacks

## Just Finished

Step 4 classified the remaining `00204` runtime mismatch after the Step 3
single-GPR by-value lane facts are present and consumed.

The remaining owner is aggregate `va_arg` register-save consumption in
`src/backend/mir/aarch64/codegen/variadic.cpp`, not caller-side call argument
payload publication, overflow stack publication, or a missing
`PreparedCallArgumentSourceSelection` fact.

Concrete evidence:

- `stdarg` publishes the second string call as `x1` for the 7-byte aggregate
  lane and `x2/x3`, `x4/x5`, `x6/x7` for the following 9-byte lanes; the first
  9-byte call likewise publishes two-GPR lanes before overflow.
- Prepared `myprintf` facts are complete: register save area stack offset
  `1072`, `gp_register_save_area@8`, initial `gp_offset=-56`, and aggregate
  va_arg plans with `source_class=register_save_area`, `progression_field=gp_offset`,
  `progression_stride=8` for the 7-byte aggregate and `progression_stride=16`
  for the 9-byte aggregate.
- Emitted `myprintf` stores variadic GPRs to `[sp,#1072]..[sp,#1120]`, but
  the register-save path increments `gp_offset` before forming the source
  address. For the 7-byte aggregate it changes `-56 -> -48`, then loads from
  `gr_top - 48`, which is the next variadic GPR slot, so the `%7s` argument
  is read as the following 9-byte lane. The 9-byte path similarly uses the
  post-incremented offset after `+16`.

## Suggested Next

Delegate an implementation packet for
`src/backend/mir/aarch64/codegen/variadic.cpp` and focused tests. The packet
should make aggregate `va_arg` register-save lowering form the source address
from the pre-increment `gp_offset`/`fp_offset` value, then write the advanced
offset afterward. Preserve the existing overflow branch and HFA lane-home
behavior.

## Watchouts

Do not fix this by scanning for `00204`, format strings, temporary spellings,
or literal string contents. The semantic contract is the AAPCS64 va_list
register-save rule: the register-save source address uses the old negative
offset, while the va_list progression field receives the old offset plus the
prepared stride.

Treat existing dirty `memory.cpp`, `dispatch_edge_copies.cpp`, and transient
`review/*` files as external context.

## Proof

Proof command:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_codegen_route_aarch64_pointer_select_aggregate_byte_copy|backend_codegen_route_aarch64_variadic_aggregate_overflow_byte_copy|backend_codegen_route_aarch64_alu_unpublished_load_local_(after_call|call_boundary)|c_testsuite_aarch64_backend_src_00164_c|c_testsuite_aarch64_backend_src_00176_c|c_testsuite_aarch64_backend_src_00181_c|c_testsuite_aarch64_backend_src_00204_c)$'`

Result: build passed, focused subset `7/8` passed, and only
`c_testsuite_aarch64_backend_src_00204_c` failed with `[RUNTIME_MISMATCH]`.
Proof log path: `test_after.log`.
