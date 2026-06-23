# Variadic Target IR ABI Boundary Triage

## Goal

Map where variadic calls and `stdarg` lowering should diverge between targets
before attempting RV64 variadic runtime support.

This is a triage/design idea. It should compare clang's target-specific LLVM IR
for AArch64 and RV64, inspect c4c's LIR and LIR-to-LLVM-IR behavior, and decide
which facts must be target-dependent before BIR/prepared lowering versus which
facts can remain shared.

## Why This Exists

The RV64 c-testsuite runtime work currently fails closed for many external
variadic calls, especially `printf`, with diagnostics such as:

```text
riscv prepared module emitter unsupported_external_call ... callee='printf'
wrapper_kind=direct_extern_variadic reason=variadic external calls
```

That fail-closed policy is healthy. Variadic support is not just another RV64
emitter patch:

- AArch64 and RV64 variadic ABIs differ.
- `va_list` layout, register-save areas, argument classification, and
  `va_start`/`va_arg` mechanics are target ABI facts.
- clang may emit different LLVM IR for the same C source depending on
  `--target`.
- c4c's LIR dump and LIR-to-LLVM-IR path may need to be target-dependent before
  semantic LIR-to-BIR handoff.
- If prepared BIR does not carry enough target ABI metadata, implementing
  variadic behavior inside `riscv/codegen` would become a fragile guess.

Existing AArch64 history is relevant:

- `ideas/closed/73_aarch64_variadic_prepared_entry_plan_cleanup.md`
- `ideas/closed/97_bir_prealloc_call_abi_authority_boundary_audit.md`
- `ideas/closed/285_aarch64_llvm_path_fp128_vararg_codegen_crash_triage.md`
- `ideas/closed/286_aarch64_00204_stdarg_semantic_bir_local_memory_admission.md`
- `ideas/closed/292_reopen_286_288_match_load_local_memory_admission.md`

This triage should decide whether the right next repair is frontend/LIR target
IR production, semantic BIR facts, prealloc/prepared ABI records, or backend
target emission.

## Candidate Inputs

Use small C fixtures rather than the full c-testsuite sweep:

```c
int printf(const char *, ...);
int main(void) { return printf("%d\n", 7); }
```

```c
#include <stdarg.h>
int sum(int n, ...) {
  va_list ap;
  va_start(ap, n);
  int x = va_arg(ap, int);
  va_end(ap);
  return x;
}
int main(void) { return sum(1, 7) - 7; }
```

If useful, also include one aggregate or floating `va_arg` case only as
evidence, not as a required repair target.

Compare against clang output for both targets:

```sh
clang --target=aarch64-linux-gnu -S -emit-llvm <case.c> -o <case.aarch64.ll>
clang --target=riscv64-linux-gnu -S -emit-llvm <case.c> -o <case.rv64.ll>
```

Then compare c4c's relevant dumps for the same cases:

```sh
./build/c4cll --dump-lir --target aarch64-linux-gnu <case.c>
./build/c4cll --dump-lir --target riscv64-linux-gnu <case.c>
./build/c4cll --dump-bir --target aarch64-linux-gnu <case.c>
./build/c4cll --dump-bir --target riscv64-linux-gnu <case.c>
./build/c4cll --dump-prepared-bir --target aarch64-linux-gnu <case.c>
./build/c4cll --dump-prepared-bir --target riscv64-linux-gnu <case.c>
```

Adjust exact flags to the current supported debug surface if needed; record the
actual commands used.

## In Scope

- Compare clang LLVM IR for AArch64 versus RV64 on direct variadic calls and
  `va_start`/`va_arg` cases.
- Compare c4c LIR, LIR-to-LLVM-IR, BIR, and prepared-BIR facts for the same
  target pairs.
- Identify the first layer where c4c currently loses or flattens
  target-specific variadic ABI facts.
- Distinguish direct external variadic calls such as `printf` from true
  `stdarg`/`va_list` lowering inside a variadic callee.
- Inventory the AArch64 variadic mechanisms that already exist and determine
  whether RV64 should consume analogous shared records or requires new target
  records.
