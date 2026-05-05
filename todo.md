# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Delete TypeSpec Tag And Validate

## Just Finished

Step 6's consteval member-alias `sizeof` fallout is repaired without
reintroducing `TypeSpec::tag`.

The first bad boundary was the consteval type resolver for
`typename Outer<T>::Alias`: the parsed `TypeSpec` carried structured template
owner metadata (`tpl_struct_origin_key`, concrete template args, and
`deferred_member_type_text_id`), but the evaluator only substituted direct
`TB_TYPEDEF` template params before calling `sizeof`. It therefore treated the
owner struct as the sized type and failed record-layout lookup.

The fix gives `ConstEvalEnv` a structured template-primary lookup hook and lets
`resolve_type` reduce record member typedef carriers before layout queries. The
member alias is matched by parser text id when present, the owner template
params are rebound from the concrete owner args, and the alias target is then
resolved through the existing consteval type-binding maps.

## Suggested Next

Next packet should target the final Step 6 positive/Sema failure: delegating
constructor lookup for instantiated template constructors.

## Watchouts

- Do not reintroduce `TypeSpec::tag` or rendered-string semantic lookup.
- The consteval template-primary lookup uses structured owner keys. Qualified
  origins with qualifier-path ids are still guarded rather than recovered from
  rendered spelling.
- The remaining constructor failure still reports no constructors for
  `eastl::pair_T1_T_T2_T`.

## Proof

Delegated proof command:
`cmake --build build && ctest --test-dir build -j --output-on-failure -R '^cpp_positive_sema_' > test_after.log 2>&1`

Result: monotonic improvement versus the delegated `test_before.log` baseline.

The proof improved the delegated subset from 882/884 passing to 883/884
passing. There are 0 new failures relative to `test_before.log`.

Fixed baseline failures:
- `cpp_positive_sema_consteval_typespec_member_alias_cpp`

Remaining failures:
- `cpp_positive_sema_ctor_init_piecewise_delegating_template_runtime_cpp`

Canonical proof log: `test_after.log`.
