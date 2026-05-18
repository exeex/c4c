Status: Active
Source Idea Path: ideas/open/294_aarch64_pointer_derived_address_lvalue_lowering_authority.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Repair Core Pointer-Derived Lvalue Address Publication

# Current Packet

## Just Finished

Step 2 `00130.c` local i8 subobject load consumer is complete. The slice keeps
the byte-width frame-slot memory emission (`strb`/`ldrb`) and adds a prepared
fused-compare branch fallback for same-block sign-extension consumers of
already-emitted byte loads. If the sign-extension source was not emitted by the
ordinary memory path, the fallback reloads the prepared frame-slot byte from the
same prepared memory access before extending and branching.

Generated evidence: `00130.c` prepared addressing maps `%lv.arr.7` and
`%t32.addr, addr %lv.arr.4+3` to frame slot `#2` offset `0`, size `1`, align
`1`. Generated AArch64 now stores `arr[1][3]` with `strb w9, [sp, #2]`,
loads the direct and pointer-derived i8 checks with `ldrb`, sign-extends the
loaded byte (`sxtb`), and emits the prepared compare/branch instead of falling
through or consuming stale `w19`. For the `*q` path, assembly now includes
`ldrb w10, [sp, #2]`, `sxtb w21, w10`, `cmp w21, #2`, and the prepared branch
to the true/false labels.

## Suggested Next

Keep `00180.c` and `00217.c` as separate packets. `00180.c` still needs the
direct-call argument path to publish the nonzero local subobject address, and
`00217.c` still needs the global/runtime pointer update route.

## Watchouts

- Do not repair this by naming the four tests or rewriting expectations. The
  address fact must become an authoritative value/materialization that ordinary
  load/store/call consumers can use.
- Do not reintroduce common semantic BIR pointer publications for x86; the
  regression subset is specifically guarding `%t0 = bir.load_local ptr %lv.p`
  and related load-local forms.
- `00130.c` now passes in the delegated subset; preserve the new byte-memory
  and emitted-byte fused-branch behavior when taking later pointer packets.
- `00180.c` is blocked before AArch64 call-boundary consumption: `%t5` has no
  BIR definition, so prealloc/call lowering cannot distinguish it from a stale
  spill slot without an upstream call-argument address fact.
- Keep `00217.c` separated for a global/runtime pointer packet; it still needs
  `PointerAddress` propagation through load-compute-store and likely should not
  be widened into the local frame-slot patch.

## Proof

Ran the delegated proof:

```sh
{ cmake --build build-aarch64-scan --target c4cll && ctest --test-dir build-aarch64-scan -R 'c_testsuite_aarch64_backend_src_(00032|00130|00180|00217)_c$' -j 4 --timeout 5 --output-on-failure; } > test_after.log 2>&1
```

`test_after.log` records strict pass-count improvement over `test_before.log`:
`00032.c` and `00130.c` pass, while `00180.c` and `00217.c` still fail as
separate deferred packets. The build portion passed, and the proof log
preserves the failing subset details. `git diff --check` passed.
Stale-process check found no stale `ctest`, `c4cll`, `qemu`, or build/test
process beyond the check command itself.
