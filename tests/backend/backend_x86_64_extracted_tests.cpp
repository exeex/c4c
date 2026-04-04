#include "backend.hpp"
#include "../../src/backend/ir.hpp"
#include "../../src/backend/lowering/call_decode.hpp"
#include "../../src/backend/elf/mod.hpp"
#include "../../src/backend/linker_common/mod.hpp"
#include "../../src/backend/x86/assembler/mod.hpp"
#include "../../src/backend/x86/codegen/emit.hpp"

#include <algorithm>
#include <string>

#include "backend_test_helper.hpp"

void test_x86_64_backend_call_helpers_parse_structured_declared_direct_call_module() {
  auto lowered =
      c4c::backend::lower_lir_to_backend_module(make_x86_extern_decl_inferred_param_module());

  c4c::backend::BackendFunction* decl = nullptr;
  c4c::backend::BackendFunction* main_fn = nullptr;
  for (auto& function : lowered.functions) {
    if (function.signature.name == "main") {
      main_fn = &function;
    } else {
      decl = &function;
    }
  }
  expect_true(decl != nullptr && main_fn != nullptr,
              "x86_64 structured declared direct-call module parser regression test needs lowered declaration and main functions");
  if (decl == nullptr || main_fn == nullptr || main_fn->blocks.empty() ||
      main_fn->blocks.front().insts.empty()) {
    return;
  }

  decl->signature.name = "puts_like";
  decl->signature.params[0].name = "%p.left";
  decl->signature.params[1].name = "%p.right";
  decl->signature.is_vararg = true;

  auto* call =
      std::get_if<c4c::backend::BackendCallInst>(&main_fn->blocks.front().insts.front());
  expect_true(call != nullptr,
              "x86_64 structured declared direct-call module parser regression test needs the lowered main call instruction");
  if (call == nullptr) {
    return;
  }

  call->callee.symbol_name = "puts_like";
  call->result = "%t.main.puts_like.structured";
  call->args[0].operand = "17";
  call->args[1].operand = "23";
  main_fn->blocks.front().terminator.value = "29";

  const auto parsed =
      c4c::backend::parse_backend_minimal_declared_direct_call_module(lowered);
  expect_true(parsed.has_value() && parsed->callee != nullptr && parsed->main_function != nullptr &&
                  parsed->main_block != nullptr && parsed->call != nullptr &&
                  parsed->callee->signature.name == "puts_like" &&
                  parsed->callee->signature.is_vararg &&
                  parsed->main_function->signature.name == "main" &&
                  parsed->call->result == "%t.main.puts_like.structured" &&
                  parsed->parsed_call.symbol_name == "puts_like" &&
                  parsed->parsed_call.typed_call.args.size() == 2 &&
                  parsed->parsed_call.typed_call.args[0].operand == "17" &&
                  parsed->parsed_call.typed_call.args[1].operand == "23" &&
                  parsed->args.size() == 2 &&
                  parsed->args[0].kind == c4c::backend::ParsedBackendExternCallArg::Kind::I32Imm &&
                  parsed->args[0].imm == 17 &&
                  parsed->args[1].kind == c4c::backend::ParsedBackendExternCallArg::Kind::I32Imm &&
                  parsed->args[1].imm == 23 &&
                  !parsed->return_call_result && parsed->return_imm == 29,
              "x86_64 structured declared direct-call module parser should preserve renamed helper symbols, parameters, direct-call operands, and folded return immediates without target-local helper-body scans");
}

void test_x86_64_backend_call_helpers_parse_structured_declared_direct_call_lir_module() {
  auto module = make_x86_extern_decl_inferred_param_module();
  auto& decl = module.extern_decls.front();
  auto& main_fn = module.functions.front();
  auto& call = std::get<c4c::codegen::lir::LirCallOp>(main_fn.blocks.front().insts.front());

  decl.name = "puts_like";
  call.callee = "@puts_like";
  call.result = "%t.main.puts_like.structured";
  call.args_str = "i32 17, i32 23";
  main_fn.blocks.front().terminator = c4c::codegen::lir::LirRet{std::string("29"), "i32"};

  const auto parsed =
      c4c::backend::parse_backend_minimal_declared_direct_call_lir_module(module);
  expect_true(parsed.has_value() && parsed->callee != nullptr &&
                  parsed->main_function != nullptr && parsed->main_block != nullptr &&
                  parsed->call != nullptr && parsed->callee->name == "puts_like" &&
                  parsed->main_function->name == "main" &&
                  parsed->call->result == "%t.main.puts_like.structured" &&
                  parsed->parsed_call.symbol_name == "puts_like" &&
                  parsed->parsed_call.typed_call.args.size() == 2 &&
                  parsed->parsed_call.typed_call.args[0].operand == "17" &&
                  parsed->parsed_call.typed_call.args[1].operand == "23" &&
                  parsed->args.size() == 2 &&
                  parsed->args[0].kind == c4c::backend::ParsedBackendExternCallArg::Kind::I32Imm &&
                  parsed->args[0].imm == 17 &&
                  parsed->args[1].kind == c4c::backend::ParsedBackendExternCallArg::Kind::I32Imm &&
                  parsed->args[1].imm == 23 &&
                  !parsed->return_call_result && parsed->return_imm == 29,
              "x86_64 structured declared direct-call LIR parser should preserve renamed extern declarations, direct-call operands, and folded return immediates without lowering through backend IR");
}

