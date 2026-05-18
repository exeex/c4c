# AArch64 C-Testsuite Failure Family Inventory Todo

Status: Active
Source Idea Path: ideas/open/284_aarch64_c_testsuite_failure_family_inventory.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Inspect Frontend Boundary

# Current Packet

Use the confirmed 212-case AArch64 backend scan inventory and completed Step 2
runtime-family grouping as context. Next inspect `FRONTEND_FAIL` cases to
separate true frontend/BIR gaps from backend-route handoff or printer failures
before creating focused repair ideas. Do not implement fixes in this umbrella
plan.

## Just Finished

- Completed plan.md Step 2, "Inspect Runtime Failure Families", for the
  requested representative runtime sample using existing artifacts only.
- Sampled `RUNTIME_NONZERO`: `00009.c`, `00011.c`, `00012.c`, `00019.c`,
  `00022.c`, `00023.c`.
- Sampled `RUNTIME_MISMATCH`: `00156.c`, `00158.c`, `00160.c`, `00168.c`,
  `00169.c`.
- Observed low-numbered nonzero symptoms:
  - `00009.c` should compute `((1 * 10) / 2) % 3 - 2 == 0`, but generated
    assembly stores `w19` repeatedly after loading `x`, with no visible
    `mul`, divide, or modulo operation; scan exit was `118`.
  - `00011.c` and `00022.c` both lower simple zeroed locals to stack frames
    of 8 and 4 bytes respectively and then fault with `Bus error`; the likely
    semantic owner is AArch64 stack frame sizing/alignment for SP-relative
    locals, not C semantics.
  - `00012.c` should fold `(2 + 2) * 2 - 8` to zero, but assembly returns
    `sub w0, w19, #8`; scan exit was `160`, pointing at missing/incorrect
    constant expression and scalar arithmetic value materialization.
  - `00019.c` should follow a self-referential local struct pointer to `s.x`,
    but assembly only stores zero and an uninitialized `x13` pointer, then
    returns without loading `x`; scan exit was `1`, pointing at aggregate
    alloca/address/GEP materialization.
  - `00023.c` should zero and return global `x`, but assembly is just `ret`;
    scan exit was `1`, pointing at global object access/storage omission.
- Observed mismatch symptoms:
  - `00156.c`, `00158.c`, and `00160.c` are loop-and-`printf` cases whose
    expected output is present in the scan log but actual output is empty.
    Their assembly compares loop variables against `0` instead of the source
    upper bounds, and calls `printf` with `x19` as the format pointer; there is
    no `.rodata` string literal materialization in the generated `.s` files.
  - `00168.c` combines recursion, multiplication, loop output, and `printf`.
    Assembly compares the function argument against `0`, calls `factorial`
    with `x13`, does not materialize the `i - 1` recursive argument or the
    multiply, and also uses `x19` as the `printf` format pointer; actual output
    is empty.
  - `00169.c` nested loops should print all `x y z` triples. Assembly compares
    each loop variable against `0` and uses `x19` as the `printf` format
    pointer; actual output is empty.
- Grouped runtime families by likely semantic owner:
  - `aarch64-stack-frame-local-alignment`: SP-relative local frames that are
    not 16-byte aligned before memory operations (`00011.c`, `00022.c`).
  - `aarch64-scalar-arithmetic-value-lowering`: constant/binop arithmetic
    results replaced by uninitialized callee-saved registers or missing
    operators (`00009.c`, `00012.c`, also likely feeds recursion in
    `00168.c`).
  - `aarch64-aggregate-and-global-memory-addressing`: local aggregate pointer
    chains and global object load/store materialization are missing or
    incomplete (`00019.c`, `00023.c`).
  - `aarch64-loop-compare-branch-lowering`: loop bounds and branch predicates
    appear collapsed to comparisons with zero, so positive-bound loops exit
    before producing output (`00156.c`, `00158.c`, `00160.c`, `00168.c`,
    `00169.c`).
  - `aarch64-string-literal-and-variadic-call-lowering`: `printf` format
    strings are not emitted/addressed, and calls use `x19` as the format
    pointer with no `.rodata`/`adrp`/`:lo12` sequence in the sampled `.s`
    files (`00156.c`, `00158.c`, `00160.c`, `00168.c`, `00169.c`).
  - `aarch64-recursive-call-and-return-value-lowering`: recursive call
    arguments and multiply/return propagation are not credible in `00168.c`;
    this needs more evidence before splitting from scalar arithmetic and call
    lowering.
- Candidate focused idea names:
  - `aarch64_backend_stack_frame_alignment_for_sp_relative_locals`
  - `aarch64_backend_scalar_arithmetic_and_constant_value_lowering`
  - `aarch64_backend_loop_predicate_compare_bound_lowering`
  - `aarch64_backend_printf_string_literal_and_variadic_call_lowering`
  - `aarch64_backend_aggregate_global_address_value_materialization`
- Cases needing more evidence before repair selection:
  - `00168.c` spans recursion, scalar arithmetic, loop predicates, and
    `printf`; use it as an integration check after narrower families, not as
    the first repair target.
  - `00158.c` spans loop predicates plus `switch` dispatch and may need a
    separate switch-specific sample if control-flow work starts there.

## Suggested Next

- Execute plan.md Step 3, "Inspect Frontend Boundary": sample representative
  `FRONTEND_FAIL` cases from `/tmp/c4c_aarch64_backend_scan_212.log` and
  classify whether each is a true frontend/BIR gap, an AArch64 backend-route
  handoff failure, or a target-instruction printer coverage gap.
- Keep the Step 2 runtime families as context only while inspecting Step 3; do
  not create focused ideas until frontend-owned and backend-owned boundaries
  are clear.

## Watchouts

- Do not hand broad runtime scans to executors without timeout and stale-process
  cleanup.
- Do not count timeout/hang cases as ordinary repair packets.
- Do not change expected outputs, allowlists, unsupported classifications, or
  CTest behavior to improve counts.
- Keep frontend-owned failures separate from AArch64 backend repair ideas.
- The `RUNTIME_MISMATCH` samples with empty actual output are not proof that
  `printf` alone is broken; loop predicates often prevent reaching the call,
  and the generated call sequence also lacks credible format-string addressing.
- Avoid choosing `00168.c` as an initial focused repair target; it is a compound
  integration case with multiple owners.

## Proof

Metadata repair proof: no tests were run, per delegated proof. This edit only
advanced `Current Step ID` / `Current Step Title` to Step 3 and made the
current packet and suggested next work consistent with frontend-boundary
inspection.

Step 2 runtime-family inspection proof preserved below.

Used existing artifacts only:

```bash
/tmp/c4c_aarch64_backend_scan_212.classified
/tmp/c4c_aarch64_backend_scan_212.log
tests/c/external/c-testsuite/src/{00009,00011,00012,00019,00022,00023,00156,00158,00160,00168,00169}.c
build-aarch64-scan/c_testsuite_aarch64_backend/src/{00009,00011,00012,00019,00022,00023,00156,00158,00160,00168,00169}.c.s
build-aarch64-scan/c_testsuite_aarch64_backend/src/{00009,00011,00012,00019,00022,00023,00156,00158,00160,00168,00169}.c.bin.run-output
```

No broad CTest was run. No generated binaries were run manually. No
`test_after.log` was produced because this delegated packet was
artifact-inspection only and the proof contract explicitly requested existing
artifacts.
