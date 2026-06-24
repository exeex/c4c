# Native Object Emission Umbrella

## Closure Note

Closed after final umbrella acceptance review confirmed the child closures
compose into native object-emission readiness for the agreed supported subset.

Evidence:

- Child ideas 330, 331, 332, 333, 334, and 335 are closed, along with focused
  follow-up target-emitter children 336, 337, and 338.
- Direct `--codegen obj` emits backend-produced object bytes and does not route
  through printed `.s` text.
- RV64 and AArch64 both have accepted direct `.o` emission paths for the
  agreed supported backend subsets.
- Existing `.s` route coverage remains selected and meaningful.
- Object-route tests cover ELF structure, diagnostics, linkability, RV64
  runtime behavior, AArch64 object-byte output, and c-testsuite object smokes.
- The c-testsuite/default strategy is intentionally dual-route for now:
  `--codegen obj` remains explicit and is not the default backend output form.
- Close-scope regression guard passed with `test_before.log` and
  `test_after.log` both at 41/41 using `--allow-non-decreasing-passed`.

Remaining boundaries are follow-up scope, not umbrella blockers: AArch64
saved-callee/local-call-frame, branch/control-flow, branch/global families,
AArch64 runtime, globals/data, pointers, aggregates, byval, indirect calls,
x86 object output, object stdout, c-testsuite object defaults, and semantic-BIR
object mode.

## Goal

Plan and decompose the native `.o` emission route for RV64 and AArch64 so c4c
can eventually produce relocatable ELF objects directly while preserving the
existing `.s` assembly route as a tested compatibility path.

This is an umbrella idea. Its first activation should research the current
backend architecture, identify the right integration seams, create focused
child ideas, and then switch execution to the first child idea.

## Why This Exists

RV64 and AArch64 can now produce `.s` output. The next capability is native
object emission:

```text
backend machine model -> object writer -> .o
```

The textual assembler path is still valuable:

```text
backend machine model -> asm writer -> .s
c4c-as .s -> object writer -> .o
```

But the compiler's primary `.o` path should not require printing `.s` and
parsing it back. The object route needs an internal representation for encoded
instructions, sections, symbols, labels, and relocations that can support both
direct object writing and later textual assembler input.

The reference implementation under
`ref/claudes-c-compiler/src/backend/riscv/assembler/` has useful design
material, especially:

- typed `EncodeResult` records
- target relocation enums
- RISC-V `pcrel_hi` / `pcrel_lo` pairing
- ELF section/symbol/relocation writer structure
- an explicit choice to leave RVC compression to linker relaxation

The reference should inform c4c's design, but this umbrella should not turn
the compiler's direct `.o` path into a text-assembler dependency.

## In Scope

- Research how current RV64 and AArch64 backend emission flows are structured.
- Identify where a shared or target-specific object-emission model should sit.
- Decide how `--codegen asm` and a future `--codegen obj` should coexist.
- Define testing strategy for dual `.s` and `.o` routes.
- Create focused child ideas under `ideas/open/` for implementation slices.
- Ensure the child ideas cover at least:
  - internal object-emission model / API shape
  - RV64 object emission
  - AArch64 object emission
  - CLI and test integration
  - textual assembler follow-up, if still needed after the direct `.o` route

## Out Of Scope

- Implementing object emission inside this umbrella plan.
- Replacing the current `.s` backend route immediately.
- Making c-testsuite default to `.o` before the object route is proven stable.
- Building a full GNU-compatible textual assembler before direct `.o` support.
- Weakening existing backend tests or removing `.s` route coverage.
- Expanding into unrelated backend semantic fixes such as function-pointer
  frontend lowering or prepared edge-publication scheduling.

## Required First Activation Behavior

When this umbrella is activated, the first runbook should be research-only and
decomposition-oriented:

1. Inspect current RV64 and AArch64 backend emission architecture.
2. Inspect current backend runtime and c-testsuite backend test wiring.
3. Inspect the existing partial C++ RV64 assembler/object writer port under
   `src/backend/mir/riscv/assembler/`.
