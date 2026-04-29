Status: Active
Source Idea Path: ideas/open/134_parser_ast_template_payload_string_bridge_cleanup.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Classify AST and Template Payload String Authority

# Current Packet

## Just Finished

Activated idea 134 `Parser AST Template Payload String Bridge Cleanup` into
`plan.md`.

## Suggested Next

Execute `plan.md` Step 1 as a classification-only packet. Inspect parser
writes to alias-template payload strings, NTTP/template argument text,
`TemplateArgRef::debug_text`, AST `Node::*` spelling fields, and
`TypeSpec::*` template/deferred-member fields; trace the first Sema/HIR
consumers; classify each path as structured-primary bridge, display-only,
diagnostic/debug-only, generated spelling, fallback-only, or semantic string
authority. Record the first owned implementation packet in `todo.md`.

## Watchouts

Do not edit implementation files during classification unless the supervisor
explicitly delegates a code packet. Do not remove AST display fields or
diagnostic/debug strings before downstream consumers are migrated.

## Proof

Lifecycle activation only; no build or test proof required.
