Status: Active
Source Idea Path: ideas/open/56_aarch64_edge_terminator_consumer_preservation_repair.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Re-establish the Edge-Preservation Failure Boundary

# Current Packet

## Just Finished

Step 1 re-established the focused edge-preservation boundary after idea 52
closure. The delegated eight-test subset now passes seven tests and fails only
`c_testsuite_aarch64_backend_src_00204_c`.

The first bad fact in the remaining `00204` failure is before the
`stdarg`/`myprintf` edge-preservation path: in `ret()`, the first mismatched
line is `fr_hfa12()` returning/printing `2.0 12.1` instead of `12.1 12.2`.
The generated callee `fr_hfa12` moves the two float lanes to ABI result
registers `s0` and `s1`, but the caller side after `bl fr_hfa12` stores
`s9`/`s13` and then converts those stale registers for `printf`. That classifies
the current first bad fact as a non-owned AArch64 HFA aggregate return-result
publication/consumption issue, not idea 56 edge/terminator consumer
preservation.

## Suggested Next

Supervisor should route a precise non-owned follow-up for AArch64 HFA aggregate
return-result consumption in callers before any idea 56 implementation packet.
If the supervisor still wants to judge later `stdarg`/`myprintf` evidence, rerun
the focused subset after the HFA return-result first bad fact is repaired.

## Watchouts

- Do not start the idea 56 edge/terminator preservation implementation from the
  current `00204` failure: the live first bad fact is caller-side HFA return ABI
  consumption in straight-line `ret()`, not an edge copy, predecessor
  terminator, or join-block consumer.
- Do not treat the historical `%t35`, `%t45`, `%t49`, `vaarg.join.39`, or
  `x13` names as implementation selectors.
- Do not reload mutated va_list locals in the join block.
- Do not weaken expectations or mark supported probes unsupported.
- Keep any implementation packet within the source idea's prepared
  edge/terminator preservation scope.

## Proof

`test_after.log` contains:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_codegen_route_aarch64_pointer_select_aggregate_byte_copy|backend_codegen_route_aarch64_variadic_aggregate_overflow_byte_copy|backend_codegen_route_aarch64_alu_unpublished_load_local_(after_call|call_boundary)|c_testsuite_aarch64_backend_src_00164_c|c_testsuite_aarch64_backend_src_00176_c|c_testsuite_aarch64_backend_src_00181_c|c_testsuite_aarch64_backend_src_00204_c)$'
```

Result: build succeeded; 7/8 selected tests passed. The only failure is
`c_testsuite_aarch64_backend_src_00204_c` with a runtime mismatch, classified
above as non-owned by idea 56.
