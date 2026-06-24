# RV64 Standard `.insn` Scalar Inline Asm Object Route

## Goal

Implement the first concrete child of the RV64 inline asm custom vector
encoding umbrella: standard RV64 `.insn` inline asm for scalar operands, with
repo-native object-byte proof.

This child is the scalar foundation for later vector-register and EV
`.insn.d` work. It should make ordinary `.insn` templates usable without
teaching c4c every custom operation as a compiler-known mnemonic.

## Why This Exists

The umbrella requires a reliable inline-asm encoding escape hatch before custom
vector encodings can be layered on top. Stage 1 should prove that c4c can own
the compiler-critical parts of scalar `.insn` handling:

- parsing and lowering inline asm templates that use standard RISC-V `.insn`
  forms;
- scalar operand constraints and tied operand behavior;
- allocated register substitution into the template;
- volatile, memory, and clobber preservation;
- object emission of instruction bytes through c4c's own object route.

## In Scope

- RV64 standard `.insn` forms needed for the initial scalar route, starting
  with the `.insn r opcode, funct3, funct7, rd, rs1, rs2` style shape used by
  the umbrella example.
- Scalar inline asm constraints needed by this route: `r`, `=r`, `+r`, and
  matching/tied operands where the existing inline-asm model supports or needs
  them.
- Register substitution for allocated scalar GPR operands.
- Clear diagnostics for malformed `.insn` templates, unsupported forms, and
  incompatible scalar constraints.
- Object-route proof that emitted bytes match expected RV64 encodings without
  relying on an external assembler as the primary proof.
- Focused frontend, backend, and object tests that cover the supported scalar
  surface and nearby negative cases.

## Out Of Scope

- Vector register constraints such as `VR`, `VRM2`, or `VRM4`.
- EV-specific 64-bit `.insn.d` encoding.
- C++ constant-evaluated asm template strings.
- Full GNU assembler compatibility beyond the standard `.insn` subset selected
  for this child.
- Floating-point constraints unless an executor proves the existing RV64 FPR
  backend path is ready and the plan owner explicitly extends this child.
- Adding compiler-known mnemonics or intrinsics for custom operations.

## Acceptance Criteria

- A scalar `.insn r ...` inline asm fixture compiles through frontend, backend,
  and object emission.
- `r`, `=r`, `+r`, and supported matching/tied scalar operands allocate and
  substitute stable GPR names or numbers in the emitted instruction fields.
- `asm volatile`, memory, and clobber behavior remains preserved for the
  supported inline-asm route.
- Object-route tests prove exact emitted instruction bytes for at least one
  supported `.insn` encoding.
- Negative tests reject malformed `.insn` forms, unsupported `.insn` variants,
  and incompatible scalar constraints with source-level diagnostics.
- Existing inline asm, backend, and object tests are not weakened or downgraded
  to make this route pass.

## Reviewer Reject Signals

- The route claims `.insn` support but primarily depends on an external
  assembler for proof instead of c4c object-byte tests.
- The implementation adds named custom mnemonics or operation-specific
  shortcuts instead of lowering a documented standard `.insn` form.
- Register substitution is testcase-shaped, for example matching the exact
  sample string or operand count without parsing the supported `.insn` grammar.
- Tied or `+r` operands are ignored while tests only cover unrelated untied
  operands.
- Malformed `.insn` templates silently pass through, miscompile, or fail only
  after object emission without a useful diagnostic.
- Unsupported expectation rewrites, weaker test contracts, or `unsupported`
  downgrades are used as evidence of progress without explicit user approval.
- Broad backend rewrites unrelated to scalar `.insn` lowering are mixed into
  this child.
- The exact old failure mode is retained behind a renamed helper or
  classification-only change.

## Umbrella Link

Parent idea: `ideas/open/339_rv64_inline_asm_custom_vector_encoding_umbrella.md`.

This child covers Stage 1 from the umbrella. It should close before the
remaining child ideas for vector register constraints, EV `.insn.d`, and
consteval/template-produced asm strings are treated as complete.

## Closure Note

Closed after committed Steps 1-5 completed the scalar RV64 `.insn r` object
route: structured metadata is parsed and carried through HIR/LIR/BIR, scalar
GPR operands are bound through the existing allocation/prealloc route, object
emission proves exact encoded bytes, and side-effect plus diagnostic coverage
preserves the supported route boundaries. Close-time regression guard passed on
the broader 17-test proof logs with no pass-count or failure regression.

Remaining vector constraints, EV `.insn.d`, consteval/template-produced asm
strings, and broader GNU assembler compatibility remain outside this child and
belong to separate follow-up source ideas.
