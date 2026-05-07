# Current Packet

Status: Active
Source Idea Path: ideas/open/149_template_instantiation_structured_argument_key.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Template Argument String Authority

## Just Finished

Lifecycle activation created the active runbook and initialized this execution
scratchpad from `ideas/open/149_template_instantiation_structured_argument_key.md`.

## Suggested Next

Begin Step 1 by inventorying `TemplateInstantiationKey::Argument`,
`canonical_key`, `make_template_instantiation_argument_key`, and all reachable
construction/comparison sites that can treat rendered or debug text as semantic
template argument identity.

## Watchouts

- Do not treat raw `TextId` spelling as the full semantic template argument key.
- Do not weaken tests or mark supported cases unsupported as a shortcut around
  structured argument identity.
- Keep rendered argument text display-only or compatibility-only unless the
  source idea is explicitly revised.

## Proof

Lifecycle-only activation. No build or test proof was run.
