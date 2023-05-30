#include <iostream>

#include "Utils/FileReader.h"
#include "Coda.h"



int repl() {

	Coda::Runtime::Environment env = Coda::Runtime::Environment::root();
	std::cout << "Welcome to Coda REPL!" << std::endl;
	while (1) {
		Coda::Error::Manager::reset();

		std::string source;
		std::cout << ">> ";
		std::getline(std::cin, source);
		if (source == ":q" 
			|| source == "exit" 
			|| source == "quit"
			|| source == "e"
			|| source == "q") break;


		if (source.empty()) continue;

		Coda::Frontend::Lexer lexer = Coda::Frontend::Lexer();
		std::vector<Coda::Frontend::Token> tokens = lexer.tokenise(source);

		IF_ERROR_CONTINUE;
		Coda::Frontend::Parser parser = Coda::Frontend::Parser();
		Coda::Frontend::Program program = parser.parse(tokens);

		IF_ERROR_CONTINUE;
		Coda::Runtime::Interpreter inter = Coda::Runtime::Interpreter();
		Coda::Runtime::Value out = inter.evaluateProgram(program, env);

	}
	return 0;
}



int main(int argc, char** argv) {
	Coda::Runtime::Environment env = Coda::Runtime::Environment::root();
	std::string filename;

	if (argc > 1) {
		filename = argv[1];
	}
	else {
		std::cout << "[Coda] File not specified. Running REPL." << std::endl;
		repl();
	}

	Coda::Utils::FileReader fr = { filename };
	std::string source = fr.readToString();

	if (source.empty()) {
		return 0;
	}

	Coda::Frontend::Lexer lexer = Coda::Frontend::Lexer();
	std::vector<Coda::Frontend::Token> tokens = lexer.tokenise(source);

	IF_ERROR_RETURN(1);
	Coda::Frontend::Parser parser = Coda::Frontend::Parser();
	Coda::Frontend::Program program = parser.parse(tokens);

	IF_ERROR_RETURN(1);
	Coda::Runtime::Interpreter inter = Coda::Runtime::Interpreter();
	inter.evaluateProgram(program, env);

	std::cout << std::endl << "Press any key to continue...";
	std::cin.get();

	return 0;
}

