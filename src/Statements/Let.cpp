#include "Statements/Let.hpp"

#include "Statements/Statements.hpp"
// #include "Statements/Utils.hpp"
// #include "SymbolTable.hpp"

namespace ag {
AG_scope<Statements>
LetStatement::ParseLet(Parser& parser) {
  // if (!parser.expect_token(TokenName::AG_IDENTIFIER)) {
  //   print_error("Expected Identifier!");
  //   return nullptr;
  // }
  // parser.consume_token();
  // auto id_position = parser.get_current_position();
  //
  // if (!parser.expect_token(TokenName::AG_COLON)) {
  //   print_error("Missing: :");
  //   return nullptr;
  // }
  // parser.consume_token();
  // // let str : string = "Hello World!";
  // switch (parser.peek_token_name()) { // TODO Use Template Rather than
  // hardcode case TokenName::AG_KEYWORD: {
  //   return LetString(parser, id_position);
  // }
  // case TokenName::AG_IDENTIFIER: {
  //   return nullptr;
  // }
  // default: {
  //   print_error("Expected data types!");
  //   return nullptr;
  // }
  // }
  //
  return nullptr;
}

void
LetStatement::generate(CodeGenContext& context) {}

AG_scope<Statements>
LetStatement::LetString(Parser& parser,
                        size_t  position) {
  // parser.consume_token();
  //
  // auto  datatype = string_view_2_string(parser.get_current_token().value);
  // auto& token    = parser.get_current_token();
  //
  // auto variable_name =
  // string_view_2_string(parser.get_token(position).value); if
  // (get_table().contains(variable_name)) {
  //   print_error("Redecralration!");
  //   return nullptr;
  // }
  //
  // if (!parser.expect_token(TokenName::AG_ASSIGN)) {
  //   print_error("Missing: =");
  //   return nullptr;
  // }
  // parser.consume_token();
  //
  // auto token_name = parser.peek_token_name();
  // if (!is_compatible(token, token_name)) {
  //   // let str : string = "Hello Wrodl!"; true
  //   // let str : string = 10; false
  // }
  // if (!parser.expect_token(TokenName::AG_STRING_LITERAL)) {
  //   print_error("Missing: string literal");
  //   return nullptr;
  // }
  // parser.consume_token();
  // auto value = string_view_2_string(parser.get_current_token().value);
  //
  // if (!parser.expect_token(TokenName::AG_SEMICOLON)) {
  //   print_error("Missing: ;");
  //   return nullptr;
  // }
  //
  // Declaration_Data data;
  // data.data_type = DATA_TYPES::STRING;
  // data.decl_type = DECLARATION_TYPES::LET;
  // data.value     = value;
  // get_table().insert_value(variable_name, data);
  //
  // return AG_cscope<LetStatement>(variable_name);
  return nullptr;
}

} // namespace ag
