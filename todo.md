Status: Active
Source Idea Path: ideas/open/83_parser_scope_textid_binding_lookup.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory parser binding tables

# Current Packet

## Just Finished
Completed plan Step 1 by inventorying parser binding tables and classifying the
main migration targets into `TextId`-native, direct `TextId` replacements,
scope-local `LocalNameTable` candidates, sequence/path-key cases, and
namespace-owned tables that must stay separate from lexical lookup.

## Suggested Next
Start Step 2 by adding explicit parser lexical scope state with push/pop
behavior for the simplest unqualified local bindings, using
`LocalNameTable`-backed visibility that stays separate from namespace
traversal state.

## Watchouts
Do not collapse namespace traversal into lexical lookup. Keep
single-segment semantic names on `TextId`, reserve sequence/path ids for
multi-segment identities, and avoid growing new parser-wide flat tables for
bindings that should follow lexical scope lifetime.

## Proof
No build/test proof for this packet. The completion artifact is the inventory
captured in `ideas/open/83_parser_scope_textid_binding_lookup.md`; no code
paths changed and no `test_after.log` was required.
