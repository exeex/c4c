Status: Active
Source Idea Path: ideas/open/294_aarch64_pointer_derived_address_lvalue_lowering_authority.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Repair Core Pointer-Derived Lvalue Address Publication

# Current Packet

## Just Finished

Step 2 `00180.c` local subobject address-valued call argument publication is
complete. The AArch64 local array-base GEP path now publishes nonzero local
subobject addresses as real pointer `add` BIR values, and the same-block
call-argument scalar producer retargets its ALU operands to already-emitted
prepared registers so the call move consumes the concrete address instead of a
raw named-register fallback.

Generated evidence: `00180.c` semantic BIR now defines `%t5` before the
`printf` call as `%t5 = bir.add ptr %lv.a.0, 1`. Generated AArch64 now emits
`mov x20, sp`, then `add x13, x20, #1`, then moves the published address with
`mov x1, x13` before `bl printf`. The previous stale `ldr w1, [sp, #16]` and
uninitialized `x19` address source are gone.

## Suggested Next

Keep `00217.c` separated for the global/runtime pointer propagation packet.
The focused local subobject address-valued call argument case is now passing.

## Watchouts

- Do not repair this by naming the four tests or rewriting expectations. The
  address fact must become an authoritative value/materialization that ordinary
  load/store/call consumers can use.
- Do not reintroduce common semantic BIR pointer publications for x86; the
  regression subset is specifically guarding `%t0 = bir.load_local ptr %lv.p`
  and related load-local forms.
- `00130.c` now passes in the delegated subset; preserve the new byte-memory
  and emitted-byte fused-branch behavior when taking later pointer packets.
- `00180.c` now has the missing `%t5` address fact and passes in the delegated
  subset; preserve nonzero local array/subobject pointer publication through
  call arguments.
- Keep `00217.c` separated for a global/runtime pointer packet; it still needs
  `PointerAddress` propagation through load-compute-store and likely should not
  be widened into the local frame-slot patch.

## Proof

Ran the delegated proof:

```sh
{ cmake --build build-aarch64-scan --target c4cll && ctest --test-dir build-aarch64-scan -R 'c_testsuite_aarch64_backend_src_(00032|00130|00180|00217)_c$' -j 4 --timeout 5 --output-on-failure; } > test_after.log 2>&1
```

`test_after.log` records strict pass-count improvement over `test_before.log`:
`00032.c`, `00130.c`, and `00180.c` pass, while `00217.c` remains the separated
global/runtime pointer failure. The build portion passed, and the proof log
preserves the remaining `00217.c` runtime mismatch. `git diff --check` passed.
Stale-process check found no stale `ctest`, `c4cll`, `qemu`, or build/test
process beyond the check command itself.
