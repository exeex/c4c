#include "src/backend/mir/prepared_view.hpp"
#include "src/backend/prealloc/module.hpp"
#include "src/target_profile.hpp"

#include <iostream>
#include <string_view>
#include <utility>

namespace {

namespace bir = c4c::backend::bir;
namespace prepare = c4c::backend::prepare;
namespace prepared = c4c::backend::mir::prepared;

int fail(std::string_view message) {
  std::cerr << message << "\n";
  return 1;
}

bir::Value named(bir::TypeKind type, const char* name) {
  return bir::Value::named(type, name);
}

prepare::PreparedBirModule make_fixture() {
  prepare::PreparedBirModule module;
  module.target_profile = c4c::default_target_profile(c4c::TargetArch::X86_64);
  module.module.target_triple = "x86_64-test-c4c";

  const auto function_name = module.names.function_names.intern("main");
  const auto declaration_name = module.names.function_names.intern("extern_fn");
  const auto missing_fact_name = module.names.function_names.intern("missing_facts");
  const auto entry_label = module.names.block_labels.intern("entry");
  const auto value_name = module.names.value_names.intern("%v");
  const auto global_name = module.module.names.link_names.intern("global_i32");
  const auto block_label_id = module.module.names.block_labels.intern("entry");

  module.module.globals.push_back(bir::Global{
      .name = "global_i32",
      .link_name_id = global_name,
      .type = bir::TypeKind::I32,
      .size_bytes = 4,
      .align_bytes = 4,
  });
  module.module.string_constants.push_back(bir::StringConstant{
      .name = ".str0",
      .bytes = "abc",
  });

  bir::Block entry;
  entry.label = "entry";
  entry.label_id = block_label_id;
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = named(bir::TypeKind::I32, "%v"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::immediate_i32(1),
      .rhs = bir::Value::immediate_i32(2),
  });
  entry.terminator = bir::ReturnTerminator{.value = named(bir::TypeKind::I32, "%v")};

  bir::Function main;
  main.name = "main";
  main.return_type = bir::TypeKind::I32;
  main.blocks.push_back(std::move(entry));
  module.module.functions.push_back(std::move(main));

  bir::Function declaration;
  declaration.name = "extern_fn";
  declaration.return_type = bir::TypeKind::I32;
  declaration.is_declaration = true;
  module.module.functions.push_back(std::move(declaration));

  bir::Function missing_facts;
  missing_facts.name = "missing_facts";
  missing_facts.return_type = bir::TypeKind::I32;
  missing_facts.blocks.push_back(bir::Block{.label = "entry"});
  module.module.functions.push_back(std::move(missing_facts));

  module.control_flow.functions.push_back(prepare::PreparedControlFlowFunction{
      .function_name = function_name,
      .blocks = {prepare::PreparedControlFlowBlock{
          .block_label = entry_label,
          .terminator_kind = bir::TerminatorKind::Return,
      }},
  });
  module.value_locations.functions.push_back(prepare::PreparedValueLocationFunction{
      .function_name = function_name,
      .value_homes = {prepare::PreparedValueHome{
          .value_id = prepare::PreparedValueId{1},
          .function_name = function_name,
          .value_name = value_name,
          .kind = prepare::PreparedValueHomeKind::RematerializableImmediate,
          .immediate_i32 = 3,
      }},
  });
  module.addressing.functions.push_back(prepare::PreparedAddressingFunction{
      .function_name = function_name,
  });

  module.control_flow.functions.push_back(prepare::PreparedControlFlowFunction{
      .function_name = missing_fact_name,
      .blocks = {prepare::PreparedControlFlowBlock{.block_label = entry_label}},
  });

  module.completed_phases.push_back("phase-sentinel-not-core");
  module.notes.push_back(prepare::PrepareNote{
      .phase = "diagnostic-phase-sentinel",
      .message = "prepare-note-sentinel-not-core",
  });
  if (declaration_name == c4c::kInvalidFunctionName) {
    module.notes.push_back(prepare::PrepareNote{
        .phase = "test",
        .message = "unreachable marker to keep declaration id live",
    });
  }
  return module;
}

int verify_core_view() {
  const auto module = make_fixture();
  const prepared::PreparedMirCoreView view(module);

  if (view.target_profile().arch != c4c::TargetArch::X86_64 ||
      view.target_triple() != "x86_64-test-c4c") {
    return fail("expected target identity to come from prepared module facts");
  }
  if (view.functions().size() != 3 || view.defined_functions().size() != 2) {
    return fail("expected all functions and declaration-filtered traversal");
  }
  if (view.globals().size() != 1 || view.string_constants().size() != 1) {
    return fail("expected module data accessors to expose read-only BIR data");
  }

  const auto main_id = view.resolve_function_name("main");
  if (!main_id.has_value() || view.function_name(*main_id) != "main") {
    return fail("expected prepared function-name resolution");
  }
  if (view.resolve_function_name("absent").has_value() ||
      view.function_view("absent").has_value()) {
    return fail("expected missing names to fail closed");
  }
  if (view.function_view("extern_fn").has_value()) {
    return fail("expected declarations to be excluded from function views");
  }
  if (view.function_view("missing_facts").has_value()) {
    return fail("expected function views to require core prepared facts");
  }

  const auto function_view = view.function_view(*main_id);
  if (!function_view.has_value() || !*function_view) {
    return fail("expected complete defined function to produce a function view");
  }
  if (&function_view->bir_function() != view.bir_function(*main_id) ||
      &function_view->control_flow() != view.control_flow(*main_id)) {
    return fail("expected function view to reference prepared module facts");
  }
  if (function_view->function_name_text() != "main" ||
      function_view->value_locations().value_homes.size() != 1 ||
      function_view->addressing().function_name != *main_id) {
    return fail("expected per-function core facts through the view");
  }
  if (function_view->blocks().size() != 1 ||
      function_view->blocks().front().block_label == c4c::kInvalidBlockLabel ||
      function_view->blocks().front().block == nullptr ||
      function_view->blocks().front().control_flow == nullptr) {
    return fail("expected block view to bind BIR and prepared control-flow facts");
  }

  const auto cursor = function_view->instruction(0, 0);
  if (!cursor.has_value() ||
      cursor->function_name != *main_id ||
      cursor->block == nullptr ||
      cursor->instruction == nullptr ||
      cursor->prepared_block == nullptr) {
    return fail("expected instruction cursor to bind BIR and prepared coordinates");
  }
  if (function_view->instruction(0, 1).has_value() ||
      function_view->instruction(1, 0).has_value()) {
    return fail("expected out-of-range instruction lookups to fail closed");
  }

  const auto value_name = module.names.value_names.find("%v");
  const auto* value_home =
      prepare::find_indexed_prepared_value_home(&function_view->prepared_lookups().value_homes,
                                                nullptr,
                                                &function_view->value_locations(),
                                                value_name);
  if (value_home == nullptr || value_home->immediate_i32 != 3) {
    return fail("expected view-owned prepared lookup cache to agree with value homes");
  }

  return 0;
}

}  // namespace

int main() {
  return verify_core_view();
}
