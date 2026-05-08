Status: Active
Source Idea Path: ideas/open/152_hir_lir_rendered_owner_compatibility_retirement.md
Source Plan Path: plan.md
Current Step ID: 7
Current Step Title: Validate And Review Remaining Compatibility

# Current Packet

## Just Finished

Completed Plan Step 7 inventory after the Step 6 commits.

Re-inventoried the touched HIR/LIR rendered-owner and aggregate compatibility routes. No new testcase-overfit, expectation downgrade, unsupported rewrite, parser-route drift, or broad unrelated lowering rewrite was found. The current code prefers structured owner keys, record definitions, structured LIR `StructNameId`s, or owner-indexed module entries before rendered compatibility in the targeted routes.

Remaining compatibility owners and removal conditions:

- HIR owner-key bridge: `typespec_aggregate_owner_key(ts, mod)` still uses rendered aggregate spelling to repair parser/module text-table identity when a direct structured key misses or collides. Limitation: the rendered spelling is a cross-table carrier and is accepted only when it maps back to an indexed module record. Removal condition: aggregate `TypeSpec` producers carry module-canonical owner keys or record definitions at the boundary.
- HIR/LIR aggregate layout bridge: `find_typespec_aggregate_layout` and `lookup_structured_layout` still have compatibility tag lookup for no-owner or incomplete-owner aggregate carriers. Limitation: complete owner-key misses stop instead of substituting a rendered record. Removal condition: all aggregate `TypeSpec`s carry complete owner metadata and all layouts are owner-indexed.
- LIR lowering: call, signature, global, indexed GEP, const-init, variadic, va_arg, field-chain, and template-local aggregate ABI paths now prefer structured aggregate names or child `StructNameId`s. Remaining rendered compatibility is secondary for legacy carriers with no usable owner key; `_tag_ctx`/`_local_text` canonicalization is constrained to a unique canonical template instantiation candidate. Removal condition: HIR export supplies complete structured aggregate names and owner keys for every aggregate use, including anonymous union child layouts.
- HIR member typedef and signature recovery: owner-key and text-id paths run before rendered member typedef recovery. The remaining rendered bridge exists because module member typedef/base-walk storage is still keyed by rendered tags after a structured owner has selected the record. Removal condition: member typedef and base-walk storage become owner-indexed, and deferred member typedef producers always carry `deferred_member_type_owner_key`, qualifier `TextId`s, and member `TextId`s.
- HIR declaration/object/member-expression compatibility: rendered aggregate comparison or generated member payload extraction is guarded behind missing structured owner identity or missing unqualified member payload. Removal condition: those producers always carry complete owner and member text metadata.

## Suggested Next

Plan-owner should close idea 152 rather than keep it active for more packets. The remaining compatibility is documented, secondary, and covered by concrete removal conditions; further deletion would be a separate cleanup initiative, not required for this plan's acceptance criteria.

## Watchouts

- Do not treat the remaining compatibility helpers as primary semantic authority in future work; they are acceptance-bounded fallbacks for incomplete metadata or legacy module storage.
- Full deletion should be split into a follow-up idea only if the project wants to owner-index the remaining module storage and remove no-owner aggregate carrier support.
- The anonymous union child-layout limitation remains the most concrete future removal blocker: current LIR layout metadata still cannot recover child layout ids for union anonymous-member recursion.

## Proof

No new test command was delegated or required for this inventory-only packet. Accepted supervisor evidence already present:

- focused Step 6 proof passed 7/7 with regression guard
- broader `ctest --test-dir build -R '^cpp_' --output-on-failure` passed 1147/1147
- full-suite `test_baseline.log` at commit `6edf2fba7` is green 3023/3023

`test_after.log` was not modified by this packet.