4. Compare only the relevant design pieces from
   `ref/claudes-c-compiler/src/backend/riscv/assembler/`.
5. Produce child ideas under `ideas/open/` with concrete ownership boundaries.
6. Switch lifecycle state to the first child idea after the child queue exists.

The umbrella should not remain the active execution plan after the child idea
queue has been created.

## Lifecycle Note

The first activation completed its research/decomposition pass and created the
child queue under `ideas/open/330_native_object_model_and_emission_api.md`
through `ideas/open/335_textual_assembler_object_route_followup.md`. Active
execution has moved to child 330; keep this umbrella open until the child queue
closes and final umbrella acceptance review confirms the children compose into
native object-emission readiness.

## Child Idea Contract

Each child idea must include:

- a narrow goal
- owned files or subsystems where knowable
- explicit non-goals
- testing and proof expectations
- reviewer reject signals
- dependency notes on earlier or later child ideas

Child ideas should be small enough to close independently. Do not create a
single child idea that implements the whole object pipeline.

## Suggested Child Queue Shape

The exact child idea list should be based on the architecture research, but a
reasonable starting shape is:

1. Native object model and emission API design.
2. RV64 minimal relocatable ELF object emission for current backend output.
3. AArch64 minimal relocatable ELF object emission for current backend output.
4. CLI/test integration for `--codegen obj` with dual asm/object runtime paths.
5. Broader c-testsuite object-route scan and default-switch readiness.
6. Textual assembler route for c4c-emitted `.s`, if still needed.

## Testing Strategy Intent

Object-route tests should be added without deleting existing asm-route tests:

- object contract tests using `readelf` and `llvm-objdump`
- runtime smoke tests where `clang` links c4c-produced `.o`
- dual route tests that keep `.s -> clang/as -> binary` coverage alive
- c-testsuite backend labels that distinguish asm and object routes

The intended transition is:

```text
asm route remains green
object route added beside it
object route becomes default only after stable proof
asm route remains as AsmWriter / c4c-as compatibility coverage
```

## Umbrella Completion Criteria

This umbrella can close only when all of the following are true:

- Every child idea created from this umbrella is closed.
- RV64 and AArch64 both have accepted direct `.o` emission paths for the agreed
  supported backend subset.
- Existing `.s` route tests remain present and meaningful.
- Object-route tests cover ELF structure, linkability, and runtime behavior.
- The c-testsuite backend strategy has either switched to `.o` by default or
  has a documented, reviewed reason to remain dual-route.
- A final umbrella acceptance review confirms the child closures compose into
  the original architecture goal.

## Final Review Requirement

Before closing this umbrella, run a final review that checks:

- no child idea was closed by weakening tests or dropping supported behavior
- direct `.o` emission does not depend on parsing c4c-emitted `.s`
- `.s` route coverage still catches text emission regressions
- object-route failures are diagnosable at the object writer / encoder /
  relocation layer instead of hidden behind clang assembler behavior
- RV64 and AArch64 route differences are explicit and justified

The umbrella must stay open if any child idea remains open or if the final
review finds that the child closures do not compose into native object
emission readiness.

## Reviewer Reject Signals

- The route claims native `.o` progress while still requiring `.s` text to be
  parsed on the compiler's primary object path.
- The child queue is missing either RV64 or AArch64 object-emission ownership.
- Existing asm-route tests are removed or weakened before object-route
  coverage is stable.
- c-testsuite expectations are downgraded, marked unsupported, or narrowed to
  hide object-route failures.
- The implementation copies the reference assembler wholesale without adapting
  it to c4c's backend architecture and lifecycle boundaries.
- Relocation handling is represented as ad hoc strings or magic numbers spread
  through backend emission instead of typed target relocation records.
- The umbrella is closed before every child idea is closed and a final
  acceptance review is recorded.
