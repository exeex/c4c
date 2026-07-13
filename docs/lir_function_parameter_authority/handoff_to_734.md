# LIR Function Parameter Authority Handoff To Idea 734

This is the Plan Step 3 handoff from idea 742 to the parked consumer
`ideas/open/734_lir_to_new_bir_container_completeness.md`, checked at commit
`c2024bf49`. It records producer and reachable-LIR-verifier facts. It does not
claim that new BIR receives any parameter row yet.

## Authority contract

[`populate_lir_function_params`](../../src/codegen/lir/hir_to_lir/hir_to_lir.cpp)
publishes the ordered logical parameter list from HIR `Function::params` into
`LirFunction.params` for declarations and definitions.
[`populate_signature_type_refs`](../../src/codegen/lir/hir_to_lir/hir_to_lir.cpp)
separately publishes fixed ABI rows in `signature_params`, their typed mirrors
in `signature_param_type_refs`, and the variadic/void-list flags.
`append_aarch64_hfa_signature_params` is the checked one-to-many ABI path.

[`verify_plain_fixed_scalar_parameter_relationship`](../../src/codegen/lir/verify.cpp)
requires exact count, order, default-shape `TypeSpec`, mirror kind, and mirror
width only for the proven nonvariadic plain fixed scalar relationship. It does
not force that rule onto transformed or expanded ABI shapes.

Parameter names, `signature_text`, and `LirTypeRef::str()` are presentation or
parity/diagnostic data. Text may confirm an already structured ABI fact, but it
must never invent logical identity, parameter count, order, type, or binding.

## Checked shape matrix

“Focused pair” below means both a declaration and definition are asserted by
`frontend_lir_function_signature_type_ref_test`. “Shared path” means both forms
use the same committed structured producer functions, but the named family has
no dedicated declaration/definition fixture in that test.

