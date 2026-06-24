# Textual Assembler Object Route Follow-Up

## Goal

Add a scoped textual assembler route for c4c-emitted assembly, if still needed
after direct native object emission, by reusing target encoders and the shared
object model without making the compiler's primary `.o` path depend on text
round-tripping.

## Why This Exists

The long-term architecture keeps two useful paths:

```text
backend machine model -> object writer -> .o
backend machine model -> asm writer -> .s
c4c-as .s -> object writer -> .o
```

The textual assembler lane is valuable for compatibility and diagnostics, but
it should follow the direct object route rather than block it. It must be
explicitly scoped to c4c-emitted or agreed supported assembly syntax before any
larger GNU-compatible assembler effort.

## In Scope

- Decide whether a textual assembler is still needed after direct RV64 and
  AArch64 object emission.
- If needed, parse the supported c4c-emitted assembly subset into target
  fragments that feed the same object model and target encoders used by direct
  object emission.
- Add round-trip compatibility tests for c4c-emitted `.s` where useful.
- Preserve diagnostics that distinguish parser errors, encoder errors,
  relocation errors, and ELF writer errors.

## Out Of Scope

- Blocking direct compiler `.o` support on textual parsing.
- Building a full GNU-compatible assembler in this child.
- Replacing the compiler's backend machine model with parser records.
- Weakening asm-route tests or object-route tests.
- Implementing unrelated target relocation or encoding features solely to
  accept arbitrary external assembly.

## Acceptance And Proof Expectations

- The child either records a reviewed decision that textual assembler work is
  not currently needed, or adds a scoped `c4c-as` style route for the agreed
  c4c-emitted subset.
- Any implemented textual assembler route feeds target fragments and the shared
  object model, then emits valid relocatable objects.
- Tests prove the textual route is separate from direct compiler `.o` emission
  and does not become a hidden dependency of `--codegen obj`.
- Existing direct object and asm-route tests continue to pass.

## Dependency Notes

- Depends on the shared object model/API and at least one target object emitter.
- Should run after direct `.o` support has established the target fragment API.
- May be skipped or closed with a no-work-needed decision if direct object
  emission and asm-route compatibility already satisfy the umbrella.

## Reviewer Reject Signals

- The compiler's primary `--codegen obj` path is changed to print `.s` and
  parse it through this textual assembler.
- The parser accepts only one named testcase shape while claiming general
  assembler support.
- The route copies the reference assembler wholesale instead of adapting a
  c4c-owned scoped parser and object model integration.
- GNU compatibility is claimed without proof for directives, expressions,
  relocations, alignment, and target syntax beyond the supported c4c-emitted
  subset.
- Existing direct object tests or asm-route tests are weakened to hide textual
  assembler gaps.
- Parser records replace backend machine facts in the direct object route.
