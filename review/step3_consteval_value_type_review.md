# Step 3.1 Consteval Value/Type Metadata Route Review

Active source idea: `ideas/open/139_parser_sema_rendered_string_lookup_removal.md`

Chosen base commit: `aceee02a7` (`[plan] split Step 3 around consteval metadata producers`)

Why this base: it is the lifecycle commit that created the current Step 3.1/3.2 split and made consteval metadata producer repair a required precondition before deleting rendered consteval compatibility. Later commits mostly returned to Step 2 parser/member-typedef work and the Step 2.5 const-int boundary, so `aceee02a7` is the relevant active Step 3 checkpoint rather than the latest todo-only packet.

Commit count since base: 84

## Findings

### Severity High: Step 3.1 Must Repair Producers Before Deleting `ConstEvalEnv` Rendered Fallback

`src/frontend/sema/consteval.hpp:213` makes `ConstEvalEnv::lookup(const Node*)` consult structured key metadata, then `TextId` metadata, then treat metadata misses as authoritative. However, `src/frontend/sema/consteval.hpp:230` still allows `lookup_rendered_nttp_compatibility()` to resolve an NTTP by rendered name after a structured/text miss, and `src/frontend/sema/consteval.hpp:258` still falls back to `nttp_bindings->find(n->name)` when the NTTP metadata maps are absent.

Valid Step 3.1 repair: make the missing producer path explicit for every parser/Sema-owned consteval caller before Step 3.2 deletes this compatibility. The first executor packet should focus on one value producer route and prove that the caller supplies `nttp_bindings_by_text` and `nttp_bindings_by_key` when it also supplies rendered `nttp_bindings`.

The clearest in-scope producers are:

- Consteval template call binding in `src/frontend/sema/consteval.cpp:501`, especially the NTTP branches at `src/frontend/sema/consteval.cpp:539`, `src/frontend/sema/consteval.cpp:550`, and `src/frontend/sema/consteval.cpp:561`. These already call `record_nttp_binding_mirrors()` and are valid to tighten with focused tests.
- Validator static-assert call binding in `src/frontend/sema/validate.cpp:1699`, which allocates the type/NTTP text and key maps and passes them into `bind_consteval_call_env()` at `src/frontend/sema/validate.cpp:1707`.
- Nested consteval call binding in `src/frontend/sema/consteval.cpp:935`, which does the same before calling `evaluate_consteval_call()` at `src/frontend/sema/consteval.cpp:948`.

Do not delete the rendered compatibility for HIR-owned callers in this step. If a remaining caller can only supply `nttp_bindings` from HIR metadata, park it under the HIR metadata idea instead of pulling `src/frontend/hir` into Step 3.1.

### Severity High: TypeSpec Lookup Must Prefer Intrinsic TypeSpec Metadata, Not Name-To-Metadata Mirrors

`src/frontend/sema/consteval.cpp:151` resolves template type bindings for `TB_TYPEDEF` TypeSpecs. The correct Step 3.1 producer route is the intrinsic `TypeSpec` metadata lookup at `src/frontend/sema/consteval.cpp:131`, using `TypeSpec::template_param_owner_namespace_context_id`, `template_param_owner_text_id`, `template_param_index`, and `template_param_text_id` from `src/frontend/parser/ast.hpp:105`.

The rendered-name mirror paths at `src/frontend/sema/consteval.cpp:118` and `src/frontend/sema/consteval.cpp:172` are not a valid final route for Step 3.1/3.2. They are useful compatibility/proof surfaces only while parser producers are incomplete.

Valid Step 3.1 repair: repair parser/Sema-owned TypeSpec producers so template-parameter TypeSpecs entering consteval evaluation carry intrinsic metadata. Existing producer infrastructure starts at `src/frontend/parser/impl/declarations.cpp:25` and recurses through template-owned declarations at `src/frontend/parser/impl/declarations.cpp:54` and `src/frontend/parser/impl/declarations.cpp:1978`. The executor should first audit and patch holes where parser creates `TB_TYPEDEF` for template parameter references without calling equivalent annotation.

Likely first holes to inspect:

- Simple identifier type production in `src/frontend/parser/impl/types/base.cpp:1689`, which sets `tag` and `tag_text_id` but does not locally attach template-param owner/index metadata.
- Dependent template-param application in `src/frontend/parser/impl/types/base.cpp:2840`, which returns a placeholder `TB_TYPEDEF` after parsing `T<...>` and should not rely on later rendered lookup.
- Template argument TypeSpec production in `src/frontend/parser/impl/types/declarator.cpp:1071`, where dependent type args are detected from rendered/tag text and should preserve intrinsic template-param metadata when the arg TypeSpec itself is a template parameter.

The executor should not treat the existing `TypeBindingNameTextMap` or `TypeBindingNameStructuredMap` in `src/frontend/sema/consteval.hpp:121` as completion. Those maps are rendered-name-to-metadata mirrors and belong to Step 3.2 deletion/collapse once intrinsic TypeSpec producer coverage is proven.

### Severity Medium: Local/Loop/Parameter/Synthetic Value Producers Are Mostly In Place, But Need One Metadata-Miss Test Before Route Deletion

For consteval function-body interpretation, value producers are mostly local to `src/frontend/sema/consteval.cpp`:

