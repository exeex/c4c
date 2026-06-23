Status: Active
Source Idea Path: ideas/open/325_variadic_target_ir_abi_boundary_triage.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Inventory Existing AArch64 Variadic Mechanisms

# Current Packet

## Just Finished

Step 4: Inventory Existing AArch64 Variadic Mechanisms completed by reading
the listed AArch64 variadic closed ideas, tracing the named helper/record/test
surfaces with targeted text search, adding
`build/variadic_abi_triage/aarch64_variadic_inventory.md`, and appending the
Step 4 classification to `build/variadic_abi_triage/notes.md`. The inventory
separates shared semantic/BIR concepts, AArch64 ABI-specific records and
constants, prepared ABI planning surfaces, and final target emitter behavior,
then states which surfaces RV64 can consume analogously versus what must be
newly modeled from RV64 ABI evidence.

## Suggested Next

Begin Step 5 by deciding the next repair boundary from the collected evidence.
The likely split to evaluate is direct-call RV64 `signext` facts versus scalar
stdarg pointer-cursor `va_list`/`va_arg` facts; keep the decision narrow enough
to become one follow-up implementation idea.

## Watchouts

Do not copy AAPCS64 constants into RV64. The reusable surface is the staged
authority pattern and prepared publication machinery; the target facts that
must be newly modeled are RV64 direct-call integer extension attributes and the
RV64 scalar stdarg pointer-cursor representation observed in clang IR.

## Proof

Proof command:

```sh
bash -lc 'test -f ideas/closed/73_aarch64_variadic_prepared_entry_plan_cleanup.md && test -f ideas/closed/97_bir_prealloc_call_abi_authority_boundary_audit.md && test -f ideas/closed/285_aarch64_llvm_path_fp128_vararg_codegen_crash_triage.md && test -f ideas/closed/286_aarch64_00204_stdarg_semantic_bir_local_memory_admission.md && test -f ideas/closed/292_reopen_286_288_match_load_local_memory_admission.md && rg "shared semantic|AArch64 ABI-specific|prepared ABI planning|final target emitter|RV64 can consume|must be newly modeled|73_aarch64|97_bir|285_aarch64|286_aarch64|292_reopen" build/variadic_abi_triage/notes.md'
```

Result: passed. The supervisor-selected proof is sufficient for this
evidence-inventory packet because it checks all required closed idea sources
and verifies the Step 4 classification terms in the triage notes. Proof log:
`test_after.log`.
