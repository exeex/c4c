# RV64 Consteval Inline Asm Template Strings

## Goal

Allow an inline asm template to come from a C++ compile-time string expression,
so user-side helper libraries can build `.insn.d` templates without adding a
compiler-known mnemonic or intrinsic for every EV operation.

The first supported route must fold accepted compile-time string expressions
into exactly the same final inline asm template text used by literal asm
templates before the existing inline asm parser, HIR/LIR carrier, substitution,
and RV64 object emission paths run.

## Why This Exists

The completed RV64 inline asm stages provide scalar `.insn`, vector register
constraints, and EV `.insn.d` object emission for literal templates. The
remaining user-facing gap is library ergonomics: a header-only EV library should
be able to generate the `.insn.d` template at compile time from template
parameters such as major opcode, EV op, and dtype, then pass that string into
`asm volatile(...)`.

This keeps EV operation vocabulary in user/library code while c4c continues to
own register binding, operand constraints, lowering, and object byte emission.

## Dependencies

- `ideas/closed/340_rv64_standard_insn_inline_asm_stage1.md`
- `ideas/closed/341_rv64_vector_register_inline_asm_constraints_stage2.md`
- `ideas/closed/342_rv64_ev_insn_d_inline_asm_stage3.md`
- `ideas/open/339_rv64_inline_asm_custom_vector_encoding_umbrella.md`

## In Scope

- Define the exact accepted compile-time string representation for inline asm
  templates in the current c4c frontend, choosing the narrowest route that fits
  existing constant-expression support.
- Accept raw string literals exactly as before.
- Accept compile-time `+` concatenation of string literals and supported fixed
  compile-time string values.
- Accept a `constexpr` or `consteval` helper result only when it folds to a
  compile-time string before inline asm lowering.
- Accept a template-built fixed-string helper only if it can be represented
  through the same constant-folded string path.
- Preserve output/input operand order, constraints, clobbers, volatility, and
  memory-side-effect metadata while changing only the template-string source.
- Prove a template-built `.insn.d` helper produces the same final emitted bytes
  as the equivalent literal `.insn.d` template.
- Reject runtime-generated asm template strings with a clear diagnostic.

## Out Of Scope

- Runtime strings or any asm template whose final text is not known at compile
  time.
- Python-style formatting, `std::format`-style formatting, reflection, or broad
  compile-time formatting machinery.
- GNU named operands such as `%[name]` and `%c[...]` modifiers.
- Reopening scalar `.insn`, vector register constraint allocation, or `.insn.d`
  object encoding except for integration defects exposed by this route.
- Adding EV operation intrinsics or compiler-known EV mnemonics.
- Broad C++ constant evaluator rewrites that are not needed to fold the chosen
  compile-time string representation.

## Accepted User Shapes

The executor should inspect the current frontend before committing to the exact
surface, but the intended first model is:

```cpp
asm volatile(cts(".insn.d ") + ev64_text + cts(", ") + evadd_text +
                 cts(", %0, %1, %2, %3, ") + dtype_i32_text
             : "=VRM2"(vd)
             : "VRM2"(a), "VRM2"(b), "VRM2"(c));
```

and a small helper shape equivalent to:

```cpp
namespace ev {
template<int Major, int Op, int DType>
consteval auto insn_d() {
  return /* compile-time string expression equivalent to the literal template */;
}
}

asm volatile(ev::insn_d<EV64, EVADD, EV_DTYPE_I32>()
             : "=VRM2"(vd)
             : "VRM2"(a), "VRM2"(b), "VRM2"(c));
```

If c4c already has a different fixed-string or compile-time string value type,
use that representation instead of inventing a parallel one.

## Acceptance Criteria

- Literal inline asm templates continue to compile and lower unchanged.
- Accepted compile-time string expressions fold to the same final
  `InlineAsmStmt` template text as an equivalent literal string.
- Constant-folded template text flows through the same inline asm parser,
  constraint handling, HIR/LIR/BIR metadata, prepared substitution, and object
  emission paths as literal templates.
- A positive RV64 EV test builds an `.insn.d` template through a compile-time
  helper and emits the same 8 bytes as the literal `.insn.d` route.
- Positive coverage includes compile-time concatenation and one helper style
  selected from the existing frontend's capabilities.
- Negative coverage rejects a non-constant or runtime asm template expression.
- Negative coverage rejects unsupported compile-time string shapes with a
  diagnostic instead of silently dropping the asm, treating it as an empty
  template, or accepting a runtime value.
- Existing `.insn`, `VR`/`VRM2`/`VRM4`, and `.insn.d` tests remain strong; no
  expectation downgrade is used as proof of progress.

## Proof Expectations

- Frontend/HIR proof: dump or test the accepted asm-template expression folding
  to the exact literal template text, including `%0`-style placeholders.
- Inline asm metadata proof: preserve constraints, operand ordering, volatile,
  clobber, and memory metadata after folding.
- RV64 integration proof: compile a consteval/template-built `.insn.d` case
  using `VRM2` operands through the same object route as the literal case.
- Object proof: compare emitted text bytes against the known EV `.insn.d`
  encoding, including the previously proven byte sequence where applicable.
- Negative proof: non-constant template expressions and unsupported string
  expression forms fail with clear diagnostics.
- Regression proof: run the focused frontend/backend tests touched by the
  implementation and a supervisor-selected broader subset when the changed
  surfaces cross frontend, HIR/LIR, BIR, and backend boundaries.

## Reviewer Reject Signals

- The route accepts runtime strings or defers asm-template text construction
  past compile time.
- The route implements a named-case shortcut for one EV helper or one literal
  spelling instead of a general constant-folded asm-template expression path.
- The route adds compiler-known EV mnemonics or intrinsics and claims that as
  consteval inline asm string support.
- The route bypasses existing inline asm parsing, constraints, substitution, or
  object emission instead of feeding the folded text into the same path used by
  literal templates.
- The route weakens, disables, or reclassifies existing inline asm, vector
  constraint, `.insn.d`, or object tests to claim progress.
- The route only rewrites expected diagnostics or helper names without adding
  accepted compile-time string behavior.
- The route accepts compile-time strings but loses operand ordering,
  constraints, volatility, clobbers, memory effects, or `%0` placeholder
  semantics.
- The route leaves the old failure mode in place behind a new abstraction name,
  such as a fixed-string wrapper that still cannot be passed to `asm volatile`.
- The route performs a broad constant evaluator or frontend rewrite that is not
  required by the selected compile-time string surface.

## Lifecycle Notes

- 2026-06-24: Current runbook was parked after Step 6 broad proof because full
  CTest failed on the unrelated backend-route publication test
  `backend_codegen_route_riscv64_pointer_typed_select_publication`, which
  still emits forbidden `mv t0, t0`. Focused inline-asm-template proof and the
  helper-style `.insn.d` object proof were complete, but this idea should not
  move to close review until that broad-proof blocker is resolved, explicitly
  baselined, or otherwise dispositioned by the supervisor.
