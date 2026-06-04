Status: Active
Source Idea Path: ideas/open/100_aarch64_00204_stdarg_hfa_runtime_repair.md
Source Plan Path: plan.md
Current Step ID: Step 5
Current Step Title: Direct HFA argument materialization

# Current Packet

## Just Finished

Completed Step 5 direct HFA argument materialization follow-up for scalar FP
HFA lanes.

First wrong fact repaired:

- Fixed-arity HFA float/double arguments were selected as FPR ABI lanes, but
  final call-boundary lowering consumed stale prepared FPR homes (`s13`/`d13`)
  or stale frame reloads instead of tracing the prepared aggregate-copy lane
  back to the same-block global payload producer.
- AArch64 call-boundary materialization now follows prepared frame-slot
  stored-value facts for scalar FP lanes and emits direct global-address FP
  loads into the outgoing ABI registers.
- Added focused route test
  `backend_codegen_route_aarch64_hfa_global_payload_call_boundary`; it proves
  global HFA float lanes reach `s0/s1` and global HFA double lanes reach
  `d0/d1` immediately before fixed-arity calls.
- Diagnostic final AArch64 for full `00204.c` now shows `fa_hfa11` through
  `fa_hfa24` loading `hfa11..hfa24` global payload lanes into `s0..s3` and
  `d0..d3`. The direct `Arguments:` output now prints the expected float and
  double HFA direct-call lines through `24.1 24.2 24.3 24.4`.

## Suggested Next

Trace/repair the next direct HFA argument family: F128/long-double HFA lanes.
The current first remaining direct HFA mismatch is `fa_hfa31` through
`fa_hfa34`, where final AArch64 still stores/reloads stale `q13` and passes
zero long-double values. Mixed direct calls `fa3`/`fa4` also still lose their
F128 HFA lanes after the scalar float/double lanes are now materialized.

## Watchouts

- Do not downgrade `00204.c` expectations, mark it unsupported, or special-case
  its literal output shape.
- Keep `00032.c` and `00182.c` visible as guard cases.
- The scalar FP repair is intentionally limited to fixed-arity before-call FPR
  call-argument moves. It does not repair F128/q-register HFA lanes, return
  value lowering, sret, stdarg, or variadic HFA paths.
- Direct `arg()` now advances through the scalar HFA float/double calls, but
  the later `stdarg:` HFA float/double output is still bad and should remain
  downstream until direct F128 HFA arguments and return-family facts are
  handled.
- Stale scratch stack stores are still present before the repaired scalar HFA
  calls, but the call boundary now overwrites the consumed ABI FP lanes from
  prepared global payloads. Treat only consumed ABI lanes as the failure fact.

## Proof

Ran:

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_codegen_route_aarch64_.*|backend_prepare_frame_stack_call_contract|c_testsuite_aarch64_backend_src_(00204|00032|00182)_c)$'
```

Result: exit code 8. Expected for this bounded packet: 18 of 19 tests passed,
including all selected backend route tests, the new HFA scalar FP route,
`backend_prepare_frame_stack_call_contract`, and guard cases `00032.c`/`00182.c`;
`c_testsuite_aarch64_backend_src_00204_c` remains the only failing test in this
scope due remaining direct F128 HFA, return/sret, and stdarg/variadic ABI
families. Canonical executor proof log: `test_after.log`.
