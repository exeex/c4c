Status: Active
Source Idea Path: ideas/open/358_rv64_object_route_abi_width_edges.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Route Simple Call and FPR ABI Edges

# Current Packet

## Just Finished

Completed Plan Step 4 RV64 object-route boundary classification for
`src/20030125-1.c` after the FPR formal cast repair.

Fresh representative proof moved past the former `unsupported_floating_cast`
edge and now stops at:

```text
error: --codegen obj failed: RISC-V backend object route unsupported
prepared module shape: unsupported_instruction_fragment: BIR instruction
requires unsupported RV64 object lowering
```

The first identifiable unsupported prepared instruction is in `@t`, entry block
instruction index 1:

```text
%t1 = bir.call double sin(double %t0)
```

Prepared context for that call is `wrapper_kind=same_module`, callee `sin`,
one FPR argument from prepared register `ft0` to ABI register `fa0`, and an FPR
result from ABI register `fa0` to prepared callee-saved register `fs1`.
This classifies the next boundary as RV64 object-route direct-call lowering for
prepared FPR call arguments/results, not the preceding `fpext` cast.

## Suggested Next

Extend RV64 object-route direct-call lowering to handle prepared FPR
register-to-register call arguments and FPR register results for simple
same-module/direct fixed-arity calls, then rerun the same `src/20030125-1.c`
representative to classify the next boundary.

## Watchouts

- Treat `src/20030125-1.c` as a representative only. The semantic class is
  prepared FPR cast/call ABI lowering, not testcase-specific math folding.
- Admission is intentionally identity-backed for FPR homes. Do not add raw
  `register_name` fallback parsing for FPR spellings in object emission.
- The current object-route direct-call fragment already admits
  `SameModule`/`DirectExternFixedArity`, but its argument/result checks require
  GPR banks. The first rejected call needs FPR `ft0 -> fa0`, call relocation,
  and FPR result publication `fa0 -> fs1`.

## Proof

```sh
tmp=$(mktemp); printf 'src/20030125-1.c\n' > "$tmp"; ALLOWLIST="$tmp" CASE_TIMEOUT_SEC=20 STOP_ON_FAILURE=1 scripts/check_progress_rv64_gcc_c_torture_backend.sh > test_after.log; rc=$?; rm -f "$tmp"; exit $rc
```

Result: failed at the expected current boundary for classification. Proof log:
`test_after.log`; detailed harness log:
`build/rv64_gcc_c_torture_backend/src_20030125-1.c/case.log`.
