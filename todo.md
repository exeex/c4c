Status: Active
Source Idea Path: ideas/open/345_builtin_vrm_register_carrier_types_to_regalloc_frontier.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add Source Type Carrier And Frontend Proof

# Current Packet

## Just Finished

Step 2 added source-level VRM carrier identity and frontend/HIR proof for
`__c4c_builtin_vrm1`, `__c4c_builtin_vrm2`, `__c4c_builtin_vrm4`, and
`__c4c_builtin_vrm8`.

Implemented details:
- Added dedicated `TypeBase::TB_VRM_REGISTER` plus `TypeSpec::vrm_width`, so
  VRM carriers are not represented as scalar, pointer, struct, or GCC vector
  storage.
- Taught parser type-start detection and `parse_base_type()` to consume the
  four builtin identifier spellings directly.
- Preserved typedef VRM aliases through parser typedef binding, TypeSpec
  equality, HIR specialization ordering/hash, and canonical type conversion.
- Included parser/HIR compatibility and lookup-parity helpers in the
  `vrm_width` identity checks so different VRM widths cannot collapse together.
- Printed HIR carrier types as `c4c.vrm1`, `c4c.vrm2`, `c4c.vrm4`, and
  `c4c.vrm8`.
- Added `frontend_hir_tests` coverage for direct and typedef spellings across
  all four widths, including checks that GCC vector metadata remains unused.

## Suggested Next

Delegate Step 3 to `c4c-executor`: add the structured LIR type carrier and
HIR-to-LIR lowering for `c4c.vrmN`, with verifier/printer proof that the
carrier remains distinct from LLVM scalar, pointer, aggregate, and GCC vector
storage.

## Watchouts

- ABI and storage semantics are still intentionally unsupported after this
  slice: ordinary function parameter/return ABI, globals, static storage,
  alloca-backed storage, loads/stores, address-taking, arrays/members,
  initializers, arithmetic, casts, comparisons, lane operations, and final
  register allocation remain future work.
- The Step 2 proof uses function declarations/empty definitions only as a
  frontend/HIR type-surface probe. Do not treat it as backend ABI support.
- `src/codegen/lir/hir_to_lir/*` and backend BIR/prealloc surfaces still need
  the next packet before VRM carriers can lower beyond HIR.
- Do not reuse `is_vector`, `vector_lanes`, or `vector_bytes`; those remain GCC
  vector storage metadata.

## Proof

Ran delegated proof successfully:
`bash -lc 'cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R "^frontend_hir_tests$" >> test_after.log 2>&1'`.

Proof log: `test_after.log`.

Supervisor acceptance also ran:
- `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`
- `ctest --test-dir build -j --output-on-failure -R '^(frontend_hir_tests|frontend_parser_tests|frontend_parser_lookup_authority_tests|frontend_hir_lookup_tests|cpp_hir_)'`
