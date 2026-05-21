Status: Active
Source Idea Path: ideas/open/295_backend_regex_failure_family_inventory.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Classify Residual Buckets

# Current Packet

## Just Finished

Completed Step 2: Classify Residual Buckets. Inspected the existing
`test_after.log`, source files under `tests/c/external/c-testsuite/src/`, and
generated AArch64 assembly under `build/c_testsuite_aarch64_backend/src/`.

Classified residual buckets:

- `00174`: runtime mismatch in scalar floating-point expression and constant
  materialization. The source is a focused float/double arithmetic, comparison,
  assignment, unary, coercion, and `sin(2)` test. `test_after.log` shows the
  comparison rows and final `2.000000` / `0.909297` survive, while the expected
  arithmetic constants print as zero. Generated `00174.c.s` starts by converting
  and printing from FP registers such as `d13` without an obvious preceding
  constant materialization for the first values, so the first bad fact is
  missing or lost scalar FP constant/expression values rather than a broad
  control-flow or aggregate failure.
- `00216`: broad aggregate initializer, compound literal, relocation, and
  function-pointer-table bucket. The source mixes nested/global/local aggregate
  initializers, compound literals, local copies, range initializers, pointer
  fields, relocation-bearing function pointer arrays, and table dispatch.
  `test_after.log` reports `[RUNTIME_NONZERO]` with a segmentation fault.
  Generated `foo` has a very large local setup and early stale-looking stack
  traffic such as loads from offsets like `[sp, #472]`, `[sp, #480]`, and
  `[sp, #400]` before a clear local value source, plus later relocation/table
  traffic in `test_compound_with_relocs` and `test_multi_relocs`. Treat this as
  a broad aggregate/relocation owner, not the next focused non-timeout owner.
- `00200`: timeout quarantine. The source is a left-shift type-promotion stress
  test generated through nested macros over signed and unsigned short/int/long/
  long-long cases. Generated `00200.c.s` is 2448 lines with hundreds of
  `.LBB90_*` labels and repeated `check` call/control-flow expansions. First bad
  fact is excessive generated loop/control-flow expansion leading to timeout,
  not a narrow runtime mismatch.
- `00207`: timeout quarantine. The source stresses VLA/dynamic stack plus
  `goto`/label control flow in `f1`, fixed-size arrays across `goto` in `f2`,
  and short-circuit/conditional paths in `f3`. Generated `f1` stores through
  `[x29, #16]` without an established frame pointer, dynamically moves `sp`, and
  restores `sp` around jumps; `test_after.log` reports timeout. First bad fact
  is dynamic stack/VLA/goto control-flow lowering, and it should stay
  quarantined unless explicitly selected.

Candidate owner ranking for Step 3:

1. `00174` - strongest next non-timeout owner. It is focused, reproducible as a
   runtime mismatch, and the generated assembly points at scalar FP
   constant/expression materialization rather than a broad family.
2. `00216` - second non-timeout owner, but too broad for the next focused slice
   without first splitting aggregate local initialization from relocation and
   function-pointer table behavior.
3. `00207` - timeout quarantine; only select with an explicit dynamic-stack/VLA
   control-flow timeout packet.
4. `00200` - timeout quarantine; only select with an explicit type-promotion
   stress/code-size timeout packet.

## Suggested Next

Execute Step 3 against `00174`: isolate scalar FP constant/expression
materialization by comparing the first few source-level arithmetic prints
against generated AArch64 value setup, then choose the smallest semantic backend
owner for missing FP constant materialization or expression value preservation.

## Watchouts

Do not implement fixes under idea 295. Keep `00200` and `00207` quarantined
unless the supervisor explicitly selects a timeout owner. Avoid accepting a
named-case shortcut for `00174`; the next slice should repair scalar FP
constant/expression lowering generally enough to cover nearby FP arithmetic
uses. Treat `00216` as needing decomposition before implementation because its
evidence spans aggregate layout, local initialization, stale stack-derived
traffic, relocations, and function-pointer tables.

## Proof

No build was requested or needed for this inventory packet. Used the existing
proof log `test_after.log`, source inspection, and generated assembly
inspection only.
