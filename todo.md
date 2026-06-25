Status: Active
Source Idea Path: ideas/open/358_rv64_object_route_abi_width_edges.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Route Simple Call and FPR ABI Edges

# Current Packet

## Just Finished

Completed Plan Step 4 classification rerun for RV64 object-route representative
`src/20030125-1.c` after FPR formal parameter-home admission.

The case still fails, but it has moved past parameter-home admission. The first
reported diagnostic is:

```text
error: --codegen obj failed: RISC-V backend object route unsupported
prepared module shape: unsupported_floating_cast: RV64 object route
supports only prepared F32-to-F64 and F64-to-F32 FPR register casts
```

Prepared context from `--dump-prepared-bir --target riscv64-linux-gnu` shows the
next boundary in `@t` at entry instruction 0:

```text
%t0 = bir.fpext float %p.a to double
storage %p.a register bank=fpr reg=a0
storage %t0 register bank=fpr placement=fpr:caller_saved#0/w1 reg=ft0
move before instruction 0: %p.a -> %t0 placement=fpr:caller_saved#0/w1
```

`@q` has the same F32-to-F64 call-argument cast shape for `floor`, and `@t`/`@q`
also contain the already intended F64-to-F32 return truncation after the call.

## Suggested Next

Teach the RV64 object route to admit and encode the prepared FPR register
floating-cast path where the source is an identity-backed FPR formal home and
the destination is a prepared FPR register value, then rerun
`src/20030125-1.c` to expose the next call/result boundary.

## Watchouts

- Treat `src/20030125-1.c` as a representative only. The semantic class is
  prepared FPR cast/call ABI lowering, not testcase-specific math folding.
- Admission is intentionally identity-backed for FPR homes. Do not add raw
  `register_name` fallback parsing for FPR spellings in object emission.
- Do not broaden the cast type matrix. The representative still only needs
  `FPExt F32 -> F64` from formal/caller-saved FPR homes and already-covered
  `FPTrunc F64 -> F32`.
- Floating calls, libm behavior, and stack-slot FPR materialization remain
  outside this classification packet. A separate `--codegen asm` observation
  reaches `unsupported_external_call function='floor' callee='abort'`, so the
  object-route cast rejection is the earlier object-specific boundary.

## Proof

```sh
tmp=$(mktemp); printf 'src/20030125-1.c\n' > "$tmp"; ALLOWLIST="$tmp" CASE_TIMEOUT_SEC=20 STOP_ON_FAILURE=1 scripts/check_progress_rv64_gcc_c_torture_backend.sh > test_after.log; rc=$?; rm -f "$tmp"; exit $rc
```

Result: failed as expected for classification. Proof log: `test_after.log`.
Per-case log:
`build/rv64_gcc_c_torture_backend/src_20030125-1.c/case.log`.
