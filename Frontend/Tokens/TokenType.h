#pragma once
namespace Coda {
	namespace Frontend {
		enum class TokenType {
			INVALID,
			// Literal Types
			BOOL,
			BYTE,
			INT,
			FLOAT,
			DOUBLE,
			LONG,
			IDENTIFIER,

			// Keywords
			LET,
			CONST,
			TYPE,

			// Operators
			EQUALS,
			SEMICOLON,
			COMMA,
			COLON,
			BINARY_OPERATOR,
				// ()
			OPEN_PAREN,
			CLOSE_PAREN,
				// []
			OPEN_BRACKET,
			CLOSE_BRACKET,
				// {}
			OPEN_BRACE,
			CLOSE_BRACE,

			// End of File
			END_OF_FILE,

		};

	}
}