void test_x86_64_shared_linker_parses_builtin_x86_extern_call_object() {
  const auto object_path = make_temp_output_path("c4c_x86_extern_call_parse");
  const auto staged = c4c::backend::x86::assemble_module(
      make_x86_extern_decl_object_module(), object_path);
  expect_true(staged.object_emitted,
              "x86 built-in assembler should emit an object for the bounded extern-call slice");

  const auto object_bytes = read_file_bytes(object_path);
  std::string error;
  const auto parsed = c4c::backend::linker_common::parse_elf64_object(
      object_bytes, object_path, c4c::backend::elf::EM_X86_64, &error);
  expect_true(parsed.has_value(),
              "x86_64 shared linker object parser should accept the built-in x86 extern-call object: " +
                  error);

  const auto& object = *parsed;
  const auto text_index = find_section_index(object, ".text");
  const auto rela_text_index = find_section_index(object, ".rela.text");
  const auto symtab_index = find_section_index(object, ".symtab");
  const auto strtab_index = find_section_index(object, ".strtab");
  const auto shstrtab_index = find_section_index(object, ".shstrtab");
  expect_true(text_index < object.sections.size() && rela_text_index < object.sections.size() &&
                  symtab_index < object.sections.size() && strtab_index < object.sections.size() &&
                  shstrtab_index < object.sections.size(),
              "x86_64 shared linker object parser should preserve the relocation-bearing x86 object section inventory");
  expect_true(object.relocations.size() == object.sections.size(),
              "x86_64 shared linker object parser should keep relocation vectors indexed by section for built-in x86 emitted objects");
  expect_true(object.section_data[text_index].size() == 6,
              "x86_64 shared linker object parser should preserve the built-in emitted x86 extern-call text payload");

  const auto main_symbol = std::find_if(
      object.symbols.begin(), object.symbols.end(),
      [&](const c4c::backend::linker_common::Elf64Symbol& symbol) {
        return symbol.name == "main";
      });
  const auto helper_symbol = std::find_if(
      object.symbols.begin(), object.symbols.end(),
      [&](const c4c::backend::linker_common::Elf64Symbol& symbol) {
        return symbol.name == "helper_ext";
      });
  expect_true(main_symbol != object.symbols.end() && main_symbol->is_global() &&
                  main_symbol->sym_type() == c4c::backend::elf::STT_FUNC &&
                  main_symbol->shndx == text_index,
              "x86_64 shared linker object parser should preserve the built-in emitted x86 main symbol inventory");
  expect_true(helper_symbol != object.symbols.end() && helper_symbol->is_undefined(),
              "x86_64 shared linker object parser should preserve the built-in emitted x86 undefined helper symbol");

  expect_true(object.relocations[text_index].size() == 1,
              "x86_64 shared linker object parser should preserve one relocation for the bounded built-in x86 extern-call object");
  const auto& relocation = object.relocations[text_index].front();
  expect_true(relocation.offset == 1 && relocation.rela_type == 4 &&
                  relocation.addend == -4 &&
                  relocation.sym_idx < object.symbols.size() &&
                  object.symbols[relocation.sym_idx].name == "helper_ext",
              "x86_64 shared linker object parser should preserve the staged x86 PLT32 relocation contract for the built-in extern-call object");

  std::filesystem::remove(object_path);
}

int main(int argc, char* argv[]) {
  if (argc >= 2) test_filter() = argv[1];
  RUN_TEST(test_x86_64_backend_call_helpers_parse_structured_declared_direct_call_module);
  RUN_TEST(test_x86_64_backend_call_helpers_parse_structured_declared_direct_call_lir_module);
  RUN_TEST(test_x86_64_shared_linker_parses_builtin_x86_extern_call_object);
  check_failures();
  return 0;
}
