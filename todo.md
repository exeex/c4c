Status: Active
Source Idea Path: ideas/open/294_aarch64_pointer_derived_address_lvalue_lowering_authority.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Repair Core Pointer-Derived Lvalue Address Publication

# Current Packet

## Just Finished

Step 2 first local-address publication repair completed. Local pointer slot
loads/stores now publish local frame-slot and local subobject addresses as
pointer-valued BIR definitions, and prepared/AArch64 lowering can materialize
frame-slot pointer values with `sp`-relative address instructions.

Generated-assembly evidence:

- `00032.c` now passes. The old out-of-frame pointer reloads such as
  `[sp, #64]`, `[sp, #72]`, and stale pointer stores are gone from the passing
  generated assembly. Local pointer publications use `mov xN, sp`/`add xN, sp,
  #offset` before storing or consuming the pointer local.
- `00130.c` partially improves: `p = arr` and `q = &arr[1][3]` now publish
  `sp`-relative local addresses (`mov x20, sp`, `add x13, sp, #1`) instead of
  leaving those pointer values wholly unmaterialized. It still fails because a
  later local subobject value consumer uses stale `w19` after the address is
  materialized, so the next local packet needs to carry the subobject address
  into the scalar load/value consumer rather than stopping at pointer storage.
- `00180.c` still materializes the base local address for the first call
  (`mov x20, sp` before `strcpy`) but `printf("%s", &a[1])` still reloads from
  the saved-register spill area (`ldr w1, [sp, #16]`) instead of passing
  `sp + 1`. This needs a follow-up call-argument/subobject publication packet.
- `00217.c` remains a global/runtime pointer case: `data` is still loaded from
  a corrupted local pointer slot before `printf`, and the compound update still
  lowers directly to `t+4` instead of preserving the runtime pointer-derived
  address through load-compute-store.

## Suggested Next

Execute a narrow follow-up local subobject consumer packet for `00130.c` and
`00180.c`: carry the newly published local frame/subobject pointer address into
scalar local subobject loads and address-valued call arguments, including
nonzero local byte offsets such as `sp + 1`.

## Watchouts

- Do not repair this by naming the four tests or rewriting expectations. The
  address fact must become an authoritative value/materialization that ordinary
  load/store/call consumers can use.
- `00130.c` proves the first publication layer is not enough: the address is
  now materialized, but a scalar subobject load still consumes a stale register.
- `00180.c` needs the call-argument route to publish local subobject address
  values with nonzero byte offsets; the base `a` argument is improved, but
  `&a[1]` is not.
- Keep `00217.c` separated for a global/runtime pointer packet; it still needs
  `PointerAddress` propagation through load-compute-store and likely should not
  be widened into the local frame-slot patch.

## Proof

Ran the delegated proof:

```sh
{ cmake --build build-aarch64-scan --target c4cll && ctest --test-dir build-aarch64-scan -R 'c_testsuite_aarch64_backend_src_(00032|00130|00180|00217)_c$' -j 4 --timeout 5 --output-on-failure; } > test_after.log 2>&1
```

`test_after.log` shows strict pass-count improvement over `test_before.log`:
`00032.c` now passes; `00130.c`, `00180.c`, and `00217.c` still fail as
described above. `git diff --check` passed. Stale-process check found no stale
`ctest`, `c4cll`, `qemu`, or build/test process beyond the check command
itself.
