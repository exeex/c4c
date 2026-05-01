# Step 2.4.4.5A Alias-Template Carrier Review

Active source idea path: `ideas/open/139_parser_sema_rendered_string_lookup_removal.md`

Report path: `review/step2_4_5a_alias_template_carrier_review.md`

Chosen base commit: `c16dd5c069cfe9be7a0b0d5dd71148a967bc16ed` (`94fea4376^`, `Narrow non-template member typedef mirror`)

Reason: the delegated review explicitly requested the diff from before `94fea4376 Construct alias template member typedef carrier` through `HEAD`. `94fea4376^` resolves unambiguously to `c16dd5c`, and the two reviewed commits are `94fea4376` and `8d4bb9d`.

Commit count since base: 2

Reviewed head: `8d4bb9daebcddec155faad8b6a9579e147cb2ba5`

## Findings

### Medium: canonical proof artifact named by `todo.md` is missing

`todo.md` says the delegated proof wrote `test_after.log` and passed the build plus `cpp_positive_sema_` subset, including the new carrier tests. The working tree currently has `test_before.log` but no `test_after.log`, so the review cannot independently confirm the fresh executor proof from the canonical artifact.

Evidence:

- `todo.md:44` records `cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^cpp_positive_sema_' >> test_after.log 2>&1`.
- `todo.md:48` records the result as passed.
- `ls -l test_before.log test_after.log` showed only `test_before.log`.

Impact: this does not invalidate the structural route review, but acceptance should recreate or restore the canonical `test_after.log` before treating Step 2.4.4.5A as fully proof-backed.

### Low: retained rendered/deferred `TypeSpec` projection is still present as the Step 2.4.4.5B bridge

The producer still projects the structured carrier into the old deferred `TypeSpec` representation via `apply_alias_template_member_typedef_compat_type`, including `render_name_in_context(...)` for the owner spelling and `parser_text(...)` for the member spelling. The consumer also has a dependent-argument deferral branch that renders owner/member text back into `ts.tpl_struct_origin` and `ts.deferred_member_type_name`.

Evidence:

- `src/frontend/parser/impl/declarations.cpp:257` defines `apply_alias_template_member_typedef_compat_type`.
- `src/frontend/parser/impl/declarations.cpp:265` renders the owner name from `info.owner_key`.
- `src/frontend/parser/impl/declarations.cpp:294` writes `deferred_member_type_name`.
- `src/frontend/parser/impl/types/base.cpp:2131` enters the still-dependent branch.
- `src/frontend/parser/impl/types/base.cpp:2132` renders the owner name and `src/frontend/parser/impl/types/base.cpp:2153` writes the deferred member type.

Impact: this is not the accepted success route for the tested concrete alias-template member typedef cases, because `resolve_structured_alias_member_type()` runs before the legacy alias substitution fallback and resolves concrete owners through `TemplateInstantiationKey + member TextId`. It is still exactly the compatibility bridge that Step 2.4.4.5B must delete or replace. Do not count this projection as bridge deletion progress.

## Structural Checks

Carrier construction before flattening: passes.

- `src/frontend/parser/impl/declarations.cpp:1497` records `alias_type_pos`.
- `src/frontend/parser/impl/declarations.cpp:1498` calls `try_parse_alias_template_member_typedef_info(parser)` before `parser.parse_type_name()` at `src/frontend/parser/impl/declarations.cpp:1502`.
- `src/frontend/parser/impl/declarations.cpp:166` parses `typename Owner<Args>::member` under a tentative guard and records the carrier before the rendered/deferred `TypeSpec` projection can flatten it.

Structured carrier contents: passes.

- `src/frontend/parser/parser_types.hpp:154` defines `ParserAliasTemplateMemberTypedefInfo` with `QualifiedNameKey owner_key`, structured `owner_args`, and `TextId member_text_id`.
- `src/frontend/parser/impl/declarations.cpp:198` derives the owner `QualifiedNameKey`.
- `src/frontend/parser/impl/declarations.cpp:202` through `src/frontend/parser/impl/declarations.cpp:205` store validity, owner key, parsed owner args, and member `TextId`.
- `src/frontend/parser/impl/declarations.cpp:1541` stores the carrier in parser active context state, and `src/frontend/parser/impl/declarations.cpp:1982` copies it into `ParserAliasTemplateInfo`.

