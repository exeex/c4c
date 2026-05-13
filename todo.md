Status: Active
Source Idea Path: ideas/open/200_hir_legacy_compatibility_retirement.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Fence Lowerer Local, Param, Static, Label, And Signature Mirrors

# Current Packet

## Just Finished

Step 4 repair completed for the broad regressions from commit `7e67958fb`.
Local, param, static/global, generic type inference, and function-pointer
fallbacks now accept explicit compatibility through either the source `TextId`
or the HIR-canonical spelling `TextId`, while name-only fallback remains limited
to no-metadata references. Range-for body variables now populate
`local_ids_by_text_id`, so source range variables no longer depend on the stale
rendered local-name bridge.

## Suggested Next

Continue Step 4 with local const binding maps, labels, and any remaining
function prototype or signature mirrors that still allow stale rendered names to
override complete source metadata.

## Watchouts

- The compatibility helper treats a valid source `TextId` as complete enough to
  reject name-only rendered compatibility; callers must insert an explicit
  `rendered_compat_*_text_ids` entry when a valid-`TextId` generated bridge is
  intentional. Generated bridges may use the HIR-canonical spelling `TextId`
  because parser-owned `TextId` values do not necessarily share the HIR
  link-name table.
- Static/global and local const bridges already have nearby focused tests; keep
  the next packet semantic rather than expectation-only.

## Proof

Passed: `bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(frontend_hir_lookup_tests|string_authority_guard|cpp_positive_sema_template_operator_call_rvalue_ref_runtime_cpp|cpp_positive_sema_fixed_vec_smoke_cpp|cpp_positive_sema_operator_this_out_of_class_runtime_cpp|cpp_positive_sema_range_for_(auto|basic|const|const_auto)_cpp|cpp_positive_sema_template_(deferred_method|struct_method|temporary_call)_cpp|cpp_hir_template_struct_body_instantiation|eastl_cpp_external_(piecewise_construct|utility)_frontend_basic_cpp|llvm_gcc_c_torture_src_(930930_1|20000112_1|20000314_3|20000808_1|20000815_1|20000914_1|20001026_1|20001101|920501_2|920612_1|921013_1|921124_1|930513_2|930630_1|930718_1|930719_1|931005_1|931017_1|931031_1|931102_1|931102_2|940115_1|941021_1|950221_1|960209_1|960419_1|960513_1|960608_1|961026_1|980506_1|980526_3|981130_1|cmpdi_1|dbra_1|ieee_920810_1|int_compare|loop_2|loop_2d|loop_3b|loop_3|loop_3c|mod_1|strct_pack_3|tstdi_1)_c)$"' > test_after.log 2>&1`

Proof log: `test_after.log`
