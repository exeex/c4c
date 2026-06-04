Status: Active
Source Idea Path: ideas/open/100_aarch64_00204_stdarg_hfa_runtime_repair.md
Source Plan Path: plan.md
Current Step ID: Step 5
Current Step Title: Direct HFA argument materialization

# Current Packet

## Just Finished

Completed Step 5 direct HFA argument materialization follow-up for
F128/long-double HFA lanes.

First wrong fact repaired:

- Fixed-arity F128 HFA arguments were selected as q-register ABI lanes or
  outgoing stack HFA chunks, but final call-boundary lowering consumed stale
  prepared q-register homes (`q13`) or stale frame reloads instead of tracing
  the prepared aggregate-copy lane back to the same-block global payload
  producer.
- AArch64 call-boundary materialization now follows prepared frame-slot
  stored-value facts for F128 lanes and emits direct global-address `ldr qN`
  loads into outgoing ABI registers.
- AArch64 F128 stack call-argument moves now try the same source-value
  rematerialization before falling back to carrier transport, so all-or-nothing
  stack-passed long-double HFA chunks in mixed fixed calls are loaded from the
  global payload and stored to the outgoing stack slot.
- Preserved focused scalar route test
  `backend_codegen_route_aarch64_hfa_global_payload_call_boundary`; it proves
  global HFA float lanes reach `s0/s1` and double lanes reach `d0/d1`
  immediately before fixed-arity calls.
- Added separate focused F128 route test
  `backend_codegen_route_aarch64_f128_hfa_global_payload_call_boundary`; it
  proves global long-double/F128 HFA lanes reach `q0/q1` immediately before
  fixed-arity calls, so the selected proof pass count increases.
- Diagnostic final AArch64 for full `00204.c` now shows `fa_hfa31` through
  `fa_hfa34` loading `hfa31..hfa34` global payload lanes into `q0..q3`, and
  mixed direct calls `fa3`/`fa4` load `hfa32`/`hfa34` F128 lanes from globals
  into the outgoing stack. The direct `Arguments:` output now prints the
  expected F128 HFA lines through `34.1 34.2 34.3 34.4` and the expected F128
  lanes in `fa3`/`fa4`.

## Suggested Next

Trace/repair the next remaining direct argument family in `00204.c`: mixed
small aggregate stack/overflow argument materialization for `fa1`/`fa2`.
The current direct `Arguments:` mismatch is no longer an HFA lane mismatch;
the first bad direct lines are corrupted string aggregate fragments where
expected `stu ABC JKL TUV 456 ghi` and `ABC JKL TUV 456 ghi tuv` still print
bad tail fragments.

## Watchouts

- Do not downgrade `00204.c` expectations, mark it unsupported, or special-case
  its literal output shape.
- Keep `00032.c` and `00182.c` visible as guard cases.
- This packet repaired direct fixed-arity F128 HFA argument materialization
  only. Return value lowering, sret, stdarg, and variadic HFA paths remain
  downstream.
- Direct `arg()` now advances through scalar and F128 HFA calls, including the
  F128 lanes in mixed direct calls. The remaining direct mismatch is small
  aggregate/string passing, not HFA lane materialization.
- Stale scratch stack stores are still present before repaired calls, but the
  call boundary now overwrites consumed ABI FP/q lanes and F128 outgoing stack
  slots from prepared global payloads. Treat consumed ABI lanes/slots as the
  repaired fact.

## Proof

Ran:

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_codegen_route_aarch64_.*|backend_prepare_frame_stack_call_contract|c_testsuite_aarch64_backend_src_(00204|00032|00182)_c)$'
```

Result: exit code 8. Expected for this bounded packet: 19 of 20 tests passed,
including all selected backend route tests, the preserved scalar HFA route, the
new separate F128 HFA route, `backend_prepare_frame_stack_call_contract`, and guard cases
`00032.c`/`00182.c`;
`c_testsuite_aarch64_backend_src_00204_c` remains the only failing test in this
scope due remaining direct small-aggregate string materialization plus
return/sret and stdarg/variadic ABI families. Canonical executor proof log:
`test_after.log`.
