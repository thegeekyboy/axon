#include "logica.h"

#include <cctype>
#include <cmath>
#include <cstring>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <functional>

namespace axon {

	namespace logica {

		/* ═══════════════════════════════════════════════════════════════
		LEXER
		═══════════════════════════════════════════════════════════════ */

		Lexer::Lexer(std::string_view src) : src_(src) {}

		char Lexer::current() const {
			return pos_ < src_.size() ? src_[pos_] : '\0';
		}

		char Lexer::peek_char(int offset) const {
			auto idx = pos_ + offset;
			return idx < src_.size() ? src_[idx] : '\0';
		}

		void Lexer::skip_whitespace() {
			while (std::isspace(static_cast<unsigned char>(current())))
				++pos_;
		}

		Token Lexer::next() {
			skip_whitespace();

			const char c = current();

			if (c == '\0') return { TokenType::Eof };

			// Two-char operators
			if (c == '=' && peek_char() == '=') { pos_ += 2; return { TokenType::Eq,     "==" }; }
			if (c == '!' && peek_char() == '=') { pos_ += 2; return { TokenType::Neq,    "!=" }; }
			if (c == '<' && peek_char() == '=') { pos_ += 2; return { TokenType::Lte,    "<=" }; }
			if (c == '>' && peek_char() == '=') { pos_ += 2; return { TokenType::Gte,    ">=" }; }

			// Single-char operators / punctuation
			if (c == '<') { ++pos_; return { TokenType::Lt,     "<"  }; }
			if (c == '>') { ++pos_; return { TokenType::Gt,     ">"  }; }
			if (c == '(') { ++pos_; return { TokenType::LParen, "("  }; }
			if (c == ')') { ++pos_; return { TokenType::RParen, ")"  }; }

			// String literal: 'value' or "value"
			if (c == '\'' || c == '"') {
				const char quote = c;
				++pos_;
				std::string value;
				while (current() != '\0' && current() != quote) {
					value += current();
					++pos_;
				}
				if (current() == quote) ++pos_; // consume closing quote
				return { TokenType::String, std::move(value) };
			}

			// Number literal (integer or decimal, optional leading minus)
			if (std::isdigit(static_cast<unsigned char>(c)) ||
				(c == '-' && std::isdigit(static_cast<unsigned char>(peek_char())))) {
				std::string raw;
				if (c == '-') { raw += c; ++pos_; }
				while (std::isdigit(static_cast<unsigned char>(current())) || current() == '.') {
					raw += current();
					++pos_;
				}
				double val = std::stod(raw);
				return { TokenType::Number, raw, val };
			}

			// Identifier or keyword
			if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
				std::string ident;
				while (std::isalnum(static_cast<unsigned char>(current())) || current() == '_') {
					ident += current();
					++pos_;
				}
				if (ident == "and") return { TokenType::And, ident };
				if (ident == "or")  return { TokenType::Or,  ident };
				if (ident == "not") return { TokenType::Not, ident };
				return { TokenType::Ident, ident };
			}

			// Unknown character: skip
			++pos_;
			return next();
		}

		Token Lexer::peek() {
			const auto saved = pos_;
			Token t = next();
			pos_ = saved;
			return t;
		}

		/* ═══════════════════════════════════════════════════════════════
		PARSER
		═══════════════════════════════════════════════════════════════ */

		Parser::Parser(std::string_view expr)
			: lexer_(expr), current_(lexer_.next()) {}

		Token Parser::advance() {
			Token prev = std::move(current_);
			current_ = lexer_.next();
			return prev;
		}

		bool Parser::check(TokenType t) const {
			return current_.type == t;
		}

		bool Parser::match(TokenType t) {
			if (check(t)) { advance(); return true; }
			return false;
		}

		void Parser::expect(TokenType t, std::string_view msg) {
			if (!check(t))
				throw ParseError(std::string(msg) + " (got '" + current_.text + "')");
			advance();
		}

		static CmpOp token_to_cmpop(TokenType t) {
			switch (t) {
				case TokenType::Eq:  return CmpOp::Eq;
				case TokenType::Neq: return CmpOp::Neq;
				case TokenType::Lt:  return CmpOp::Lt;
				case TokenType::Lte: return CmpOp::Lte;
				case TokenType::Gt:  return CmpOp::Gt;
				case TokenType::Gte: return CmpOp::Gte;
				default: throw ParseError("expected comparison operator");
			}
		}

