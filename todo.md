Status: Active
Source Idea Path: ideas/open/155_template_scope_owner_and_member_substitution_domain_keys.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Template Owner And Substitution String Authority

# Current Packet

## Just Finished

No executor packet has run for this plan yet.

## Suggested Next

Start Step 1 by inventorying `ParserTemplateScopeFrame::owner_struct_tag`,
the NTTP array-size substitution loops in
`src/frontend/parser/impl/types/base.cpp`, and member typedef
instantiation/lookup paths that still compare rendered owner or member names.
Record the first code packet target and focused proof command here before
implementation.

## Watchouts

- Do not treat rendered template owner/member/NTTP names as semantic authority
  when owner, parameter-domain, or member `TextId` metadata exists.
- Do not weaken tests or add named-fixture shortcuts to make a narrow case
  pass.
- Keep `owner_struct_tag`, syntax payloads, and display strings only as
  bounded compatibility/debug/final-spelling paths unless no structured
  carrier exists.

## Proof

Lifecycle activation only; no build or test proof was run.
