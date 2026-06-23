Status: Active
Source Idea Path: ideas/open/325_variadic_target_ir_abi_boundary_triage.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Capture c4c LLVM-Route, Semantic BIR, and Prepared BIR Evidence

# Current Packet

## Just Finished

Step 3: Capture c4c LLVM-Route, Semantic BIR, and Prepared BIR Evidence
completed by generating c4cll LLVM-route, semantic BIR, and prepared BIR
stdout/stderr/status artifacts for both variadic triage fixtures under
`aarch64-linux-gnu` and `riscv64-linux-gnu`. The triage notes now compare those
c4c artifacts with clang target LLVM IR and identify the first c4c layer where
target-specific variadic ABI facts are missing, flattened, or blocked.

## Suggested Next

Begin Step 4 by inventorying the existing AArch64 variadic mechanisms named in
`plan.md`, classifying each mechanism as shared semantic/BIR concept, AArch64
ABI-specific record or constant, prepared ABI planning surface, or final target
emitter behavior. Keep the work evidence-only and avoid implementation edits.

## Watchouts

Direct `printf` reaches all c4c layers on both targets, but c4c LLVM-route IR
already omits clang's RV64 `signext` facts; semantic BIR is target-neutral for
that call, while prepared BIR preserves target register placement but still no
RV64 extension facts. The scalar `stdarg` LLVM-route output is visibly
flattened to the AArch64-shaped structured `va_list` algorithm on RV64; its
semantic BIR and prepared BIR dumps fail symmetrically at semantic
`lir_to_bir` load local-memory admission before callee-entry or `va_arg` facts
can be compared.

## Proof

Proof command:

```sh
bash -lc 'set -u; dir=build/variadic_abi_triage; cases="direct_variadic_printf stdarg_scalar_int"; targets="aarch64-linux-gnu riscv64-linux-gnu"; for case in $cases; do for target in $targets; do short=$target; short=${short/aarch64-linux-gnu/aarch64}; short=${short/riscv64-linux-gnu/rv64}; ./build/c4cll --codegen llvm --target "$target" "$dir/$case.c" -o "$dir/$case.c4c.$short.ll" > "$dir/$case.c4c.$short.llvm.out" 2> "$dir/$case.c4c.$short.llvm.err"; printf "%s\n" "$?" > "$dir/$case.c4c.$short.llvm.status"; ./build/c4cll --dump-bir --target "$target" "$dir/$case.c" > "$dir/$case.c4c.$short.bir.txt" 2> "$dir/$case.c4c.$short.bir.err"; printf "%s\n" "$?" > "$dir/$case.c4c.$short.bir.status"; ./build/c4cll --dump-prepared-bir --target "$target" "$dir/$case.c" > "$dir/$case.c4c.$short.prepared.txt" 2> "$dir/$case.c4c.$short.prepared.err"; printf "%s\n" "$?" > "$dir/$case.c4c.$short.prepared.status"; done; done; test -f "$dir/direct_variadic_printf.c4c.aarch64.llvm.status" && test -f "$dir/direct_variadic_printf.c4c.rv64.llvm.status" && test -f "$dir/stdarg_scalar_int.c4c.aarch64.bir.status" && test -f "$dir/stdarg_scalar_int.c4c.rv64.prepared.status" && rg "c4c|LLVM-route|semantic BIR|prepared BIR|first c4c layer|target-specific|fail|missing|flatten" "$dir/notes.md"'
```

Result: passed. The supervisor-selected proof is sufficient for this c4cll
evidence packet because the per-command status files preserve successful and
failed dump stages as boundary evidence. Proof log: `test_after.log`.
