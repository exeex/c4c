Status: Active
Source Idea Path: ideas/open/325_variadic_target_ir_abi_boundary_triage.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Decide the Next Repair Boundary

# Current Packet

## Just Finished

Step 5: Decide the Next Repair Boundary completed by appending the evidence
decision to `build/variadic_abi_triage/notes.md`. The notes choose a narrow
target-aware LLVM-route/LIR fact-production follow-up for RV64 direct-call
`signext` and scalar stdarg pointer-cursor `va_list`/`va_arg` facts, while
separating later semantic BIR admission, prepared ABI publication, and RV64
target-emitter consumption work.

## Suggested Next

Begin Step 6 by creating a focused follow-up idea under `ideas/open/` for the
selected target-aware LLVM-route/LIR repair boundary. Keep the idea narrow: it
should prove RV64 direct external variadic call `signext` facts and scalar
stdarg pointer-cursor facts with the existing tiny fixtures before any broad
variadic c-testsuite registration.

## Watchouts

Do not let the follow-up drift into prepared ABI or target-emitter work before
the RV64 target route owns the facts. Reject AArch64 offset/save-area constant
copying, testcase-shaped `printf` shortcuts, and any claim that semantic BIR or
prepared ABI can recover RV64 `signext` or pointer-cursor facts after
LLVM-route flattening.

## Proof

Proof command:

```sh
bash -lc 'rg "Step 3 boundary conclusion|Step 4 Conclusion|direct variadic|external variadic call ABI|variadic callee entry setup|va_list layout|va_arg extraction" build/variadic_abi_triage/notes.md && rg "next repair boundary|target-aware LLVM-route|semantic BIR|prepared ABI|RV64 signext|pointer-cursor|proof commands|follow-up idea" build/variadic_abi_triage/notes.md'
```

Result: passed. The supervisor-selected proof is sufficient for this
evidence-decision packet because it checks the Step 3 and Step 4 evidence
anchors, the four required boundary categories, the selected next repair
boundary, and the follow-up proof terms in the triage notes. Proof log:
`test_after.log`.
