Status: Active
Source Idea Path: ideas/open/325_variadic_target_ir_abi_boundary_triage.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Create Focused Follow-Up Ideas

# Current Packet

## Just Finished

Step 6: Create Focused Follow-Up Ideas completed by adding
`ideas/open/326_rv64_variadic_target_llvm_route_facts.md`. The new idea owns
the selected target-aware LLVM-route/LIR repair boundary for RV64 direct-call
`signext` facts and scalar stdarg pointer-cursor `va_list`/`va_arg` facts,
while keeping semantic BIR admission, prepared ABI publication, and RV64
target-emitter consumption as downstream work.

## Suggested Next

The current runbook is exhausted. Ask the plan owner to decide whether to close
the active triage idea, deactivate it with durable leftovers, or switch to the
new follow-up idea.

## Watchouts

Closure should verify that the active triage final acceptance criteria are met:
clang and c4c evidence exist, the first missing layer is identified, the four
boundary categories are separated, and the focused follow-up idea exists. Do
not close if the supervisor wants the ignored evidence artifacts preserved in a
different durable form first.

## Proof

Proof command:

```sh
bash -lc 'test -f ideas/open/326_rv64_variadic_target_llvm_route_facts.md && rg "target-aware LLVM-route|direct external variadic|signext|pointer-cursor|va_list|va_arg|Proof Strategy|Reviewer Reject Signals" ideas/open/326_rv64_variadic_target_llvm_route_facts.md && rg "AArch64.*constant|testcase-shaped|unsupported diagnostics|prepared ABI|emitter|target-correct RV64 LLVM-route" ideas/open/326_rv64_variadic_target_llvm_route_facts.md'
```

Result: passed. The lifecycle proof checks that the new open idea exists, owns
the selected target-aware LLVM-route/LIR boundary, includes a concrete proof
strategy, and contains reject signals for AArch64 constant copying,
testcase-shaped shortcuts, unsupported diagnostic weakening, and prepared or
emitter claims before target-correct RV64 LLVM-route facts. Proof log:
`test_after.log`.
