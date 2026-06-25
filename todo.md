Status: Active
Source Idea Path: ideas/open/358_rv64_object_route_abi_width_edges.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Route Simple Call and FPR ABI Edges

# Current Packet

## Just Finished

Ran Plan Step 4 representative validation for `src/20030125-1.c` after prepared
RV64 FPR `bir.fptrunc double -> float` lowering.

The representative still exits nonzero during RV64 object compilation. The old
single-pair floating-cast diagnostic text is gone, but the case now stops at the
expanded structured floating-cast boundary:

```text
unsupported_floating_cast: RV64 object route supports only prepared F32-to-F64 and F64-to-F32 FPR register casts
```

The focused BIR/prepared-BIR dump for `@t` confirms the visible source pattern
is still FPR cast/call ABI work:

```text
%t0 = bir.fpext float %p.a to double
%t1 = bir.call double sin(double %t0)
%t2 = bir.fptrunc double %t1 to float
```

## Suggested Next

Classify the remaining `unsupported_floating_cast` site in the RV64 object route
and either add a sharper diagnostic for the failing prepared-home/ABI shape or
lower the next semantic FPR ABI edge if the facts are complete.

## Watchouts

- Treat `src/20030125-1.c` as a representative only. The semantic class is
  prepared FPR cast/call ABI lowering, not testcase-specific math folding.
- The FPR cast lowerer requires `PreparedTargetRegisterIdentity` to identify
  RV64 FPR physical registers; it does not infer FPR numbers from names alone.
- The representative BIR only shows the supported `fpext`/`fptrunc` type pairs,
  so the next packet should inspect the exact failing value home or function
  before adding more type-pair support.
- Floating calls, libm behavior, and stack-slot FPR materialization remain
  outside this validation-only packet.

## Proof

Delegated proof:

```sh
tmp=$(mktemp); printf 'src/20030125-1.c\n' > "$tmp"; ALLOWLIST="$tmp" CASE_TIMEOUT_SEC=20 STOP_ON_FAILURE=1 scripts/check_progress_rv64_gcc_c_torture_backend.sh > test_after.log; rc=$?; rm -f "$tmp"; exit $rc
```

Result: failed with exit code 1 at a later structured boundary in the same
floating-cast family.

Proof logs:
- `test_after.log`
- `build/rv64_gcc_c_torture_backend/src_20030125-1.c/case.log`