Alias instantiation uses the structured carrier first: passes.

- `src/frontend/parser/impl/types/base.cpp:2096` defines `resolve_structured_alias_member_type`.
- `src/frontend/parser/impl/types/base.cpp:2106` finds the owner primary by `ati->member_typedef.owner_key`.
- `src/frontend/parser/impl/types/base.cpp:2117` through `src/frontend/parser/impl/types/base.cpp:2129` substitutes parsed carrier owner args through alias actuals.
- `src/frontend/parser/impl/types/base.cpp:2162` through `src/frontend/parser/impl/types/base.cpp:2173` builds `TemplateInstantiationKey { owner_key, argument_keys }` and resolves the member typedef by `member_text_id`.
- `src/frontend/parser/impl/types/base.cpp:2182` through `src/frontend/parser/impl/types/base.cpp:2217` falls back to selecting the owner primary/specialization and matching member typedefs by member `TextId`, not by rendered owner spelling.
- `src/frontend/parser/impl/types/base.cpp:2221` through `src/frontend/parser/impl/types/base.cpp:2224` runs this structured resolver before the old alias-argument substitution fallback.

Qualified alias relay avoidance: passes for this route.

- `src/frontend/parser/impl/types/base.cpp:1611` derives the alias lookup key from saved structured type metadata.
- `src/frontend/parser/impl/types/base.cpp:1667` probes `find_alias_template_info(alias_key)` before the fallback `find_alias_template_info_in_context(...)` at `src/frontend/parser/impl/types/base.cpp:1674`.
- The new qualified test distinguishes `lib::select_t<int, short>` from `::select_t<int, short>`, which would fail if the normal route relayed through current-context rendered spelling.

String-derived carrier seeding / local alias-of-alias parser: passes.

- The reviewed diff deletes the local token splitter/encoder in `declarations.cpp` that previously reconstructed owner/member information from token spellings.
- The remaining carrier producer uses `consume_qualified_type_spelling_with_typename`, `parse_template_argument_list`, `QualifiedNameKey`, structured `TemplateArgParseResult`, and member `TextId`.
- I did not find a narrow parser block that compensates for missing carrier data by reparsing rendered alias-of-alias text in the reviewed route.

Debug-text argument parsing on the normal route: passes with a caveat.

- The normal structured carrier resolver substitutes `ParserTemplateArgParseResult` values and types directly.
- Existing debug-text parsing helpers remain elsewhere in `types/base.cpp` and HIR code, and `alias_template_arg_debug_text` still populates the compatibility `TypeSpec` bridge. Those are not the success route for the Step 2.4.4.5A concrete carrier resolution.

Test shape: acceptable for Step 2.4.4.5A.

- `tests/cpp/internal/postive_case/runtime/template_alias_member_typedef_structured_carrier_runtime.cpp:1` tests a carrier where rendered/deferred alias shape would choose the wrong argument order if the carrier is not preserved.
- `tests/cpp/internal/postive_case/runtime/template_alias_member_typedef_qualified_carrier_runtime.cpp:1` tests namespace/current-context disagreement and global-vs-qualified alias selection.
- I found no expectation downgrade or named-case shortcut in the reviewed diff.

## Judgments

Idea-alignment judgment: matches source idea.

Runbook-transcription judgment: plan matches idea.

Route-alignment judgment: on track.

Technical-debt judgment: watch.

Validation sufficiency: needs broader proof, specifically fresh canonical `test_after.log` for the stated build plus `cpp_positive_sema_` subset before acceptance.

Reviewer recommendation: continue current route after restoring fresh proof. Step 2.4.4.5A is structurally ready for Step 2.4.4.5B bridge deletion, provided Step 2.4.4.5B treats the remaining rendered/deferred `TypeSpec` projection as the bridge to delete rather than as already-accepted structured progress.
