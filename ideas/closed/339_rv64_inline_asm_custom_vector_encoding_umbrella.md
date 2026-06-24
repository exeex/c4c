# RV64 Inline Asm Custom Vector Encoding Umbrella

## Goal

Build a staged RV64 inline-asm capability that lets users define custom vector
instructions from C++ without requiring c4c to know every custom mnemonic.

The intended user-facing path is:

```cpp
asm volatile(".insn ... template ..." : "=r"(out) : "r"(a), "r"(b));
asm volatile(".insn.d ... template ..." : "=VRM2"(vd) : "VRM2"(a), "VRM2"(b));
```

Then, once constant-evaluated asm strings are supported:

```cpp
asm volatile(ev::insn_d<EV64, EVADD, dtype_i32>() : "=VRM2"(vd) : "VRM2"(a), "VRM2"(b));
```

This umbrella should decompose into child ideas. It should close only after the
standard `.insn` route, vector register-group constraints, EV 64-bit template
route, and consteval/template string route are all completed and reviewed
together.

## Why This Exists

RV64 custom vector instruction work should not require c4c's assembler or
backend to grow a built-in mnemonic for every experimental operation. A better
C++-first model is to let the user describe custom instruction encodings in
inline asm, while c4c still owns the compiler-critical responsibilities:

- operand constraints
- register classes and allocation
- tied operands and overlap rules
- instruction byte emission
- object emission and relocation boundaries
- volatile, memory, and clobber semantics

This is stronger than C intrinsics because C++ templates and `consteval` can
define typed, reusable instruction families at the user/library layer. The
compiler provides the safe register-binding and encoding escape hatch; the user
library provides the custom operation vocabulary.

## Stage 1: Standard RISC-V `.insn` Inline Asm

Support inline asm templates that align with the standard RISC-V `.insn`
surface for current RV64 instruction lengths and operand constraints.

Required behavior:

- Parse and lower inline asm templates containing `.insn` forms.
- Support ordinary scalar constraints such as:
  - `r`
  - `=r`
  - `+r`
  - matching/tied operands where already supported or required
  - `f` / `=f` if the existing RV64 backend has FPR support ready enough for
    the chosen scope
- Substitute allocated registers into `.insn` templates correctly.
- Preserve `asm volatile`, clobber, and memory-side-effect behavior.
- Ensure object emission can encode or carry the resulting instruction bytes
  without routing through an external assembler as the primary proof.

Suggested initial examples:

```cpp
asm volatile(".insn r 0x0b, 0, 0, %0, %1, %2"
             : "=r"(out)
             : "r"(a), "r"(b));
```

Stage 1 should be compatible with the standard RISC-V mental model before EV
custom vector extensions are introduced.

## Stage 2: RV64 Vector Register Constraints

Add vector-register inline asm constraints suitable for RVV-style and custom
EV-style operands.

Required constraints:

- `VR`: one vector register, allocated from `v0` through `v31`
- `VRM2`: aligned consecutive vector register pair, allocated from
  `v0,v2,...,v30`, occupying `base` and `base+1`
- `VRM4`: aligned consecutive vector register group of four, allocated from
  `v0,v4,...,v28`, occupying `base` through `base+3`

The allocator should treat these like normal physical-register classes with
larger occupancy:

```text
VR    width=1 alignment=1
VRM2  width=2 alignment=2
VRM4  width=4 alignment=4
```

Substitution for grouped vector operands should print or encode the base vector
register. For example, a `VRM4` operand allocated at `v8` represents the group
`v8-v11`, but `%0` should identify the base register `v8` for instruction
encoding.

Required semantics:

- Distinct untied vector operands must not overlap.
- Tied operands such as `+VRM2` or matching constraints may intentionally
  overlap according to inline asm rules.
- Invalid group placement, impossible constraints, or unsupported register
  classes must fail explicitly.
- Mask-specific policy should not be silently conflated with ordinary vector
  registers; if mask constraints are needed, create a focused child idea rather
  than hiding that behavior inside `VR`.

## Stage 3: EV 64-Bit `.insn.d` Template

Add a c4c/EV-specific 64-bit instruction template form for custom vector
instructions.

