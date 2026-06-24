Status: Active
Source Idea Path: ideas/open/346_rv64_standard_insn_scalar_inline_asm_object_route.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Map Existing Inline Asm And Object Surfaces

# Current Packet

## Just Finished

Lifecycle activation created this child plan from the RV64 inline asm custom
vector encoding umbrella. No implementation work has started.

## Suggested Next

Execute Step 1: map existing inline asm parsing, lowering, register
substitution, and object-emission surfaces; then record the selected route and
focused proof command here.

## Watchouts

- Keep this child limited to standard scalar RV64 `.insn`; vector constraints,
  EV `.insn.d`, and consteval/template asm strings belong to later child ideas.
- Do not prove progress by matching the umbrella's exact sample string.
- Do not rely on an external assembler as the primary object-byte proof.

## Proof

Lifecycle-only activation. No code validation was run.
