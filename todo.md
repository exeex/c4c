Status: Active
Source Idea Path: ideas/open/343_rv64_consteval_inline_asm_template_strings.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Map Existing String And Inline Asm Surfaces

# Current Packet

## Just Finished

Lifecycle switch created the consteval/template inline asm string child idea
and activated its runbook.

## Suggested Next

Delegate Step 1 to `c4c-executor`: inspect the existing inline asm and
compile-time string surfaces, then identify the narrowest implementation route
for folding accepted asm-template expressions before inline asm lowering.

## Watchouts

- Do not accept runtime strings.
- Do not add EV intrinsics or compiler-known EV mnemonics as a substitute for
  consteval inline asm template support.
- Keep literal asm templates on the existing path.
- Preserve `%0` positional placeholders, operand order, constraints, clobbers,
  volatility, and memory metadata after folding.

## Proof

Lifecycle-only switch. No code validation required.