		// cmp_expr → IDENT  CMP_OP  (NUMBER | STRING)
		AstNodePtr Parser::parse_cmp() {
			if (!check(TokenType::Ident))
				throw ParseError("expected field name, got '" + current_.text + "'");

			Token field_tok = advance();

			CmpOp op = token_to_cmpop(current_.type);
			advance(); // consume operator

			auto node      = std::make_unique<CmpNode>();
			node->field    = field_tok.text;
			node->op       = op;

			if (check(TokenType::Number)) {
				node->rhs = current_.value;
				advance();
			} else if (check(TokenType::String)) {
				node->rhs = current_.text;
				advance();
			} else {
				throw ParseError("expected number or string on RHS, got '" + current_.text + "'");
			}
			return node;
		}

		// atom → '(' expr ')' | cmp_expr
		AstNodePtr Parser::parse_atom() {
			if (match(TokenType::LParen)) {
				auto inner = parse_expr();
				expect(TokenType::RParen, "expected closing ')'");
				return inner;
			}
			return parse_cmp();
		}

		// not_expr → 'not' not_expr | atom
		AstNodePtr Parser::parse_not() {
			if (match(TokenType::Not)) {
				auto operand  = parse_not();
				auto node     = std::make_unique<NotNode>();
				node->operand = std::move(operand);
				return node;
			}
			return parse_atom();
		}

		// and_expr → not_expr ( 'and' not_expr )*
		AstNodePtr Parser::parse_and() {
			auto left = parse_not();
			while (check(TokenType::And)) {
				advance();
				auto right      = parse_not();
				auto node       = std::make_unique<AndNode>();
				node->left      = std::move(left);
				node->right     = std::move(right);
				left            = std::move(node);
			}
			return left;
		}

		// or_expr → and_expr ( 'or' and_expr )*
		AstNodePtr Parser::parse_or() {
			auto left = parse_and();
			while (check(TokenType::Or)) {
				advance();
				auto right      = parse_and();
				auto node       = std::make_unique<OrNode>();
				node->left      = std::move(left);
				node->right     = std::move(right);
				left            = std::move(node);
			}
			return left;
		}

		AstNodePtr Parser::parse_expr() { return parse_or(); }

		AstNodePtr Parser::parse() {
			auto root = parse_expr();
			if (!check(TokenType::Eof))
				throw ParseError("unexpected token '" + current_.text + "' after expression");
			return root;
		}

		/* ═══════════════════════════════════════════════════════════════
		FIELD REGISTRY
		═══════════════════════════════════════════════════════════════ */

		enum class FieldKind { Uint8, Uint16, Long, Double, String };

		struct FieldDef {
			FieldKind   kind;
			std::size_t offset;
		};

		// Returns a resolved numeric double or string from the transaction.
		using FieldValue = std::variant<double, std::string>;

		static const std::unordered_map<std::string, FieldDef>& field_registry() {
			static const std::unordered_map<std::string, FieldDef> reg = {
				{ "orderid",               { FieldKind::String, offsetof(Transaction, orderid)               } },
				{ "trans_status",          { FieldKind::Uint8,  offsetof(Transaction, trans_status)          } },
				{ "trans_initate_time",    { FieldKind::Long,   offsetof(Transaction, trans_initate_time)    } },
				{ "trans_end_time",        { FieldKind::Long,   offsetof(Transaction, trans_end_time)        } },
				{ "debit_party_id",        { FieldKind::Long,   offsetof(Transaction, debit_party_id)        } },
				{ "debit_party_mnemonic",  { FieldKind::String, offsetof(Transaction, debit_party_mnemonic)  } },
				{ "credit_party_id",       { FieldKind::Long,   offsetof(Transaction, credit_party_id)       } },
				{ "credit_party_mnemonic", { FieldKind::String, offsetof(Transaction, credit_party_mnemonic) } },
				{ "reason_type",           { FieldKind::Uint16, offsetof(Transaction, reason_type)           } },
				{ "type_code",             { FieldKind::Uint16, offsetof(Transaction, type_code)             } },
				{ "org_amount",            { FieldKind::Double, offsetof(Transaction, org_amount)            } },
				{ "actual_amount",         { FieldKind::Double, offsetof(Transaction, actual_amount)         } },
				{ "fee",                   { FieldKind::Double, offsetof(Transaction, fee)                   } },
				{ "commission",            { FieldKind::Double, offsetof(Transaction, commission)            } },
				{ "is_reversed",           { FieldKind::Uint8,  offsetof(Transaction, is_reversed)           } },
			};
			return reg;
		}