The discussed conceptual layout is:

```text
  63      56 55                  40 39                 32
 +----------+----------------------+---------------------+
 | opcode8  | dtype/policy/imm16   | evop8 or subop8     |
 +----------+----------------------+---------------------+

  31   30 29   25 24   20 19   15 14   12 11    7 6      0
 +-------+-------+-------+-------+-------+-------+--------+
 |funct2 | rs3   | rs2   | rs1   |funct3 | rd    |opcode7 |
 +-------+-------+-------+-------+-------+-------+--------+
```

Design intent:

- `opcode7` is a fixed EV 64-bit split marker / namespace discriminator using
  an RV encoding value reserved for this custom instruction set.
- `opcode8` or `evop8` selects the EV operation class, for example `EVADD`.
- `rd`, `rs1`, `rs2`, and `rs3` are register fields supplied by inline asm
  operands and may be scalar or vector depending on constraints.
- `funct3` and `funct2` are compact subformat / minor-function fields.
- `dtype/policy/imm16` describes element type, data shape, signedness,
  rounding, saturation, mask/tail policy, or immediate payload according to the
  selected EV operation family.

The exact bit allocation may be refined during child implementation, but the
route must preserve the high-level model:

```text
EV64 marker = instruction format / namespace
EV op       = operation selector inside EV
dtype       = type/policy/data-shape metadata
constraints = compiler-owned register allocation contract
```

Suggested user style:

```cpp
#define EV64  0x0a
#define EVADD 0x0b

asm volatile(".insn.d %c[major], %c[op], %0, %1, %2, %3, %c[dtype]"
             : "=VRM2"(vd)
             : "VRM2"(a), "VRM2"(b), "VRM2"(c),
               [major] "i"(EV64),
               [op] "i"(EVADD),
               [dtype] "i"(EV_DTYPE_I32));
```

The `.insn.d` route should support fixed immediate fields, register operand
substitution, and object emission of 64-bit instruction bytes.

## Final Stage: Consteval/Template Asm Strings

Allow the inline asm template string to be produced by C++ constant evaluation
rather than only by a raw string literal.

The goal is to support header-only custom instruction libraries:

```cpp
namespace ev {
template<int Major, int Op, int DType>
consteval auto insn_d() {
  return /* compile-time asm template string */;
}
}

asm volatile(ev::insn_d<EV64, EVADD, EV_DTYPE_I32>()
             : "=VRM2"(vd)
             : "VRM2"(a), "VRM2"(b));
```

This stage should define the exact accepted constant-expression string type:

- string literal
- `constexpr` array/reference
- consteval function result
- template-built fixed string type, if that is the existing or planned c4c
  representation

The asm string must still be known at compile time. Runtime strings are out of
scope.

## In Scope

- RV64 inline asm parsing/lowering for `.insn` and `.insn.d`.
- Scalar and vector inline asm constraint handling needed by these forms.
- Vector register-group allocation for `VR`, `VRM2`, and `VRM4`.
- EV 64-bit instruction-byte emission for the agreed `.insn.d` layout.
- Object-route proof for emitted bytes.
- C++ constant-evaluated asm template strings.
- Focused tests for constraints, encoding, object output, and diagnostics.
- Child idea creation with clear dependencies between stages.

## Out Of Scope

- Teaching c4c semantic meanings for every EV custom operation.
- Replacing user-defined custom encodings with compiler built-in intrinsics.
- Full GNU assembler compatibility beyond the `.insn` forms needed here.
- Runtime-generated asm strings.
- Adding full RVV semantic lowering outside inline asm constraints.
- Supporting every possible vector mask convention in the first `VR`/`VRM*`
  implementation.
- Switching unrelated backend codegen to use EV instructions.

## Suggested Child Queue Shape

The exact queue should be finalized when this umbrella is activated, but the
expected child ideas are:

1. RV64 standard `.insn` inline asm scalar constraints and object proof.
2. RV64 vector register classes and inline asm constraints: `VR`, `VRM2`,
   `VRM4`.
