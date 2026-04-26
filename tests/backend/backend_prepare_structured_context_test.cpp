#include "src/backend/bir/bir.hpp"
#include "src/backend/bir/lir_to_bir.hpp"
#include "src/codegen/lir/ir.hpp"
#include "src/shared/text_id_table.hpp"
#include "src/target_profile.hpp"

#include <iostream>
#include <memory>
#include <string>

namespace {

namespace bir = c4c::backend::bir;
namespace lir = c4c::codegen::lir;

int fail(const char* message) {
  std::cerr << message << "\n";
  return 1;
}

lir::LirModule make_structured_decl_lir_module() {
  lir::LirModule module;
  module.target_profile = c4c::target_profile_from_triple("x86_64-unknown-linux-gnu");
  module.link_name_texts = std::make_shared<c4c::TextTable>();
  module.link_names.attach_text_table(module.link_name_texts.get());
  module.struct_names.attach_text_table(module.link_name_texts.get());

  const c4c::StructNameId pair_id = module.struct_names.intern("%struct.Pair");
  const c4c::StructNameId nested_id = module.struct_names.intern("%struct.Nested");

  module.record_struct_decl(lir::LirStructDecl{
      .name_id = pair_id,
      .fields = {lir::LirStructField{lir::LirTypeRef("i32")},
                 lir::LirStructField{lir::LirTypeRef("i64")}},
  });
  module.record_struct_decl(lir::LirStructDecl{
      .name_id = nested_id,
      .fields = {lir::LirStructField{lir::LirTypeRef("%struct.Pair")}},
      .is_packed = true,
  });
  module.type_decls.push_back("%struct.Pair = type { i32, i64 }");
  module.type_decls.push_back("%struct.Nested = type <{ %struct.Pair }>");

  lir::LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()";
  function.return_type = c4c::TypeSpec{.base = c4c::TB_INT};

  lir::LirBlock entry;
  entry.label = "entry";
  entry.terminator = lir::LirRet{
      .value_str = std::string("0"),
      .type_str = "i32",
  };
  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

int check_structured_context_population() {
  const auto lir_module = make_structured_decl_lir_module();
  const auto lowered =
      c4c::backend::try_lower_to_bir_with_options(lir_module, c4c::backend::BirLoweringOptions{});
  if (!lowered.module.has_value()) {
    return fail("structured declaration fixture did not lower to BIR");
  }

  const auto& context = lowered.module->structured_types;
  if (context.declarations.size() != 2) {
    return fail("BIR structured type spelling context did not preserve declaration count");
  }

  const bir::StructuredTypeDeclSpelling* pair = context.find_struct_decl("%struct.Pair");
  if (pair == nullptr || pair->is_packed || pair->is_opaque || pair->fields.size() != 2 ||
      pair->fields[0].type_name != "i32" || pair->fields[1].type_name != "i64") {
    return fail("BIR structured type spelling context did not preserve Pair spelling metadata");
  }

  const bir::StructuredTypeDeclSpelling* nested = context.find_struct_decl("%struct.Nested");
  if (nested == nullptr || !nested->is_packed || nested->is_opaque ||
      nested->fields.size() != 1 || nested->fields[0].type_name != "%struct.Pair") {
    return fail("BIR structured type spelling context did not preserve Nested spelling metadata");
  }

  const std::string printed = bir::print(*lowered.module);
  if (printed.find("%struct.Pair") != std::string::npos ||
      printed.find("%struct.Nested") != std::string::npos) {
    return fail("BIR printer started rendering structured context before the render step");
  }
  return 0;
}

int check_call_return_rendering_prefers_structured_context() {
  bir::Module module;
  module.structured_types.declarations.push_back(bir::StructuredTypeDeclSpelling{
      .name = "%struct.Pair",
      .fields = {bir::StructuredTypeFieldSpelling{.type_name = "i32"},
                 bir::StructuredTypeFieldSpelling{.type_name = "i32"}},
  });

  bir::Function function;
  function.name = "main";
  function.return_type = bir::TypeKind::Void;

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::CallInst{
      .callee = "make_pair",
      .args = {bir::Value::named(bir::TypeKind::Ptr, "%ret.sret")},
      .arg_types = {bir::TypeKind::Ptr},
      .arg_abi = {bir::CallArgAbiInfo{
          .type = bir::TypeKind::Ptr,
          .size_bytes = 8,
          .align_bytes = 4,
          .primary_class = bir::AbiValueClass::Memory,
          .sret_pointer = true,
      }},
      .structured_return_type_name = "%struct.Pair",
      .return_type_name = "legacy-return-text-should-not-render",
      .return_type = bir::TypeKind::Void,
      .result_abi = bir::CallResultAbiInfo{
          .type = bir::TypeKind::Void,
          .primary_class = bir::AbiValueClass::Memory,
          .returned_in_memory = true,
      },
  });
  entry.terminator = bir::ReturnTerminator{};
  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));

  const std::string printed = bir::print(module);
  if (printed.find("bir.call void make_pair(ptr sret(size=8, align=4) %ret.sret)") ==
      std::string::npos) {
    return fail("BIR call return rendering did not use structured context for sret void spelling");
  }
  if (printed.find("legacy-return-text-should-not-render") != std::string::npos) {
    return fail("BIR call return rendering used legacy return_type_name before structured context");
  }
  return 0;
}

}  // namespace

int main() {
  if (const int status = check_structured_context_population(); status != 0) {
    return status;
  }
  return check_call_return_rendering_prefers_structured_context();
}
