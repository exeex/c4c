Status: Active
Source Idea Path: ideas/open/102_hir_struct_method_member_identity_dual_lookup.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Define HIR Record-Owner Key Shape

# Current Packet

## Just Finished

Completed Step 2 by defining the HIR record-owner structured key contract in `src/frontend/hir/hir_ir.hpp`.

Added:
- `HirRecordOwnerKeyKind`, `HirRecordOwnerTemplateIdentity`, `HirRecordOwnerKey`, and `HirRecordOwnerKeyHash` for future semantic record-owner maps.
- `HirRecordOwnerKey::debug_label()` for map diagnostics and parity/mismatch messages without requiring rendered output changes.
- Construction helpers from `NamespaceQualifier` plus declaration `TextId`, from `HirStructDef`, and template-instantiation helpers carrying explicit primary-declaration/specialization bridge identity.

Existing rendered-name maps, lookup behavior, HIR dumps, codegen spelling, tests, and expectation files were untouched.

## Suggested Next

Proceed to Step 3 by dual-writing the first structured mirror beside `Module::struct_defs`/`struct_def_order` and the lowerer struct-definition registration path, while keeping rendered-name lookup as the authority until parity is proven.

## Watchouts

Keep `TypeSpec::tag`, `HirStructDef::tag`, `Module::struct_defs`, and `struct_def_order` as the rendered output-spelling bridge. The new key intentionally excludes rendered tags; Step 3 should populate a parallel mirror and mismatch/debug labels rather than redirecting existing codegen or dump consumers.

## Proof

Ran exactly `(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -L hir) > test_after.log 2>&1`.

Result: passed. `test_after.log` reports 73/73 HIR-labeled tests passing.