3. EV 64-bit `.insn.d` format design and minimal encoder/object emission.
4. `.insn.d` register substitution and vector-group operand tests.
5. C++ consteval/template-produced asm string support.
6. Final integration review for custom vector instruction library readiness.

## Testing Expectations

Stage 1 tests should prove:

- `.insn` templates compile through inline asm.
- `r`, `=r`, and tied scalar operands allocate and substitute correctly.
- emitted object bytes match expected instruction encodings.
- invalid constraints and malformed `.insn` forms diagnose clearly.

Stage 2 tests should prove:

- `VR`, `VRM2`, and `VRM4` allocate legal base registers.
- grouped operands reserve all occupied registers.
- un-tied groups do not overlap.
- tied vector operands can reuse registers when the constraint says so.
- impossible allocation and bad constraints fail explicitly.

Stage 3 tests should prove:

- `.insn.d` emits exactly one 64-bit instruction encoding.
- `opcode7` marks the EV64 namespace.
- `opcode8` / EV op, `dtype/policy/imm16`, `funct3`, `funct2`, and register
  fields land in the expected bits.
- `VR`/`VRM2`/`VRM4` operands encode their base register field.
- object output can be inspected with repo-native byte or objdump/readelf
  checks.

Final-stage tests should prove:

- asm templates can come from accepted constant-evaluated C++ string forms.
- template-built EV instruction helpers produce the same bytes as literal
  `.insn.d` templates.
- non-constant asm strings remain rejected.

## Umbrella Completion Criteria

This umbrella can close only when:

- all child ideas created from it are closed;
- standard RV64 `.insn` inline asm works for the agreed scalar constraints;
- `VR`, `VRM2`, and `VRM4` constraints work with correct allocation and
  overlap rules;
- `.insn.d` supports the agreed EV64 64-bit encoding template;
- emitted `.insn.d` bytes are proven through object-route tests;
- C++ consteval/template-produced asm strings are accepted for inline asm;
- final review confirms that user-side C++ templates can define custom vector
  instruction families without modifying c4c's assembler for each operation.

## Closure Note

Closed 2026-06-24 after the staged child queue and final integration review
completed the umbrella contract. The closed scalar `.insn` object-route child
proved target-owned `.insn r` parsing, metadata carriage, GPR constraint
binding, diagnostics, and object-byte emission. The closed vector-constraint
child proved `VR`, `VRM2`, and `VRM4` allocation, alignment, overlap/tied
rules, and base-register substitution. The closed EV `.insn.d` child proved
the agreed 64-bit EV encoding template, immediate validation, grouped operand
encoding by base register, and little-endian object bytes. The closed
consteval/template-string child proved compile-time asm-template expressions
fold into the literal inline-asm path and reject runtime strings.

The final integration review child
`ideas/closed/347_rv64_inline_asm_custom_vector_integration_review.md` mapped
the parent completion criteria to closed child evidence and current tests,
verified the composed literal/helper `.insn.d` route through `VRM*`
constraints and RV64 object bytes, and found no required follow-up child before
parent closure. Close-time regression guard passed on the existing broader
`test_before.log` / `test_after.log` pair in non-decreasing mode: 80 passed
before and after, 0 failures, and no new failures.

Remaining GNU named operands, `%c[...]` modifiers, mask-specific constraints,
broader GNU assembler compatibility, runtime asm strings, and compiler-known
EV mnemonics/intrinsics remain outside this umbrella unless opened as separate
ideas.

## Reviewer Reject Signals

- The route claims custom instruction support while requiring every EV
  operation to be added as a compiler-known mnemonic or intrinsic.
- `.insn` support only works by delegating primary proof to an external
  assembler instead of c4c's object route.
- `VRM2` or `VRM4` allocation can choose misaligned bases or overlap unrelated
  operands.
- Grouped vector operands substitute a non-base register or encode an unstable
  register number.
- `.insn.d` is implemented as ad hoc string matching rather than a documented
  64-bit encoding template.
- Runtime asm strings are accepted as if they were compile-time templates.
- Existing asm/object tests are weakened to hide unsupported constraints or
  encoding failures.
- The umbrella is closed before all child ideas close and a final integration
  review records that the stages compose into the intended C++ custom-vector
  workflow.