		static FieldValue resolve_field(const std::string& name, const Transaction& txn) {
			const auto& reg = field_registry();
			auto it = reg.find(name);
			if (it == reg.end())
				throw EvalError("Unknown field: '" + name + "'");

			const auto& def = it->second;
			const auto* base = reinterpret_cast<const std::byte*>(&txn) + def.offset;

			switch (def.kind) {
				case FieldKind::Uint8:  return static_cast<double>(*reinterpret_cast<const uint8_t* >(base));
				case FieldKind::Uint16: return static_cast<double>(*reinterpret_cast<const uint16_t*>(base));
				case FieldKind::Long:   return static_cast<double>(*reinterpret_cast<const long*    >(base));
				case FieldKind::Double: return                     *reinterpret_cast<const double*   >(base);
				case FieldKind::String: return std::string(reinterpret_cast<const char*>(base));
			}
			throw EvalError("Unhandled field kind for '" + name + "'");
		}

		/* ═══════════════════════════════════════════════════════════════
		EVALUATOR
		═══════════════════════════════════════════════════════════════ */

		static bool apply_cmp(CmpOp op, const FieldValue& lhs, const RhsValue& rhs) {
			// Both must be the same variant alternative
			if (std::holds_alternative<double>(lhs) && std::holds_alternative<double>(rhs)) {
				double l = std::get<double>(lhs);
				double r = std::get<double>(rhs);
				switch (op) {
					case CmpOp::Eq:  return std::fabs(l - r) < 1e-12;
					case CmpOp::Neq: return std::fabs(l - r) >= 1e-12;
					case CmpOp::Lt:  return l <  r;
					case CmpOp::Lte: return l <= r;
					case CmpOp::Gt:  return l >  r;
					case CmpOp::Gte: return l >= r;
				}
			}

			if (std::holds_alternative<std::string>(lhs) && std::holds_alternative<std::string>(rhs)) {
				const auto& l = std::get<std::string>(lhs);
				const auto& r = std::get<std::string>(rhs);
				int cmp = l.compare(r);
				switch (op) {
					case CmpOp::Eq:  return cmp == 0;
					case CmpOp::Neq: return cmp != 0;
					case CmpOp::Lt:  return cmp <  0;
					case CmpOp::Lte: return cmp <= 0;
					case CmpOp::Gt:  return cmp >  0;
					case CmpOp::Gte: return cmp >= 0;
				}
			}

			// Type mismatch
			if (std::holds_alternative<std::string>(lhs))
				throw EvalError("Field is a string but RHS is a number");
			throw EvalError("Field is numeric but RHS is a string");
		}

		bool Evaluator::eval(const AstNode& node, const Transaction& txn) {
			if (const auto* cmp = dynamic_cast<const CmpNode*>(&node)) {
				FieldValue lhs = resolve_field(cmp->field, txn);
				return apply_cmp(cmp->op, lhs, cmp->rhs);
			}

			if (const auto* and_node = dynamic_cast<const AndNode*>(&node)) {
				// Short-circuit
				return eval(*and_node->left, txn) && eval(*and_node->right, txn);
			}

			if (const auto* or_node = dynamic_cast<const OrNode*>(&node)) {
				// Short-circuit
				return eval(*or_node->left, txn) || eval(*or_node->right, txn);
			}

			if (const auto* not_node = dynamic_cast<const NotNode*>(&node)) {
				return !eval(*not_node->operand, txn);
			}

			throw EvalError("Unknown AST node type");
		}

		/* ═══════════════════════════════════════════════════════════════
		CONVENIENCE
		═══════════════════════════════════════════════════════════════ */

		AstNodePtr compile(std::string_view expr) {
			Parser p(expr);
			return p.parse();
		}

		bool eval_expr(std::string_view expr, const Transaction& txn) {
			auto ast = compile(expr);
			return Evaluator::eval(*ast, txn);
		}

	} // logica
} // axon

