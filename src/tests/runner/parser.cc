#include "core/expr.hh"
#include "parser/parser.hh"
#include "analyzers/NameResolver.hh"
#include "analyzers/TypeResolver.hh"
#include "tests/runner/base.hh"

#include "parser.hh"

#include <iostream>
#include <sstream>
namespace coral {
  namespace tests {
    void ParserTests::parse_and_print(const char * name, const char * path) {
      std::cout
        << "-------------------- ["
        << name
        << "] --------------------"
        << "\n";
      auto parser = coralParseModule(path);
      auto module = _coralModule(parser);
      total++;
      if (module) success++;
      coralPrintAST(parser);
      coralDestroyModule(parser);
    }

    void ParserTests::checkTypeInference() {
      auto parser = coralParseModule("tests/cases/features/typeInference.coral");
      auto module = (ast::Module *)_coralModule(parser);
      analyzers::NameResolver nresolve(module);
      analyzers::TypeResolver treslve(module);
      BeginTest("Type Inference");
      for(auto && expr : module->body->lines) {
        auto func = dynamic_cast<ast::Func *>(expr.get());
        if (!func) continue;
        if (func->name == "returnsInt") {
          Assert(func->type->params.back() == coral::type::Type("Int32"));
        } else if (func->name == "returnsIntVar") {
          Assert(func->type->params.back() == coral::type::Type("Int32"));
        }
        else if (func->name == "returnsIntParam") {
          Assert(func->type->params.back() == coral::type::Type("Int32"));
        }
        else if (func->name == "returnsIntCall") {
          Assert(func->type->params.back() == coral::type::Type("Int32"));
        }
        else if (func->name == "returnsIntAddition") {
          Assert(func->type->params.back() == coral::type::Type("Int32"));
        }
        else if (func->name == "returnsString") {
          Assert(func->type->params.back() == coral::type::Type("Ptr"));
        }
      }
      EndTest();
    }

	ParserTests * run_parser_tests() {
	  auto T = new ParserTests();
	  T->parse_and_print("If Statement", "tests/cases/features/ifstatements.coral");
	  T->parse_and_print("Hello World", "tests/cases/simple/hello_world.coral");
	  T->parse_and_print("Factorial", "tests/cases/simple/factorial.coral");
	  T->parse_and_print("Collatz Function", "tests/cases/simple/collatz.coral");
	  T->parse_and_print("Fasta", "tests/cases/shootout/fasta.coral");
	  T->parse_and_print("Knucleotide", "tests/cases/shootout/knucleotide.coral");
	  T->parse_and_print("Pidigits", "tests/cases/shootout/pidigits.coral");
	  T->parse_and_print("Regex Redux", "tests/cases/shootout/regexredux.coral");
      T->checkTypeInference();
	  return T;
	}

  }
}