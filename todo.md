# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_legacy_string_lookup_resweep.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Inventory Parser and Sema String Lookup Authority

## Just Finished

Completed Step 1 inventory for parser/Sema rendered-string lookup authority.

Structured-primary surfaces:
- Parser local typedef/value scopes use `TextId` through `LocalNameTable`; visible
  typedef/value lookup prefers `find_local_visible_*`, `QualifiedNameKey`
  typedef/value bindings, and `resolve_visible_*` before final spelling
  projection.
- Parser known-function classification uses namespace-aware
  `QualifiedNameKey` paths first, including current-record member keys and
  resolved visible-value keys.
- Parser template primary/specialization lookup is keyed by
  `QualifiedNameKey`; rendered mirrors are consulted only when no structured
  name is available or as mismatch counters.
- Parser NTTP default cache lookup prefers `NttpDefaultExprKey` and only uses
  rendered cache keys for TextId-less/final-spelling fallback.
- Sema local/global/enum `lookup_symbol` uses structured local or symbol keys
  when reference nodes carry identity, then falls back to rendered maps.
- Sema record owner, static-member, and instance-field lookup use structured
  record keys and member `TextId` when available.

Legitimate string surfaces:
- Diagnostics, source spelling, final spelling projection, mangling/debug text,
  namespace compatibility names, parser token spelling, and parser/HIR dump
  strings.
- `TypeSpec::tag` and rendered struct names where they are final spelling or
  compatibility carriers rather than record-layout authority.

Compatibility or TextId-less fallbacks:
- Parser `struct_tag_def_map` after `TypeSpec::record_def` /
  `resolve_record_type_spec` fails.
- Parser string-only helper overloads such as `find_visible_typedef_type`,
  `find_visible_var_type`, `resolve_visible_*_name`, and
  known-function compatibility fallback.
- Parser template rendered mirrors and NTTP rendered cache mirrors when no
  structured key is available.
- Parser support `eval_const_int` string-map overload and Sema/HIR late
  `struct_defs` string maps for producer-boundary callers without structured
  identity.
- Sema `structured_record_keys_by_tag_` is a guarded bridge from legacy record
  tags to structured keys, with ambiguity detection.

Suspicious string-authority routes:
- `ConstEvalEnv::lookup(const Node*)` computes text/key lookups but returns the
  legacy rendered-name result.
- `resolve_type` in `src/frontend/sema/consteval.cpp` computes text/key type
  binding checks but returns `env.type_bindings->find(name)`.
- `lookup_consteval_function` in `src/frontend/sema/consteval.cpp` computes
  text/key function checks but returns the rendered-name map result.
- `lookup_ref_overloads_by_name`, `lookup_cpp_overloads_by_name`, and
  `lookup_consteval_function_by_name` in Sema validation compare structured
  mirrors but still return rendered-name results.
- Parser `find_typedef_type(QualifiedNameKey, fallback_name)` renders a
  structured key and falls through to string lookup after structured miss;
  this should be reviewed before any broad typedef cleanup.

## Suggested Next

Smallest first implementation packet: convert Sema consteval function lookup to
structured-primary in both `src/frontend/sema/validate.cpp` and
`src/frontend/sema/consteval.cpp`. Specifically, when a reference/callee node
provides a valid structured key, return the key lookup result before the
rendered-name map; keep rendered lookup as explicit compatibility fallback when
no structured key exists, and preserve mismatch comparison as advisory.

Suggested proof command for that packet:
`cmake --build build --target c4cll && ctest --test-dir build -R 'cpp_(positive_sema_consteval|hir_consteval|negative_tests_bad_consteval|negative_tests_static_assert_consteval)' --output-on-failure`

## Watchouts

Do not edit HIR, LIR, BIR, or backend implementation for this frontend resweep
except to record a separate metadata handoff blocker under `ideas/open/`.
Rendered strings may remain for diagnostics, display, final spelling,
compatibility input, or explicit TextId-less fallback.

Parser already has many classified compatibility bridges from the prior cleanup
series. Avoid starting with broad parser typedef fallback removal; the Sema
consteval lookup packet is narrower and has direct structured mirrors already
available.

## Proof

Inventory-only packet; no build or test proof required, and no root-level proof
log was created.
