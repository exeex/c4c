Status: Active
Source Idea Path: ideas/open/150_nttp_type_binding_domain_key_contract.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Repair HIR Late Substitution Domain Carriers

# Current Packet

## Just Finished

Reviewer report `review/step4_hir_late_substitution_review.md` rejected the completed Step 4 route as still drifting from the source idea.

- The blocking issue is HIR NTTP late substitution still treating `TemplateArgRef::nttp_text_id` as enough authority, then resolving through the current primary parameter table and legacy `nttp_bindings` string map.
- Type substitution has a related missing-owner `TextId` acceptance path that must be removed or explicitly classified as temporary compatibility.
- The `Args1#0` / `Base#N` pack bridge is acceptable only as temporary compatibility for legacy explicit pack-series maps; it is not final domain-key authority.

## Suggested Next

Next coherent packet: execute rewritten `plan.md` Step 4, "Repair HIR Late Substitution Domain Carriers".

Required outcome before further cleanup:

- Add or route a real NTTP parameter-domain carrier through `TemplateArgRef` or an adjacent HIR binding carrier.
- The carrier must include owner template context/key, parameter index, parameter kind, spelling `TextId`, and structured value/deferred payload, or an equivalent HIR binding object that provides that authority.
- HIR NTTP late substitution must not accept `TextId` alone, rendered parameter-name strings, `TemplateArgRef::debug_text`, or current-primary name lookup as semantic authority.
- Inventory `find_template_typedef_binding` and any remaining HIR `type_bindings.find(key)`, `nttp_bindings.find(key)`, missing-owner `TextId`, or debug-text lookup in the touched paths; remove them or label them compatibility-only with a removal condition.
- Leave Step 5 tests/probes blocked until Step 4 has the real NTTP domain carrier.

## Watchouts

- `TextId` is spelling identity only. It can be stored in the carrier, but cannot be the binding key by itself.
- The `Args1#0` / `Base#N` pack bridge may remain only as legacy explicit-pack compatibility with a documented removal condition.
- Debug text and rendered strings are allowed for diagnostics, dumps, syntax compatibility, or transitional adapters only.
- Do not claim progress through expectation downgrades, unsupported conversions, named-case shortcuts, or helper renames that preserve the old spelling-only authority.

## Proof

Pending. Supervisor should delegate a fresh build/proof command with the Step 4 carrier-repair packet.
