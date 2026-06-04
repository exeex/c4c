Status: Active
Source Idea Path: ideas/open/100_aarch64_00204_stdarg_hfa_runtime_repair.md
Source Plan Path: plan.md
Current Step ID: Step 5
Current Step Title: Direct >16-byte byval/address materialization

# Current Packet

## Just Finished

Completed Step 5 follow-up trace for direct >16-byte AArch64 byval/address
argument materialization in `00204.c` after commit `6e0b504e5`.

Last known-good facts:

- The landed 1..16 byte direct byval register-lane repair is still visible in
  final AArch64 for `arg`: calls through `fa_s16` load bytes from globals such
  as `s16`, pack them into outgoing ABI GPR lanes `x0`/`x1`, and branch to the
  helper. The focused route test
  `backend_codegen_route_aarch64_byval_global_payload_call_boundary` continues
  to prove the same final call-boundary fact without depending on full
  `00204.c` output.
- Semantic BIR for the next call is correct:
  `bir.call void fa_s17(ptr byval(size=17, align=1) %t18)`, preceded by
  `%t18.global.aggregate.load.0..16 = bir.load_local i8 ..., addr s17+N` and
  corresponding stores to `%t18.0..16`.
- Prepared BIR still carries useful payload metadata for the `s17` local
  aggregate object: prepared-addressing records stores of
  `%t18.global.aggregate.load.0..16` into consecutive frame slots
  `#903..#919`, and the prepared call plan for `fa_s17` publishes
  `arg index=0 value_bank=gpr source_encoding=frame_slot source_value_id=990
  source_stack_offset=1344 dest_reg=x0` with
  `arg.source_selection=frame_slot_address`.

First wrong fact:

- Final AArch64 for the `fa_s17(s17)` caller materializes the address argument
  but not the pointed-to payload. Immediately after the good `fa_s16` call, the
  emitted code stores stale `w13` into `sp+1064..1080`, then emits
  `add x0, sp, #1064` and `bl fa_s17`. There are no `s17` global byte loads or
  payload-copy stores feeding that 17-byte object before the call.
- This is a different failure from the repaired 1..16 register-lane case: the
  outgoing ABI register `x0` is an address for byval-indirect passing, and the
  pointee object must be materialized from the prepared aggregate payload bytes
  before the call boundary consumes the address.

## Suggested Next

Repair direct >16-byte byval/address call argument materialization in
`src/backend/mir/aarch64/codegen/calls.cpp`, likely around the address path
rooted at `materialize_local_aggregate_address_call_argument` and
`materialize_missing_frame_slot_call_arguments`. The repair should use prepared
addressing / producer facts to populate the addressable outgoing byval object
from the aggregate payload bytes before passing its address in `x0`.

## Watchouts

- Do not downgrade `00204.c` expectations, mark it unsupported, or special-case
  its literal output shape.
- Keep `00032.c` and `00182.c` visible as guard cases.
- Do not start with HFA or return/sret repair for this packet. Direct HFA
  argument materialization is a separate FPR/HFA lane problem that starts only
  after `fa_s17`; direct aggregate/sret return materialization is callee-side
  return lowering, not the caller address-argument setup traced here.
- Do not start with stdarg/HFA-vararg repair: those sections execute after the
  direct `arg` and `ret` families, and remain downstream until direct argument
  and return paths are correct.
- The fixed direct integer byval register-lane calls still leave stale stack
  publications in the assembly, but the call boundary no longer consumes them.
  Do not treat those stale stores as proof of failure unless a later consumer
  still reloads them into the ABI argument registers.
- For `s17`, the stale stores are consumed: the callee receives a pointer to
  the stale object. That makes this a real address/pointee materialization bug,
  not just harmless leftover stack publication.

## Proof

Ran:

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_codegen_route_aarch64_byval_global_payload_call_boundary|backend_prepare_frame_stack_call_contract|c_testsuite_aarch64_backend_src_00204_c|c_testsuite_aarch64_backend_src_00032_c|c_testsuite_aarch64_backend_src_00182_c)$'
```

Result: exit code 8. Expected for this trace-only packet: the focused byval
call-boundary route proof, the existing prepared contract, and both guard
cases passed; `c_testsuite_aarch64_backend_src_00204_c` remains the only
failing test in this scope. Canonical executor proof log: `test_after.log`.
