# Builtin VRM Register Carrier Types To Regalloc Frontier

## Goal

Add source-level builtin VRM carrier types so inline asm vector-register
operands can be represented as real compiler values before register allocation.

The immediate gap is that `VRM2` constraints currently classify the inline asm
operand as a vector register group, but a normal C scalar such as `long` still
lowers as a GPR value. That correctly produces prepared inline asm carrier
failures such as `operand_home_incompatible_register_class`.

This idea should establish the type/lowering chain for:

```c
__c4c_builtin_vrm1
__c4c_builtin_vrm2
__c4c_builtin_vrm4
__c4c_builtin_vrm8
```

The type meaning is only the register footprint:

- `vrm1`: one vector register
- `vrm2`: two contiguous vector registers
- `vrm4`: four contiguous vector registers
- `vrm8`: eight contiguous vector registers

It must not encode element dtype, signedness, lane count, operation family,
mask/tail policy, or any EV-specific instruction semantics. Users and library
authors can `typedef` these builtin carriers into their own ISA-facing names.

## Scope

- Teach the frontend parser/type system to accept the four builtin type names
  as first-class type spellings.
- Preserve the VRM carrier identity through typedefs and C++ template type
  substitution paths that already preserve ordinary type identity.
- Lower the VRM carrier identity through HIR/LIR into BIR with an explicit
  representation instead of falling back to `int`, `long`, struct, or GCC
  vector storage.
- Add BIR/prealloc metadata needed to recognize VRM values before register
  allocation:
  - register class: vector
  - register group width: 1, 2, 4, or 8
- Make prepared inline asm carrier validation distinguish real VRM values from
  scalar values when checking `VR`, `VRM2`, `VRM4`, and future `VRM8`
  constraints.
- Provide diagnostics or negative coverage for mismatched scalar operands, for
  example `"VRM2"(long_value)`.

## Out Of Scope

- Do not implement final register allocation placement changes unless an
  existing regalloc hook already consumes the new metadata naturally.
- Do not implement RV64 `.insn.d` object emission changes in this idea.
- Do not decide EV opcode layout, dtype layout, relocation, relaxation,
  disassembly, or linker behavior.
- Do not model vector element types, lane counts, or memory layout semantics.
- Do not add load/store intrinsics for a specific vector ISA.
- Do not make the VRM builtin types part of the ordinary C ABI yet. Function
  parameters, returns, globals, and general memory storage may remain rejected
  or explicitly unsupported unless the route proves a narrow, coherent local
  representation.

## Suggested Route

1. Add a source type carrier.
   - Prefer a dedicated `TypeSpec` carrier such as a small enum for builtin
     register carrier types.
   - Do not reuse `TypeSpec::is_vector`; GCC vector types carry storage and
     element-shape implications that VRM types intentionally avoid.

2. Add a stable lowering spelling.
   - Introduce a LIR-visible spelling such as `c4c.vrm1`, `c4c.vrm2`,
     `c4c.vrm4`, and `c4c.vrm8`, or an equivalent structured type ref.
   - Ensure dumps make it obvious that the type did not decay to `i64`,
     `ptr`, a struct, or `<N x T>`.

3. Add a BIR representation.
   - Add either explicit BIR type kinds or equivalent BIR-side value metadata
     for VRM width.
   - Keep the representation target-neutral: it means "vector register group
     width M", not "EV vector value".

4. Publish pre-regalloc classification metadata.
   - Before regalloc placement, VRM values should classify as
     `PreparedRegisterClass::Vector`.
   - The default contiguous width should come from the source type:
     `vrm1 -> 1`, `vrm2 -> 2`, `vrm4 -> 4`, `vrm8 -> 8`.
   - Inline asm constraint overrides may still validate or reinforce the same
     width, but they should not be the only source of vector identity.

5. Validate the frontier.
   - Stop at the regalloc frontier with prepared/liveness/prealloc dumps or
     tests that prove the value is ready to be allocated as a vector group.
   - Leave final allocation, substitution, and object-byte proof to the next
     idea after this foundation is reviewed.

## Acceptance Criteria

- `__c4c_builtin_vrm1`, `__c4c_builtin_vrm2`, `__c4c_builtin_vrm4`, and
  `__c4c_builtin_vrm8` parse as type names.
- A typedef smoke case preserves the carrier identity:

```c
typedef __c4c_builtin_vrm2 user_vec_pair;
```

- HIR or canonical dumps show the VRM identity distinctly enough that reviewers
  can tell it is not an integer, struct, GCC vector, or pointer fallback.
- LIR/BIR lowering preserves VRM identity through a stable internal spelling or
  structured type representation.
- Prealloc/liveness/regalloc-frontier metadata shows:
  - `__c4c_builtin_vrm1` wants vector class width 1
  - `__c4c_builtin_vrm2` wants vector class width 2
  - `__c4c_builtin_vrm4` wants vector class width 4
  - `__c4c_builtin_vrm8` wants vector class width 8
- An inline asm smoke before final allocation can form a prepared inline asm
  carrier without reporting scalar/vector class incompatibility for matching
  VRM operands.
- A negative scalar case still rejects or reports incompatibility:

```c
long x;
asm volatile("" : : "VRM2"(x));
```

## Example Source Shape

```c
typedef __c4c_builtin_vrm2 my_vrm2;

void use(my_vrm2 a, my_vrm2 b) {
  my_vrm2 out;
  asm volatile("" : "=VRM2"(out) : "VRM2"(a), "VRM2"(b));
}
```

If function parameters are not part of the first supported ABI surface, the
same proof can use locals or frontend-supported temporary values instead. The
important proof is that the source type becomes a vector-register-group value
before register allocation.

## Reviewer Reject Signals

- The route makes `VRM2` work by treating `long`, `i64`, or arbitrary scalar
  values as vector registers.
- The route reuses GCC vector type metadata and thereby ties VRM carriers to
  element dtype, lane count, or storage layout.
- The proof only checks parser acceptance but does not show the identity at
  HIR/LIR/BIR/prealloc.
- The proof relies on a fixture-built prepared module while real source-level
  type lowering remains absent.
- The implementation special-cases one test spelling instead of adding a
  durable builtin type representation.
- The idea drifts into `.insn.d` encoding, EV instruction design, linker,
  relocation, or disassembler work before the VRM type chain is proven.

## Follow-Up Ideas

- Use the VRM carrier types to complete source-level RV64 inline asm
  source-to-object proof for `.insn.d`.
- Add full regalloc allocation/substitution proof for `VRM1/2/4/8`, including
  aligned contiguous spans.
- Define optional ABI and memory rules for passing, returning, loading, or
  storing VRM carrier values if user code needs those semantics.