- Parameters are bound through `InterpreterBindings::bind()` in `src/frontend/sema/consteval.cpp:1224`.
- Local declarations are bound in `src/frontend/sema/consteval.cpp:1056`.
- Assignments and loop updates bind/update through `src/frontend/sema/consteval.cpp:952`, and loop bodies use the same `interp_stmt` path at `src/frontend/sema/consteval.cpp:1113`.
- `InterpreterBindings::bind()` records rendered, text, and structured local-key maps at `src/frontend/sema/consteval.cpp:739`.

This is in scope for Step 3.1, but the valid repair is not a broad rewrite. The next packet should prove and, if needed, tighten the smallest missing producer case, for example a same-spelling shadowed consteval parameter/local/for-init variable where `n->name` could still recover the wrong value after a structured or `TextId` miss. If the AST node lacks `unqualified_text_id`, repair the parser/Sema producer for that node; do not add a testcase-shaped branch inside `ConstEvalEnv::lookup()`.

Synthetic locals in Sema validation, such as `__func__`, `__FUNCTION__`, `__PRETTY_FUNCTION__`, and `this` at `src/frontend/sema/validate.cpp:1813`, are out of the consteval integer/value deletion path unless a consteval test demonstrates they are used by `evaluate_constant_expr()` as integer lookup inputs. Do not expand Step 3.1 into general Sema synthetic local semantics.

### Severity Medium: Sema Validation Local Const Producers Are Valid, But Should Not Be Mixed With Interpreter Local Producers

Sema validation already has parser/Sema-owned const-eval environment producers:

- `populate_const_eval_env()` wires global/local/enum rendered, text, and key maps at `src/frontend/sema/validate.cpp:956`.
- Global const bindings populate text/key maps at `src/frontend/sema/validate.cpp:979`.
- Local const bindings populate text/key maps at `src/frontend/sema/validate.cpp:990`.
- Local declaration validation evaluates and records local consts at `src/frontend/sema/validate.cpp:1930`.
- Range-for locals bind Sema local metadata at `src/frontend/sema/validate.cpp:2062`, but range-for consteval evaluation itself is explicitly unsupported in the interpreter at `src/frontend/sema/consteval.cpp:1151`.

Valid Step 3.1 repair: use these validation producer paths only for constant-expression validation and case/static-assert evaluation. Do not conflate them with the consteval function-body interpreter maps in `InterpreterBindings`; they have different lifetime/scope mechanics.

## Out-Of-Scope Routes To Park

- HIR `NttpBindings` migration and any deletion requiring `src/frontend/hir` metadata carriers. This belongs under `ideas/open/140_hir_legacy_string_lookup_metadata_resweep.md`.
- Removal of `TypeSpec::tag` as a field or cross-stage TypeSpec tag migration. This belongs under `ideas/open/141_typespec_tag_field_removal_metadata_migration.md`.
- General Sema owner/member/static rendered-route deletion outside consteval value/type producers. That is Step 3.3, not Step 3.1.
- Consteval function lookup API cleanup that deletes string-keyed `consteval_fns` compatibility. That is Step 3.2 after producer completeness is proven.
- HIR layout lookup for `sizeof`/`alignof` through `struct_defs` in `src/frontend/sema/consteval.cpp:247`; record layout carrier migration is not a parser/Sema Step 3.1 packet.

## Recommended First Executor Packet

Attempt the TypeSpec producer packet first:

1. Name the route: `TB_TYPEDEF` template-parameter TypeSpecs used by consteval `resolve_type()` must carry intrinsic owner/index/text metadata, so `lookup_type_binding_by_typespec_key()` can decide before rendered name mirrors.
2. Audit parser creation sites for template-param `TB_TYPEDEF` TypeSpecs, starting with `src/frontend/parser/impl/types/base.cpp:1689`, `src/frontend/parser/impl/types/base.cpp:2840`, and `src/frontend/parser/impl/types/declarator.cpp:1071`.
3. Reuse or localize the existing annotation logic from `src/frontend/parser/impl/declarations.cpp:25`; do not add a new rendered-name lookup wrapper.
4. Add a focused frontend/Sema test where a template type parameter TypeSpec has drift-prone rendered spelling but a valid intrinsic owner/index/text identity, and consteval `sizeof`/`alignof` or cast type resolution follows the intrinsic metadata.
5. Leave `ConstEvalEnv::lookup()` rendered compatibility intact unless the exact producer for the covered route is proven complete and the deletion belongs to the same narrow route.

Second packet, after the TypeSpec packet or if the supervisor chooses value-first:

1. Name the route: consteval function-body parameter/local/for-init bindings must populate `by_text` and `by_key` before `ConstEvalEnv::lookup(const Node*)`.
2. Prove the current `InterpreterBindings::bind()` coverage for parameters, local declarations, assignments, and for-init locals.
3. Patch only missing AST metadata producers (`unqualified_text_id` or local key availability) for nodes that lack metadata. Do not special-case a named testcase in `lookup()`.

## Judgments

Idea-alignment judgment: matches source idea.

Runbook-transcription judgment: plan matches idea.

Route-alignment judgment: narrow next packet. Step 3.1 is correctly scoped as producer repair, but execution must avoid jumping straight to Step 3.2 fallback deletion.

Technical-debt judgment: watch. The current coexistence of intrinsic TypeSpec metadata and rendered-name mirror maps is acceptable only as a transition state.

Validation sufficiency: needs broader proof for any code-changing Step 3.1 packet: fresh build plus focused frontend/Sema tests for same-feature drifted-string behavior, with canonical `test_after.log`.

Reviewer recommendation: narrow next packet. Start with the TypeSpec intrinsic metadata producer route unless supervisor has a stronger value-lookup failure reproducer ready.
