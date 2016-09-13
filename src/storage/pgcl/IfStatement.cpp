/* 
 * File:   IfStatement.cpp
 * Author: Lukas Westhofen
 * 
 * Created on 11. April 2015, 17:42
 */

#include "IfStatement.h"
#include "src/storage/pgcl/AbstractStatementVisitor.h"

namespace storm {
    namespace pgcl {
        IfStatement::IfStatement(storm::pgcl::BooleanExpression const& condition, std::shared_ptr<storm::pgcl::PgclBlock> const& body) :
            ifBody(body), condition(condition) {
        }

        IfStatement::IfStatement(storm::pgcl::BooleanExpression const& condition, std::shared_ptr<storm::pgcl::PgclBlock> const& ifBody, std::shared_ptr<storm::pgcl::PgclBlock> const& elseBody) :
            ifBody(ifBody), elseBody(elseBody), condition(condition) {
            this->hasElseBody = true;
        }

        std::shared_ptr<storm::pgcl::PgclBlock> const& IfStatement::getIfBody() const {
            return this->ifBody;
        }

        std::shared_ptr<storm::pgcl::PgclBlock> const& IfStatement::getElseBody() const {
            if(this->elseBody) {
                return this->elseBody;
            } else {
                throw "Tried to access non-present else body of if statement.";
            }
        }
        
        bool IfStatement::hasElse() const{
            return this->hasElseBody;
        }
        
        storm::pgcl::BooleanExpression const& IfStatement::getCondition() const {
            return this->condition;
        }

        void IfStatement::accept(storm::pgcl::AbstractStatementVisitor& visitor) {
            visitor.visit(*this);
        }

    }
}