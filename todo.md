Status: Active
Source Idea Path: ideas/open/100_aarch64_00204_stdarg_hfa_runtime_repair.md
Source Plan Path: plan.md
Current Step ID: Step 5
Current Step Title: Direct >16-byte byval/address materialization

# Current Packet

## Just Finished

Completed Step 5 follow-up implementation for direct >16-byte AArch64
byval/address argument materialization.

Implemented facts:

- The existing 1..16 byte direct byval register-lane repair remains green. The
  focused route test
  `backend_codegen_route_aarch64_byval_global_payload_call_boundary` continues
  to prove final call-boundary GPR lane materialization from prepared global
  payload bytes.
- Direct >16-byte byval/address arguments now materialize the pointed-to
  outgoing object before the address is placed in the outgoing ABI GPR. The new
  focused route test
  `backend_codegen_route_aarch64_byval_global_payload_address_call_boundary`
  proves a compact `s1..s17` sequence where `s17` is selected as
  `frame_slot_address`; final AArch64 reloads bytes from global `s17`, stores
  them into the outgoing stack object, then emits `add x0, sp, #136` and
  `bl fa_s17`.
- Diagnostic final AArch64 for full `00204.c` now shows the previous
  `fa_s17(s17)` wrong fact repaired: after the stale scratch stores, the
  call-boundary path reloads `s17+0..16`, writes `sp+1064..1080`, then passes
  `add x0, sp, #1064` to `fa_s17`.

## Suggested Next

Trace/repair the next remaining `00204.c` ABI families after the direct integer
byval argument section. The current runtime output reaches and correctly prints
the direct `Arguments:` integer aggregate calls through `fa_s17`; the first
remaining visible mismatch is the direct HFA argument block printing `0.0`
instead of the expected HFA float/double/long-double values. Return/sret,
stdarg, and HFA-vararg output remains downstream/separate and should not be
folded into this direct byval/address packet.

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
- For `s17`, the stale stores are no longer the consumed pointee state: the
  final call-boundary materialization overwrites them from the prepared global
  payload bytes immediately before passing the address.

## Proof

Ran:

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_codegen_route_aarch64_byval_global_payload_address_call_boundary|backend_codegen_route_aarch64_byval_global_payload_call_boundary|backend_prepare_frame_stack_call_contract|c_testsuite_aarch64_backend_src_00204_c|c_testsuite_aarch64_backend_src_00032_c|c_testsuite_aarch64_backend_src_00182_c)$'
```

Result: exit code 8. Expected for this bounded packet: both focused byval
call-boundary route proofs, the existing prepared contract, and both guard
cases passed; `c_testsuite_aarch64_backend_src_00204_c` remains the only
failing test in this scope due later HFA/return/stdarg families. Canonical
executor proof log: `test_after.log`.