| Shape | `LirFunction.params` logical authority | `signature_params` ABI authority | Mirrors and flags | Declaration/definition evidence | Exact idea-734 receiver disposition |
|---|---|---|---|---|---|
| Empty C `()` | Empty ordered logical list | No fixed ABI rows | No parameter mirrors; `signature_has_void_param_list=false`; nonvariadic unless separately declared variadic | Focused pair in `test_definition_logical_parameter_publication` | **Unblocked only for bounded zero-fixed-parameter signature receipt.** It creates no body parameter identity. |
| Explicit `(void)` | One logical `TB_VOID` sentinel | No fixed ABI rows | No parameter mirrors; `signature_has_void_param_list=true`; `signature_is_variadic=false` | Focused pairs in `test_definition_logical_parameter_publication` and the main void fixture | **Unblocked only for bounded void-list signature-shape receipt.** The sentinel is not a bindable body parameter. |
| Plain fixed `int`/`uint`/`long long`/`unsigned long long`/`float`/`double`, default shape | Ordered HIR-owned `TypeSpec`; no pointer/reference/array/vector/function-pointer/byval shape | Exact same count, order, base, and default shape; every row has `is_byval=false` | Integer mirrors are typed `i32` for `int`/`uint` and `i64` for `long long`/`unsigned long long`; floating mirrors are typed `float`/`double`; void-list and variadic flags are false | Focused representative pair covers `int`, `long long`, `float`, `double`; verifier enumeration and shared producer path cover the named unsigned partners | **Unblocked for one bounded fixed-signature receipt packet only.** Import exact structured parity; do not parse names/text. Body use stays blocked because display names and raw operands are not parameter identity. |
| Plain fixed `long`/`unsigned long`, default shape | Ordered HIR-owned `TB_LONG`/`TB_ULONG` `TypeSpec` | Producer and verifier currently require an `i64` ABI row even for I686 | Typed integer mirror is currently `i64`; existing new-BIR target policy instead lowers I686 C `long`/`unsigned long` to `i32` | Shared declaration/definition producer path exposes the policy disagreement; inactive [idea 743](../../ideas/open/743_lir_i686_long_width_policy_convergence.md) owns cross-target convergence | **Blocked.** Do not receive either row until producer/verifier i64 policy and new-BIR I686 i32 policy converge under idea 743. |
| Pointer | Ordered logical pointee `TypeSpec` plus pointer/declarator shape | Fixed ABI row retains the structured pointer shape for the checked ordinary case | Typed pointer mirror (`ptr` presentation); nonbyval | Focused `int *` pair in `test_definition_logical_parameter_publication` | **Blocked.** Define the explicit pointee/type-mirror receiver contract first; neither `ptr` text nor ABI position can reconstruct it. |
| Narrow `char`/`schar`/`uchar`/`short`/`ushort` | Ordered logical narrow base and declarator shape | Fixed ABI row retains the structured narrow base in the ordinary case | Integer ABI mirror (`i8` or `i16` presentation); the current handoff proves no structured sign/zero-extension receiver fact | Focused `char` pair; shared producer and verifier exclusion cover the family | **Blocked.** Add structured integer-extension authority and its receiver mapping before receipt. |
| Direct aggregate | One logical aggregate `TypeSpec` with owned record identity | One direct aggregate ABI row for the checked amd64 `Pair`; `is_byval=false` | Structured aggregate mirror with `StructNameId` | Focused declaration/definition `Pair` assertions in the test main | **Blocked.** Raw BIR needs an aggregate parameter container and exact record/type mapping; do not infer it from `%struct.*` text. |
| amd64 byval aggregate | One logical aggregate `TypeSpec` | One aggregate ABI row with `is_byval=true` | ABI mirror carries the byval pointer fragment for parity; the structured byval flag and logical aggregate type are authoritative | Focused declaration/definition `Big` assertions in the test main | **Blocked.** Add typed byval aggregate, alignment, pointee, and ownership receipt; mirror text alone is insufficient. |
| AArch64 HFA | One logical aggregate `TypeSpec` | Multiple ordered floating lane rows emitted by `append_aarch64_hfa_signature_params`; each is nonbyval | One typed floating mirror per lane | Focused declaration/definition two-float HFA in `test_aarch64_hfa_parameter_classification`; reachable `verify_module` accepts one logical row versus two ABI rows | **Blocked.** Add an explicit one-logical-to-many-ABI receiver relation with aggregate and lane ownership. Never flatten lanes back into a guessed logical parameter. |
| Vector or other ABI expansion/transformation | One logical vector/aggregate/complex `TypeSpec` | Target ABI may retain, transform, or expand it into different structured rows | Mirrors describe emitted ABI rows; target-specific flags/shapes remain distinct from logical authority | Shared producer path; verifier deliberately exempts expanded logical shapes from scalar parity | **Blocked.** Require a separately checked transformation/expansion contract and typed receiver container for each admitted shape. |
| Variadic fixed prefix | `params` contains the ordered named fixed parameters; the ellipsis is not a logical parameter row | `signature_params` and mirrors contain only the fixed ABI prefix | `signature_is_variadic=true`; `signature_has_void_param_list=false` | Focused declaration/definition `int fixed, ...` assertions in the test main | **Blocked, including an otherwise plain prefix.** Add a variadic function contract before receiving the prefix; do not silently treat it as nonvariadic plain scalar receipt. |
| Function pointer | Logical `TypeSpec` carries `is_fn_ptr` and its declarator facts | Ordinary ABI carrier may be pointer-shaped, but remains separate from callable logical identity | Pointer-like ABI mirror is not a function-signature identity | Shared producer path; excluded from the exact plain verifier contract | **Blocked.** Add structured callable signature/pointee identity and receiver verification first. |
| `va_list` | Logical `TB_VA_LIST` and declarator shape | ABI carrier is target-specific and may differ from the logical form | Mirror records the emitted ABI carrier only | Shared producer path; excluded from the exact plain verifier contract | **Blocked.** Add a target-authored but lossless `va_list` carrier contract and typed receiver destination first. |

## Evidence and boundaries

- `test_definition_logical_parameter_publication` proves zero/void distinction,
  exact representative plain-scalar parity, presentation drift independence,
  pointer/narrow preservation, and malformed plain-row rejection.
- `test_aarch64_hfa_parameter_classification` proves a declaration and a
  definition retain one logical aggregate while publishing two float ABI lanes
  and mirrors.
- The main fixture in
  [`frontend_lir_function_signature_type_ref_test.cpp`](../../tests/frontend/frontend_lir_function_signature_type_ref_test.cpp)
  proves direct aggregate, amd64 byval, and variadic fixed-prefix publication
  for declarations and definitions, followed by reachable `verify_module`.
- The committed verifier rejects missing, reordered, type-conflicting,
  shape-conflicting, or mirror-conflicting exact plain rows. Its exemptions are
  preservation boundaries, not receiver support.
- Idea 742 added no new-BIR field, builder, verifier, importer mapping, or body
  parameter binding. All non-plain rows above remain fail-closed for idea 734.

## Bounded resume point for idea 734

The first authorized receiver packet is fixed signature receipt for exact
default-shape nonvariadic `int`, `uint`, `long long`, `unsigned long long`,
`float`, and `double` rows, plus explicit preservation of the zero-argument
versus void-list signature shape. It must consume the three structured tracks
and flags transactionally and reject disagreement.

That packet must not bind function-body operands to parameters. A later packet
needs native parameter value identity; names, raw operands, rendered signature
text, and ABI position cannot supply it. `long`/`unsigned long`, pointer,
narrow, aggregate, expanded, variadic, function-pointer, and `va_list` rows
remain blocked exactly as the matrix states.