- Decide which facts belong to:
  - target-aware LIR / LLVM IR production;
  - semantic LIR-to-BIR;
  - prealloc or prepared ABI planning;
  - final target-specific code emission.
- Write follow-up ideas under `ideas/open/` for the next implementation slices.

## Out Of Scope

- Implementing RV64 `printf`, variadic external calls, or `va_arg` runtime
  support in this triage idea.
- Treating all external variadic calls as supported just because a direct call
  instruction can be emitted.
- Adding fake libc stubs, fake `printf` bodies, or c-testsuite-specific symbol
  shortcuts.
- Copying AArch64 variadic constants into RV64 without proving the ABI
  boundary.
- Weakening the current RV64 fail-closed diagnostics for unsupported variadic
  external calls.
- Registering the full variadic c-testsuite bucket as required CTest coverage.

## Suggested Execution Order

1. Pick two or three tiny C fixtures:
   - direct external variadic call;
   - scalar `va_start`/`va_arg`;
   - optional aggregate or floating `va_arg` evidence case.
2. Generate clang LLVM IR for AArch64 and RV64. Record concrete differences in
   function signatures, `va_list` representation, intrinsics, loads/stores, and
   target datalayout.
3. Generate c4c LIR, BIR, and prepared-BIR dumps for both targets. If a dump
   command is unsupported, document the nearest available debug flag and the
   limitation.
4. Compare clang IR against c4c's LIR-to-LLVM-IR path:
   - identify whether c4c currently emits target-neutral IR where clang is
     target-specific;
   - identify whether c4c's LIR already records enough target ABI information.
5. Trace how existing AArch64 variadic support consumes target facts through
   prealloc/prepared records.
6. Decide the next repair boundary:
   - LIR/LLVM target-dependent dump production;
   - semantic BIR variadic records;
   - shared prepared ABI records;
   - RV64 target emitter consumption.
7. Create focused follow-up ideas. Prefer at least:
   - one design/implementation idea for target-dependent variadic IR/LIR facts
     if clang comparison proves the divergence starts before BIR;
   - one RV64-specific idea only after the required target ABI records are
     known.

## Acceptance Criteria

- The triage records clang AArch64 and RV64 LLVM IR evidence for at least one
  direct variadic call and one `va_start`/`va_arg` case.
- The triage records c4c LIR/BIR/prepared-BIR evidence for the same cases, or
  explicitly documents which dump stage is unavailable.
- The writeup identifies the first c4c layer where target-specific variadic ABI
  facts must exist.
- The writeup separates:
  - external variadic call ABI;
  - variadic callee entry setup;
  - `va_list` layout;
  - `va_arg` extraction.
- At least one follow-up idea is created under `ideas/open/` with a concrete
  implementation boundary and proof strategy.
- Existing RV64 unsupported variadic diagnostics remain fail-closed unless a
  later implementation idea proves real support.
- Worktree state is clean or contains only committed lifecycle/idea artifacts
  when this triage closes.

## Reviewer Reject Signals

- The triage skips clang target comparison and assumes AArch64 and RV64 can
  share the same variadic IR shape.
- The result proposes an RV64 emitter-only patch while c4c lacks target ABI
  facts before prepared emission.
- It treats `printf` direct-call emission as equivalent to full variadic ABI
  support.
- It copies AArch64 `va_list` or register-save constants into RV64 without
  ABI evidence.
- It claims progress by replacing fail-closed diagnostics with runnable but
  semantically fake external stubs.
- It creates one giant "support variadic" implementation idea instead of
  separating IR boundary, ABI records, and target emission.

## Notes For The Agent

- Start from clang comparison. The important question is not "how do we call
  `printf` from RV64 asm?" but "where does the target ABI first change the IR
  and records c4c must preserve?"
- Keep generated IR and dump artifacts under `build/` or another non-root
  scratch location.
- If the answer is that c4c's LIR-to-LLVM-IR dump must become
  target-dependent, write that as a first-class follow-up rather than hiding it
  inside RV64 backend code.
