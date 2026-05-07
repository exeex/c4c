Status: Active
Source Idea Path: ideas/open/146_qualified_name_deferred_carrier_authority.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Qualified-Name Authority

# Current Packet

## Just Finished

Lifecycle activation created `plan.md` from
`ideas/open/146_qualified_name_deferred_carrier_authority.md` and initialized
canonical execution state for Step 1.

## Suggested Next

Executor should run Step 1 from `plan.md`: inventory every parser, Sema,
shared, and HIR path that accepts one `TextId` whose spelling may contain `::`
and uses it for semantic lookup. Record the classification in this file and
name the first lookup family to migrate.

## Watchouts

Do not treat one `TextId` containing `A::B::C` as equivalent to a structured
qualified-name key. Do not claim progress by renaming rendered-string helpers
while still resolving semantic identity by rendering and splitting qualified
spelling. Do not weaken tests, mark qualified-name paths unsupported, or add
named-case shortcuts.

## Proof

Expected Step 1 proof is inventory-only unless the supervisor delegates a small
classification documentation update. If implementation files are touched, run a
fresh build or compile proof plus the exact narrow command delegated by the
supervisor and write the result to `test_after.log`.
