Status: Active
Source Idea Path: ideas/open/295_backend_regex_failure_family_inventory.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Classify Residual Failure Families

# Current Packet

## Just Finished

Step 2 classified the 17 Step 1 residuals from `test_after.log` by
first-bad-fact family using prepared-BIR and generated-assembly spot checks.

- Dominant family: address-valued memory and stack-home publication confusion
  across indirect load/store and call-argument boundaries.
  - Cases: `00005`, `00020`, `00103`, `00170`, `00173` timeout, `00181`,
    `00189`.
  - Evidence: `00020` prepared BIR has `pp -> *pp -> **pp`, but generated
    AArch64 returns `x21`, the address loaded from `pp`, instead of the final
    `i32`. `00173` pointer loops keep reloading fixed `.str0`/`.str0+1`
    instead of `*b`/`*src`, so the loop does not advance semantically. `00170`
    prepares `%lv.epos` as an address-exposed frame slot for `deref_uintptr`,
    but generated code uses `ldr x0, [sp, #8]` rather than materializing
    `sp+8`. `00189` prepares `stdout` in a stack slot for the second indirect
    call and then emits `ldr x0, [sp]` without first publishing the loaded
    global pointer there.
- Scalar comparison/select materialization family.
  - Cases: `00112`, `00123`, `00183`, `00200`, likely `00218`.
  - Evidence: `00112` prepared BIR is `bir.eq ptr %t0, 0` and return `%t2`,
    but generated AArch64 returns uninitialized `x13`. `00123` prepares
    `bir.slt double %t0, %t1`, but generated AArch64 returns raw register
    content after the double load. `00183` prepares a ternary select with
    join-transfer publication, but generated AArch64 omits the selected
    multiply values and prints the loop counter. `00218` zero-extends the
    8-bit bit-field in `convert_like_real`, but `main` writes the bit-field at
    a different stack offset than the callee reads, so the remaining owner is
    probably bit-field/aggregate-field address publication rather than enum
    semantics.
- Floating-point scalar and variadic-call family.
  - Case: `00174`.
  - Evidence: prepared BIR contains double arithmetic, comparisons, float
    trunc/extend, and variadic FPR call arguments, but generated output is
    mostly zeroes and corrupt integer comparison prints, distinct from the
    pointer/address cluster.
- Aggregate/byval and AArch64 composite ABI family.
  - Cases: `00140`, `00204`.
  - Evidence: `00140` is a byval struct plus variadic aggregate-call case; the
    generated caller passes uninitialized preserved registers for the variadic
    aggregate call. `00204` reaches the machine printer and fails on
    `deferred_unsupported: call-boundary move node requires prepared GPR
    registers, scalar FPR registers, or structured f128 q-register authority`,
    with representative HFA/f128 prepared-BIR call arguments in `arg`.
- Dynamic stack/control-flow family.
  - Case: `00207` timeout.
  - Evidence: `f1` prepares `llvm.stacksave`, dynamic alloca, two
    `llvm.stackrestore` sites, and a user-label/goto loop; this is separate
    from fixed frame-slot publication.
- Complex aggregate initializer/relocation family.
  - Case: `00216`.
  - Evidence: source stress-tests nested compound literals, flexible arrays,
    unnamed unions, range initializers, and function-pointer relocations. It
    should not be folded into the pointer-call owner without a smaller
    first-bad-fact probe.

## Suggested Next

Have the supervisor route Step 3 to a focused semantic owner for
address-valued memory/call-argument publication. A tight first packet should
prove one or two minimal representatives such as `00020` plus `00170` or
`00189`, with nearby pointer cases checked as adjacency, before touching the
scalar compare/select or aggregate ABI families.

## Watchouts

- This umbrella is classification-only; do not edit implementation files or
  tests here.
- Do not reopen closed or parked focused owners from pass counts alone.
- Rejected adjacent owners for Step 3: scalar compare/select publication
  (`00112`, `00123`, `00183`, `00200`, probably `00218`), floating-point
  variadic scalar correctness (`00174`), composite/byval/HFA ABI (`00140`,
  `00204`), dynamic stack/goto (`00207`), and complex aggregate initializer
  relocation (`00216`).
- `00173` is a timeout but shares the pointer-deref first bad fact with the
  address publication family; `00207` is the timeout that should stay separate.
- Keep Step 3 semantic: avoid named-case special handling. The owner boundary
  should be framed around when a value is an address to materialize, a pointer
  value to reload, or a pointee to load/store.

## Proof

No proof command was run for this classification-only packet. Evidence source
was the existing Step 1 `test_after.log` plus read-only `c4cll`
`--dump-prepared-bir`, `--dump-bir`, `--trace-mir`, and `--codegen asm`
spot checks on representative external c-testsuite cases. Proof log remains
`test_after.log`.
