Status: Active
Source Idea Path: ideas/open/326_rv64_variadic_target_llvm_route_facts.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Locate Current RV64 Variadic Fact Boundaries

# Current Packet

## Just Finished

Lifecycle activation created `plan.md` and this executor-compatible `todo.md` for Step 1.

## Suggested Next

Execute Step 1: inspect the current LLVM-route/LIR boundaries for RV64 direct variadic integer-call extension facts and scalar `stdarg` pointer-cursor lowering. Record the selected code surfaces, observed current IR shapes, and the exact first implementation proof command here before changing implementation code.

## Watchouts

- Do not edit `ideas/open/326_rv64_variadic_target_llvm_route_facts.md` unless durable source intent changes.
- Do not weaken unsupported diagnostics or rewrite expectations as progress.
- Do not add `printf`-specific, fixture-shaped, or named-symbol shortcuts.
- Keep AArch64 structured `%struct.__va_list` behavior intact.
- Prefer target-owned LLVM-route/LIR facts before semantic BIR, prepared ABI, or final emission changes.

## Proof

Lifecycle-only activation; no build or code validation run.
