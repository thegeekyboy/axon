#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <memory>
#include <variant>
#include <optional>
#include <stdexcept>
#include <vector>

namespace axon {

	namespace logica {

		/* ═══════════════════════════════════════════════════════════════
		TRANSACTION  (your domain struct — unchanged layout)
		═══════════════════════════════════════════════════════════════ */
		struct Transaction {
			char     orderid[10]                 {};
			uint8_t  trans_status                {};
			long     trans_initate_time          {};
			long     trans_end_time              {};
			long     debit_party_id              {};
			char     debit_party_mnemonic[256]   {};
			long     credit_party_id             {};
			char     credit_party_mnemonic[256]  {};
			uint16_t reason_type                 {};
			uint16_t type_code                   {};
			double   org_amount                  {};
			double   actual_amount               {};
			double   fee                         {};
			double   commission                  {};
			uint8_t  is_reversed                 {};
		};

		/* ═══════════════════════════════════════════════════════════════
		ERRORS
		═══════════════════════════════════════════════════════════════ */
		struct ParseError : std::runtime_error {
			explicit ParseError(const std::string& msg) : std::runtime_error(msg) {}
		};

		struct EvalError : std::runtime_error {
			explicit EvalError(const std::string& msg) : std::runtime_error(msg) {}
		};

		/* ═══════════════════════════════════════════════════════════════
		TOKENS
		═══════════════════════════════════════════════════════════════ */
		enum class TokenType {
			Eof,
			Ident,          // field name
			Number,         // 3.14, 42
			String,         // 'hello'
			And, Or, Not,
			Eq, Neq, Lt, Lte, Gt, Gte,
			LParen, RParen,
		};

		struct Token {
			TokenType   type  { TokenType::Eof };
			std::string text  {};               // raw lexeme / string value / ident
			double      value {};               // populated when type == Number
		};

		/* ═══════════════════════════════════════════════════════════════
		LEXER
		═══════════════════════════════════════════════════════════════ */
		class Lexer {
		public:
			explicit Lexer(std::string_view src);

			Token next();                       // consume and return next token
			Token peek();                       // look-ahead without consuming

		private:
			std::string_view src_;
			std::size_t      pos_ { 0 };

			void        skip_whitespace();
			char        current() const;
			char        peek_char(int offset = 1) const;
		};

		/* ═══════════════════════════════════════════════════════════════
		AST
		═══════════════════════════════════════════════════════════════ */
		enum class CmpOp { Eq, Neq, Lt, Lte, Gt, Gte };

		// Right-hand side of a comparison: either a number or a string
		using RhsValue = std::variant<double, std::string>;

		struct AstNode {
			virtual ~AstNode() = default;
		};
		using AstNodePtr = std::unique_ptr<AstNode>;

		struct CmpNode : AstNode {
			std::string field;
			CmpOp       op;
			RhsValue    rhs;
		};

		struct AndNode : AstNode {
			AstNodePtr left, right;
		};

		struct OrNode : AstNode {
			AstNodePtr left, right;
		};

		struct NotNode : AstNode {
			AstNodePtr operand;
		};

		/* ═══════════════════════════════════════════════════════════════
		PARSER
		═══════════════════════════════════════════════════════════════ */
		class Parser {
		public:
			explicit Parser(std::string_view expr);

			// Returns the root AST node; throws ParseError on failure.
			AstNodePtr parse();

		private:
			Lexer lexer_;
			Token current_;

			Token       advance();
			bool        check(TokenType t) const;
			bool        match(TokenType t);
			void        expect(TokenType t, std::string_view msg);

			AstNodePtr  parse_expr();
			AstNodePtr  parse_or();
			AstNodePtr  parse_and();
			AstNodePtr  parse_not();
			AstNodePtr  parse_atom();
			AstNodePtr  parse_cmp();
		};

		/* ═══════════════════════════════════════════════════════════════
		EVALUATOR
		═══════════════════════════════════════════════════════════════ */
		class Evaluator {
		public:
			// Evaluate a pre-compiled AST against a transaction.
			// Returns true/false; throws EvalError on type mismatches or unknown fields.
			static bool eval(const AstNode& node, const Transaction& txn);
		};

		/* ═══════════════════════════════════════════════════════════════
		CONVENIENCE  free functions
		═══════════════════════════════════════════════════════════════ */

		// Compile an expression string into an AST (throws ParseError).
		AstNodePtr compile(std::string_view expr);

		// Parse + evaluate in one call (throws ParseError or EvalError).
		bool eval_expr(std::string_view expr, const Transaction& txn);

	} // logica
} // axon