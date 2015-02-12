#ifndef STORM_STORAGE_EXPRESSIONS_DOUBLELITERALEXPRESSION_H_
#define STORM_STORAGE_EXPRESSIONS_DOUBLELITERALEXPRESSION_H_

#include "src/storage/expressions/BaseExpression.h"
#include "src/utility/OsDetection.h"

namespace storm {
    namespace expressions {
        class DoubleLiteralExpression : public BaseExpression {
        public:
            /*!
             * Creates an double literal expression with the given value.
             *
             * @param manager The manager responsible for this expression.
             * @param value The value of the double literal.
             */
            DoubleLiteralExpression(ExpressionManager const& manager, double value);
            
            // Instantiate constructors and assignments with their default implementations.
            DoubleLiteralExpression(DoubleLiteralExpression const& other) = default;
            DoubleLiteralExpression& operator=(DoubleLiteralExpression const& other) = delete;
#ifndef WINDOWS
            DoubleLiteralExpression(DoubleLiteralExpression&&) = default;
            DoubleLiteralExpression& operator=(DoubleLiteralExpression&&) = delete;
#endif
            virtual ~DoubleLiteralExpression() = default;
            
            // Override base class methods.
            virtual double evaluateAsDouble(Valuation const* valuation = nullptr) const override;
            virtual bool isLiteral() const override;
            virtual void gatherVariables(std::set<storm::expressions::Variable>& variables) const override;
            virtual std::shared_ptr<BaseExpression const> simplify() const override;
            virtual boost::any accept(ExpressionVisitor& visitor) const override;

            /*!
             * Retrieves the value of the double literal.
             *
             * @return The value of the double literal.
             */
            double getValue() const;
            
        protected:
            // Override base class method.
            virtual void printToStream(std::ostream& stream) const override;
            
        private:
            // The value of the double literal.
            double value;
        };
    }
}

#endif /* STORM_STORAGE_EXPRESSIONS_DOUBLELITERALEXPRESSION_H_ */