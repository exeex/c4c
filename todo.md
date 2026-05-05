# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Delete TypeSpec Tag And Validate

## Just Finished

Step 6's global namespace-qualified typedef-ref alias cast fallout is repaired
without reintroducing `TypeSpec::tag`.

The first bad boundary was the qualified type probe to declarator handoff:
`probe_qualified_type` resolved `::ns::AliasL` through the namespace-context
structured typedef key, but `try_parse_qualified_base_type` discarded that
resolved `TypeSpec` and tried to rebuild the payload from the spelled qualified
key. That left `using AliasL = int&` as an anonymous `TB_TYPEDEF`, so the
c-style cast lost the lvalue-reference carrier and selected the rvalue overload.

The fix carries the resolved typedef `TypeSpec` on `QualifiedTypeProbe` and
lets `try_parse_qualified_base_type` project that structured payload before
falling back to member-typedef or unresolved typedef metadata.

## Suggested Next

Next packet should target one of the two remaining Step 6 positive/Sema
failures: delegating constructor lookup for instantiated template constructors,
or consteval `sizeof(typename Outer<T>::Alias)` record-layout resolution.

## Watchouts

- Do not reintroduce `TypeSpec::tag` or rendered-string semantic lookup.
- The repaired path uses the resolved namespace-context typedef key as
  structured authority; it does not infer meaning from `::ns::AliasL` spelling.
- The two remaining failures still fail with their baseline symptoms:
  delegating constructor lookup reports no constructors for
  `eastl::pair_T1_T_T2_T`, and consteval member alias sizing reports unresolved
  record layout.

## Proof

Delegated proof command:
`cmake --build build && ctest --test-dir build -j --output-on-failure -R '^cpp_positive_sema_' > test_after.log 2>&1`

Result: monotonic improvement versus the delegated `test_before.log` baseline.

The proof improved the delegated subset from 881/884 passing to 882/884
passing. There are 0 new failures relative to `test_before.log`.

Fixed baseline failures:
- `cpp_positive_sema_c_style_cast_global_qualified_typedef_ref_alias_basic_cpp`

Remaining failures:
- `cpp_positive_sema_ctor_init_piecewise_delegating_template_runtime_cpp`
- `cpp_positive_sema_consteval_typespec_member_alias_cpp`

Canonical proof log: `test_after.log`.
