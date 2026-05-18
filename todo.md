# AArch64 Stack Frame SP Alignment Todo

Status: Active
Source Idea Path: ideas/open/288_aarch64_stack_frame_sp_alignment.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Sample the Wider Bus-Error Cluster and Decide Closure

# Current Packet

## Just Finished

Completed Step 4, "Sample the Wider Bus-Error Cluster and Decide Closure".

Ran the delegated wider AArch64 backend c-testsuite sample covering 28 former
bus-error-cluster cases. The build was clean and CTest selected all 28 cases.
Result: 17 passed, 11 failed, 0 timed out.

Former bus-error cases now passing in this sample: `00004.c`, `00005.c`,
`00011.c`, `00013.c`, `00014.c`, `00015.c`, `00016.c`, `00017.c`,
`00020.c`, `00022.c`, `00026.c`, `00052.c`, `00053.c`, `00103.c`,
`00117.c`, `00118.c`, and `00153.c`.

Remaining failures are not showing the repaired SP/frame-alignment owner:

- Runtime nonzero, non-bus-error behavior to split or defer outside this idea:
  `00032.c` pointer increment/decrement over an array, `00042.c` union aliasing,
  `00058.c` adjacent string-literal indexing, `00111.c` mixed short/long
  arithmetic assignment, `00113.c` int/float comparison, `00137.c` macro
  stringification plus adjacent literal, and `00138.c` macro expansion to a
  string literal.
- Remaining bus-error cases to split or defer to function-pointer/global-init
  owners: `00087.c`, `00089.c`, `00124.c`, and `00210.c`. Their generated
  frames are aligned (`sub sp, sp, #32` or `sub sp, sp, #48`), and the crash
  shape is bad or missing function-pointer values before indirect `blr`, not
  an unaligned stack adjustment.

## Suggested Next

Supervisor should decide whether to close the SP/frame-alignment idea after
review and open follow-on work for the remaining function-pointer/global-init
bus errors plus the non-bus runtime semantic failures.

## Watchouts

- Do not change tests, expected outputs, runner behavior, CTest registration,
  allowlists, unsupported classifications, or timeout policy.
- Reject named-case shortcuts, exact frame-size shortcuts, local-variable
  spelling shortcuts, and emitted-instruction-text matching.
- Separate remaining bus-error cases that prove to be broader pointer, array,
  struct, function-pointer, or local-state owners.
- Keep timeout-sensitive cases `00132.c`, `00173.c`, and `00220.c` out of this
  route unless a separate timeout-specific idea is created.
- The AArch64 fix intentionally rounds only the frame boundary facts consumed
  by prologue/epilogue emission; it does not rewrite prepared stack-layout or
  frame-plan slot offsets.
- The whole sampled former bus-error cluster is not green, so closure should be
  based on source-idea exhaustion and explicit split/defer of non-alignment
  owners, not on a full-cluster pass claim.

## Proof

Ran the delegated Step 4 proof:

```sh
{ cmake --build build-aarch64-scan --target c4cll && ctest --test-dir build-aarch64-scan --output-on-failure --timeout 5 -R '^c_testsuite_aarch64_backend_src_00(004|005|011|013|014|015|016|017|020|022|026|032|042|052|053|058|087|089)_c$|^c_testsuite_aarch64_backend_src_001(03|11|13|17|18|24|37|38|53)_c$|^c_testsuite_aarch64_backend_src_00210_c$'; } > /tmp/c4c_sp_alignment_wider_bus_error_subset.log 2>&1
```

Result: build passed; CTest selected `28/28`; `17/28` passed, `11/28`
failed, `0/28` timed out.

Proof log: `/tmp/c4c_sp_alignment_wider_bus_error_subset.log`.
