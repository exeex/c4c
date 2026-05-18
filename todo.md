Status: Active
Source Idea Path: ideas/open/294_aarch64_pointer_derived_address_lvalue_lowering_authority.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Repair Core Pointer-Derived Lvalue Address Publication

# Current Packet

## Just Finished

Step 2 cross-route regression repair completed. x86 backend-route semantic BIR
now keeps the pre-`69c6b83b5` generic `bir.load_local ptr %lv.*` shape instead
of publishing local pointer loads as `bir.add ptr %lv.*, 0`. The AArch64
frame-slot publication path remains enabled only for AArch64 lowering, which
preserves the `00032.c` improvement without changing x86 semantic BIR.

Route boundary found: once the synthetic pointer SSA definitions are removed
from common semantic BIR, cases such as `00032.c` contain pointer names like
`%t5`/`%t7` only as stored values, with no BIR-side definition that prealloc can
soundly use to infer frame-slot provenance across CFG blocks. A linear
prealloc reconstruction would be route-unsafe, so this slice keeps generic x86
semantic BIR stable and limits the publication to AArch64 where the current
frame-slot materialization support consumes it.

## Suggested Next

Execute a narrow follow-up local subobject consumer packet for `00130.c` and
`00180.c`: carry the AArch64-published local frame/subobject pointer address
into scalar local subobject loads and address-valued call arguments, including
nonzero local byte offsets such as `sp + 1`.

## Watchouts

- Do not repair this by naming the four tests or rewriting expectations. The
  address fact must become an authoritative value/materialization that ordinary
  load/store/call consumers can use.
- Do not reintroduce common semantic BIR pointer publications for x86; the
  regression subset is specifically guarding `%t0 = bir.load_local ptr %lv.p`
  and related load-local forms.
- Prealloc cannot currently recover undefined pointer SSA provenance such as
  `%t5` in `00032.c` from BIR alone without a CFG-aware provenance fact. Avoid a
  linear scan reconstruction unless a later packet adds real dominance/CFG
  authority.
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
{ ctest --test-dir build -R 'backend_codegen_route_x86_64_(local_pointer_deref|variadic_double_bytes|inline_asm_(input_local_ptr|output_ptr|output_readwrite_ptr))_observe_semantic_bir' --output-on-failure && cmake --build build-aarch64-scan --target c4cll && ctest --test-dir build-aarch64-scan -R 'c_testsuite_aarch64_backend_src_(00032|00130|00180|00217)_c$' -j 4 --timeout 5 --output-on-failure; } > test_after.log 2>&1
```

`test_after.log` records that all five x86 semantic BIR regression tests pass
and `c_testsuite_aarch64_backend_src_00032_c` passes. The delegated command
still exits nonzero because `00130.c`, `00180.c`, and `00217.c` remain the
known follow-up failures. `git diff --check` passed. Stale-process check found
no stale `ctest`, `c4cll`, `qemu`, or build/test process beyond the check
command itself.
