/*
 * gkpro @ 2021-03-19
 *   Nexus Library
 *   Expression eval header (reference material)
 *
 *   Basic implementation for converting an expression to
 *   Reverse Polish Notation and evaluating it
 *
 *   Expression evaluation only handles integers as operands
 *   There are no error handling nor error checking
 */

#include <algorithm>
#include <array>
#include <cmath>
#include <stack>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

#include "utility.h"

namespace nxs::expr {
namespace detail {

    constexpr auto arrOpt   = std::array{ '+', '-', '*', '/', '^' };
    constexpr auto arrParen = std::array{ '(', ')' };
    constexpr auto arrWs    = std::array{ ' ', '\n', '\t' };

    struct InvalidOperator : public std::runtime_error {
        InvalidOperator() : std::runtime_error("Invalid operator") {}
    };

    enum class OperatorType {
        NA,
        ADD,
        SUB,
        MUL,
        DIV,
        POW
    };

    enum class Paren { OPEN, CLOSE };

    struct Operator {
        OperatorType type;
        char precedence;
    };

    struct Operand {    // TODO: string for variables
        using value_type = int;
        int value;
    };

    using Token = std::variant<Operator, Operand>;
    using OpNParen = std::variant<Operator, Paren>;

    struct RPN {
        std::vector<Token> r;
    };

    [[nodiscard]] constexpr bool isOperator(char ch) {
        return std::any_of(std::begin(arrOpt), std::end(arrOpt), [ch](auto x) { return x == ch; });
    }

    [[nodiscard]] constexpr bool isParen(char ch) {
        return std::any_of(std::begin(arrParen), std::end(arrParen), [ch](auto x) { return x == ch; });
    }

    [[nodiscard]] constexpr bool isWhitespace(char ch) {
        return std::any_of(std::begin(arrWs), std::end(arrWs), [ch](auto x) { return x == ch; });
    }

    [[nodiscard]] constexpr bool isOpenParen(char ch) {
        return ch == '(';
    }

    [[nodiscard]] constexpr bool isCloseParen(char ch) {
        return ch == ')';
    }

    [[nodiscard]] constexpr Operator createOperator(char ch) {
        if (ch == '+') { return Operator{ OperatorType::ADD, 1 }; }
        if (ch == '-') { return Operator{ OperatorType::SUB, 1 }; }
        if (ch == '*') { return Operator{ OperatorType::MUL, 2 }; }
        if (ch == '/') { return Operator{ OperatorType::DIV, 2 }; }
        if (ch == '^') { return Operator{ OperatorType::POW, 3 }; }
        return Operator{ OperatorType::NA, 0 };
    }

    [[nodiscard]] inline Operand createOperand(const std::string& str) {
        return Operand{ std::stoi(str) };
    }

    /**
     * @brief   Functions defined for the operators
     */
    [[nodiscard]] inline Operand execOperator(Operator op, Operand lhs, Operand rhs) {
        if (op.type == OperatorType::ADD) {
            return Operand{ lhs.value + rhs.value };
        }
        if (op.type == OperatorType::SUB) {
            return Operand{ lhs.value - rhs.value };
        }
        if (op.type == OperatorType::MUL) {
            return Operand{ lhs.value * rhs.value };
        }
        if (op.type == OperatorType::DIV) {
            return Operand{ lhs.value / rhs.value };
        }
        if (op.type == OperatorType::POW) {
            return Operand{ static_cast<Operand::value_type>(std::pow(lhs.value, rhs.value)) };
        }
        throw InvalidOperator();
    }

    /**
     * @brief   Moves the top of stack to the token list
     */
    inline void transferTop(std::stack<OpNParen>& stack, RPN& tokens) {
        tokens.r.push_back(std::get<Operator>(stack.top()));
        stack.pop();
    }

    /**
     * @brief   Append the operand (if it is not empty) to the token list
     */
    inline void append(std::string& operand, RPN& tokens) {
        if (!operand.empty()) {
            tokens.r.push_back(createOperand(operand));
            operand = "";
        }
    }

    /**
     * @brief   Convert input into tokens in Reverse Polish Notation
     *
     * - pass through operands as is
     * - push operators onto a stack and append to the token list after the last iteration
     *   - if the current operator's precedence is lower than the ones on the stack,
     *     append them to the token list
     * - in case of closing paren move the operators from the stack until the open paren
     *   to the token list
     * - whitespaces are discarded
     */
    RPN inline rpn(const std::string& expr) {
        RPN tokens;
        std::stack<OpNParen> stack;
        std::string operand;

        for (const auto& ch : expr) {
            if (isWhitespace(ch)) { /* NOOP */ }
            else if (isOperator(ch)) {
                append(operand, tokens);

                const auto op = createOperator(ch);
                while (!stack.empty() && std::holds_alternative<Operator>(stack.top())
                    && op.precedence < std::get<Operator>(stack.top()).precedence)
                {
                    transferTop(stack, tokens);
                }
                stack.push(op);
            }
            else if (isOpenParen(ch)) {
                stack.push(Paren::OPEN);
            }
            else if (isCloseParen(ch)) {
                append(operand, tokens);
                while (std::holds_alternative<Operator>(stack.top())) {
                    transferTop(stack, tokens);
                }
                stack.pop();    // (open) paren is just a marker, discard it
            }
            else {  // TODO: isOperand()
                operand.push_back(ch);
            }
        }

        append(operand, tokens);

        while (!stack.empty()) {
            transferTop(stack, tokens);
        }

        return tokens;
    }

    /**
     * @brief   Evaluate RPN Tokens
     *
     * Push the operands onto a stack, in case of an operator, pop
     * two operands from the stack, evaluate them via the operator
     * and push the result back onto the stack.
     */
    inline auto eval(const RPN& tokens) {
        std::stack<Token> stack;

        for (const auto& token : tokens.r) {
            std::visit(Lambdas {
                [&stack](const Operand& operand) {
                    stack.push(operand);
                },
                [&stack](const Operator& op) {
                    const auto rhs = std::get<Operand>(stack.top());
                    stack.pop();
                    const auto lhs = std::get<Operand>(stack.top());
                    stack.pop();
                    stack.push(execOperator(op, lhs, rhs));
                }
            }, token);
        }

        return std::get<Operand>(stack.top()).value;
    }

    std::string inline toString(OperatorType x) {
        switch(x) {
            case OperatorType::ADD:     return "+";
            case OperatorType::SUB:     return "-";
            case OperatorType::MUL:     return "*";
            case OperatorType::DIV:     return "/";
            case OperatorType::POW:     return "^";
            default:                    return "<NA>";
        }
    }

    std::string inline toString(const Token& token) {
        return std::visit(Lambdas{
            [](Operand op) { return nxs::toString(op.value); },
            [](Operator op) { return toString(op.type); }
        }, token);
    }

} // namespace detail

std::string inline toString(const detail::RPN& tokens) {
    std::string str;
    for (const auto& token : tokens.r) {
        str = joinStr(" ", str, token);
    }
    return str;
}

[[nodiscard]] inline auto evalExpr(const std::string& expr) {
    using namespace detail;
    return eval(rpn(expr));
}

} // namespace nxs
