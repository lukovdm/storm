#include <map>
#include <string>

#include "gtest/gtest.h"
#include "src/storage/expressions/Expression.h"
#include "src/storage/expressions/ExpressionManager.h"
#include "src/storage/expressions/LinearityCheckVisitor.h"
#include "src/storage/expressions/SimpleValuation.h"
#include "src/exceptions/InvalidTypeException.h"

TEST(Expression, FactoryMethodTest) {
    std::shared_ptr<storm::expressions::ExpressionManager> manager(new storm::expressions::ExpressionManager());
    EXPECT_NO_THROW(manager->boolean(true));
    EXPECT_NO_THROW(manager->boolean(false));
    EXPECT_NO_THROW(manager->integer(3));
    EXPECT_NO_THROW(manager->rational(3.14));
    EXPECT_NO_THROW(manager->declareBooleanVariable("x"));
    EXPECT_NO_THROW(manager->declareIntegerVariable("y"));
    EXPECT_NO_THROW(manager->declareRationalVariable("z"));
}

TEST(Expression, AccessorTest) {
    std::shared_ptr<storm::expressions::ExpressionManager> manager(new storm::expressions::ExpressionManager());
    
    storm::expressions::Expression trueExpression;
    storm::expressions::Expression falseExpression;
    storm::expressions::Expression threeExpression;
    storm::expressions::Expression piExpression;
    storm::expressions::Expression boolVarExpression;
    storm::expressions::Expression intVarExpression;
    storm::expressions::Expression rationalVarExpression;
    
    ASSERT_NO_THROW(trueExpression = manager->boolean(true));
    ASSERT_NO_THROW(falseExpression = manager->boolean(false));
    ASSERT_NO_THROW(threeExpression = manager->integer(3));
    ASSERT_NO_THROW(piExpression = manager->rational(3.14));
    ASSERT_NO_THROW(boolVarExpression = manager->declareBooleanVariable("x"));
    ASSERT_NO_THROW(intVarExpression = manager->declareIntegerVariable("y"));
    ASSERT_NO_THROW(rationalVarExpression = manager->declareRationalVariable("z"));
    
    EXPECT_TRUE(trueExpression.hasBooleanType());
    EXPECT_TRUE(trueExpression.isLiteral());
    EXPECT_TRUE(trueExpression.isTrue());
    EXPECT_FALSE(trueExpression.isFalse());
    EXPECT_TRUE(trueExpression.getVariables() == std::set<storm::expressions::Variable>());
    
    EXPECT_TRUE(falseExpression.hasBooleanType());
    EXPECT_TRUE(falseExpression.isLiteral());
    EXPECT_FALSE(falseExpression.isTrue());
    EXPECT_TRUE(falseExpression.isFalse());
    EXPECT_TRUE(falseExpression.getVariables() == std::set<storm::expressions::Variable>());

    EXPECT_TRUE(threeExpression.hasIntegralType());
    EXPECT_TRUE(threeExpression.isLiteral());
    EXPECT_FALSE(threeExpression.isTrue());
    EXPECT_FALSE(threeExpression.isFalse());
    EXPECT_TRUE(threeExpression.getVariables() == std::set<storm::expressions::Variable>());
    
    EXPECT_TRUE(piExpression.hasRationalType());
    EXPECT_TRUE(piExpression.isLiteral());
    EXPECT_FALSE(piExpression.isTrue());
    EXPECT_FALSE(piExpression.isFalse());
    EXPECT_TRUE(piExpression.getVariables() == std::set<storm::expressions::Variable>());
    
    EXPECT_TRUE(boolVarExpression.hasBooleanType());
    EXPECT_FALSE(boolVarExpression.isLiteral());
    EXPECT_FALSE(boolVarExpression.isTrue());
    EXPECT_FALSE(boolVarExpression.isFalse());
    EXPECT_TRUE(boolVarExpression.getVariables() == std::set<storm::expressions::Variable>({manager->getVariable("x")}));

    EXPECT_TRUE(intVarExpression.hasIntegralType());
    EXPECT_FALSE(intVarExpression.isLiteral());
    EXPECT_FALSE(intVarExpression.isTrue());
    EXPECT_FALSE(intVarExpression.isFalse());
    EXPECT_TRUE(intVarExpression.getVariables() == std::set<storm::expressions::Variable>({manager->getVariable("y")}));

    EXPECT_TRUE(rationalVarExpression.hasRationalType());
    EXPECT_FALSE(rationalVarExpression.isLiteral());
    EXPECT_FALSE(rationalVarExpression.isTrue());
    EXPECT_FALSE(rationalVarExpression.isFalse());
    EXPECT_TRUE(rationalVarExpression.getVariables() == std::set<storm::expressions::Variable>({manager->getVariable("z")}));
}

TEST(Expression, OperatorTest) {
    std::shared_ptr<storm::expressions::ExpressionManager> manager(new storm::expressions::ExpressionManager());
    
    storm::expressions::Expression trueExpression;
    storm::expressions::Expression falseExpression;
    storm::expressions::Expression threeExpression;
    storm::expressions::Expression piExpression;
    storm::expressions::Expression boolVarExpression;
    storm::expressions::Expression intVarExpression;
    storm::expressions::Expression rationalVarExpression;
    
    ASSERT_NO_THROW(trueExpression = manager->boolean(true));
    ASSERT_NO_THROW(falseExpression = manager->boolean(false));
    ASSERT_NO_THROW(threeExpression = manager->integer(3));
    ASSERT_NO_THROW(piExpression = manager->rational(3.14));
    ASSERT_NO_THROW(boolVarExpression = manager->declareBooleanVariable("x"));
    ASSERT_NO_THROW(intVarExpression = manager->declareIntegerVariable("y"));
    ASSERT_NO_THROW(rationalVarExpression = manager->declareRationalVariable("z"));
    
    storm::expressions::Expression tempExpression;

    ASSERT_THROW(tempExpression = storm::expressions::ite(trueExpression, falseExpression, piExpression), storm::exceptions::InvalidTypeException);
    ASSERT_NO_THROW(tempExpression = storm::expressions::ite(boolVarExpression, threeExpression, rationalVarExpression));
    EXPECT_TRUE(tempExpression.hasRationalType());
    ASSERT_NO_THROW(tempExpression = storm::expressions::ite(boolVarExpression, threeExpression, intVarExpression));
    EXPECT_TRUE(tempExpression.hasIntegralType());
    ASSERT_NO_THROW(tempExpression = storm::expressions::ite(boolVarExpression, trueExpression, falseExpression));
    EXPECT_TRUE(tempExpression.hasBooleanType());
    
    ASSERT_THROW(tempExpression = trueExpression + piExpression, storm::exceptions::InvalidTypeException);
    ASSERT_NO_THROW(tempExpression = threeExpression + threeExpression);
    EXPECT_TRUE(tempExpression.hasIntegralType());
    ASSERT_NO_THROW(tempExpression = threeExpression + piExpression);
    EXPECT_TRUE(tempExpression.hasRationalType());
    ASSERT_NO_THROW(tempExpression = rationalVarExpression + rationalVarExpression);
    EXPECT_TRUE(tempExpression.hasRationalType());
    
    ASSERT_THROW(tempExpression = trueExpression - piExpression, storm::exceptions::InvalidTypeException);
    ASSERT_NO_THROW(tempExpression = threeExpression - threeExpression);
    EXPECT_TRUE(tempExpression.hasIntegralType());
    ASSERT_NO_THROW(tempExpression = threeExpression - piExpression);
    EXPECT_TRUE(tempExpression.hasRationalType());
    ASSERT_NO_THROW(tempExpression = rationalVarExpression - rationalVarExpression);
    EXPECT_TRUE(tempExpression.hasRationalType());
    
    ASSERT_THROW(tempExpression = -trueExpression, storm::exceptions::InvalidTypeException);
    ASSERT_NO_THROW(tempExpression = -threeExpression);
    EXPECT_TRUE(tempExpression.hasIntegralType());
    ASSERT_NO_THROW(tempExpression = -piExpression);
    EXPECT_TRUE(tempExpression.hasRationalType());
    ASSERT_NO_THROW(tempExpression = -rationalVarExpression);
    EXPECT_TRUE(tempExpression.hasRationalType());

    ASSERT_THROW(tempExpression = trueExpression * piExpression, storm::exceptions::InvalidTypeException);
    ASSERT_NO_THROW(tempExpression = threeExpression * threeExpression);
    EXPECT_TRUE(tempExpression.hasIntegralType());
    ASSERT_NO_THROW(tempExpression = threeExpression * piExpression);
    EXPECT_TRUE(tempExpression.hasRationalType());
    ASSERT_NO_THROW(tempExpression = intVarExpression * intVarExpression);
    EXPECT_TRUE(tempExpression.hasIntegralType());

    ASSERT_THROW(tempExpression = trueExpression / piExpression, storm::exceptions::InvalidTypeException);
    ASSERT_NO_THROW(tempExpression = threeExpression / threeExpression);
    EXPECT_TRUE(tempExpression.hasIntegralType());
    ASSERT_NO_THROW(tempExpression = threeExpression / piExpression);
    EXPECT_TRUE(tempExpression.hasRationalType());
    ASSERT_NO_THROW(tempExpression = rationalVarExpression / intVarExpression);
    EXPECT_TRUE(tempExpression.hasRationalType());

    ASSERT_THROW(tempExpression = trueExpression && piExpression, storm::exceptions::InvalidTypeException);
    ASSERT_NO_THROW(tempExpression = trueExpression && falseExpression);
    EXPECT_TRUE(tempExpression.hasBooleanType());
    ASSERT_NO_THROW(tempExpression = boolVarExpression && boolVarExpression);
    EXPECT_TRUE(tempExpression.hasBooleanType());

    ASSERT_THROW(tempExpression = trueExpression || piExpression, storm::exceptions::InvalidTypeException);
    ASSERT_NO_THROW(tempExpression = trueExpression || falseExpression);
    EXPECT_TRUE(tempExpression.hasBooleanType());
    ASSERT_NO_THROW(tempExpression = boolVarExpression || boolVarExpression);
    EXPECT_TRUE(tempExpression.hasBooleanType());

    ASSERT_THROW(tempExpression = !threeExpression, storm::exceptions::InvalidTypeException);
    ASSERT_NO_THROW(tempExpression = !trueExpression);
    EXPECT_TRUE(tempExpression.hasBooleanType());
    ASSERT_NO_THROW(tempExpression = !boolVarExpression);
    EXPECT_TRUE(tempExpression.hasBooleanType());

    ASSERT_THROW(tempExpression = trueExpression == piExpression, storm::exceptions::InvalidTypeException);
    ASSERT_NO_THROW(tempExpression = threeExpression == threeExpression);
    EXPECT_TRUE(tempExpression.hasBooleanType());
    ASSERT_NO_THROW(tempExpression = intVarExpression == rationalVarExpression);
    EXPECT_TRUE(tempExpression.hasBooleanType());

    ASSERT_THROW(tempExpression = trueExpression != piExpression, storm::exceptions::InvalidTypeException);
    ASSERT_NO_THROW(tempExpression = threeExpression != threeExpression);
    EXPECT_TRUE(tempExpression.hasBooleanType());
    ASSERT_NO_THROW(tempExpression = intVarExpression != rationalVarExpression);
    EXPECT_TRUE(tempExpression.hasBooleanType());

    ASSERT_THROW(tempExpression = trueExpression > piExpression, storm::exceptions::InvalidTypeException);
    ASSERT_NO_THROW(tempExpression = threeExpression > threeExpression);
    EXPECT_TRUE(tempExpression.hasBooleanType());
    ASSERT_NO_THROW(tempExpression = intVarExpression > rationalVarExpression);
    EXPECT_TRUE(tempExpression.hasBooleanType());
    
    ASSERT_THROW(tempExpression = trueExpression >= piExpression, storm::exceptions::InvalidTypeException);
    ASSERT_NO_THROW(tempExpression = threeExpression >= threeExpression);
    EXPECT_TRUE(tempExpression.hasBooleanType());
    ASSERT_NO_THROW(tempExpression = intVarExpression >= rationalVarExpression);
    EXPECT_TRUE(tempExpression.hasBooleanType());
    
    ASSERT_THROW(tempExpression = trueExpression < piExpression, storm::exceptions::InvalidTypeException);
    ASSERT_NO_THROW(tempExpression = threeExpression < threeExpression);
    EXPECT_TRUE(tempExpression.hasBooleanType());
    ASSERT_NO_THROW(tempExpression = intVarExpression < rationalVarExpression);
    EXPECT_TRUE(tempExpression.hasBooleanType());
    
    ASSERT_THROW(tempExpression = trueExpression <= piExpression, storm::exceptions::InvalidTypeException);
    ASSERT_NO_THROW(tempExpression = threeExpression <= threeExpression);
    EXPECT_TRUE(tempExpression.hasBooleanType());
    ASSERT_NO_THROW(tempExpression = intVarExpression <= rationalVarExpression);
    EXPECT_TRUE(tempExpression.hasBooleanType());
    
    ASSERT_THROW(tempExpression = storm::expressions::minimum(trueExpression, piExpression), storm::exceptions::InvalidTypeException);
    ASSERT_NO_THROW(tempExpression = storm::expressions::minimum(threeExpression, threeExpression));
    EXPECT_TRUE(tempExpression.hasIntegralType());
    ASSERT_NO_THROW(tempExpression = storm::expressions::minimum(intVarExpression, rationalVarExpression));
    EXPECT_TRUE(tempExpression.hasRationalType());

    ASSERT_THROW(tempExpression = storm::expressions::maximum(trueExpression, piExpression), storm::exceptions::InvalidTypeException);
    ASSERT_NO_THROW(tempExpression = storm::expressions::maximum(threeExpression, threeExpression));
    EXPECT_TRUE(tempExpression.hasIntegralType());
    ASSERT_NO_THROW(tempExpression = storm::expressions::maximum(intVarExpression, rationalVarExpression));
    EXPECT_TRUE(tempExpression.hasRationalType());
    
    ASSERT_THROW(tempExpression = storm::expressions::implies(trueExpression, piExpression), storm::exceptions::InvalidTypeException);
    ASSERT_NO_THROW(tempExpression = storm::expressions::implies(trueExpression, falseExpression));
    EXPECT_TRUE(tempExpression.hasBooleanType());
    ASSERT_NO_THROW(tempExpression = storm::expressions::implies(boolVarExpression, boolVarExpression));
    EXPECT_TRUE(tempExpression.hasBooleanType());
    
    ASSERT_THROW(tempExpression = storm::expressions::iff(trueExpression, piExpression), storm::exceptions::InvalidTypeException);
    ASSERT_NO_THROW(tempExpression = storm::expressions::iff(trueExpression, falseExpression));
    EXPECT_TRUE(tempExpression.hasBooleanType());
    ASSERT_NO_THROW(tempExpression = storm::expressions::iff(boolVarExpression, boolVarExpression));
    EXPECT_TRUE(tempExpression.hasBooleanType());
    
    ASSERT_THROW(tempExpression = trueExpression != piExpression, storm::exceptions::InvalidTypeException);
    ASSERT_NO_THROW(tempExpression = trueExpression != falseExpression);
    EXPECT_TRUE(tempExpression.hasBooleanType());
    ASSERT_NO_THROW(tempExpression = boolVarExpression != boolVarExpression);
    EXPECT_TRUE(tempExpression.hasBooleanType());
    
    ASSERT_THROW(tempExpression = storm::expressions::floor(trueExpression), storm::exceptions::InvalidTypeException);
    ASSERT_NO_THROW(tempExpression = storm::expressions::floor(threeExpression));
    EXPECT_TRUE(tempExpression.hasIntegralType());
    ASSERT_NO_THROW(tempExpression = storm::expressions::floor(rationalVarExpression));
    EXPECT_TRUE(tempExpression.hasIntegralType());

    ASSERT_THROW(tempExpression = storm::expressions::ceil(trueExpression), storm::exceptions::InvalidTypeException);
    ASSERT_NO_THROW(tempExpression = storm::expressions::ceil(threeExpression));
    EXPECT_TRUE(tempExpression.hasIntegralType());
    ASSERT_NO_THROW(tempExpression = storm::expressions::ceil(rationalVarExpression));
    EXPECT_TRUE(tempExpression.hasIntegralType());

    ASSERT_THROW(tempExpression = trueExpression ^ piExpression, storm::exceptions::InvalidTypeException);
    ASSERT_NO_THROW(tempExpression = threeExpression ^ threeExpression);
    EXPECT_TRUE(tempExpression.hasIntegralType());
    ASSERT_NO_THROW(tempExpression = intVarExpression ^ rationalVarExpression);
    EXPECT_TRUE(tempExpression.hasRationalType());
}

TEST(Expression, SubstitutionTest) {
    std::shared_ptr<storm::expressions::ExpressionManager> manager(new storm::expressions::ExpressionManager());
    
    storm::expressions::Expression trueExpression;
    storm::expressions::Expression falseExpression;
    storm::expressions::Expression threeExpression;
    storm::expressions::Expression piExpression;
    storm::expressions::Expression boolVarExpression;
    storm::expressions::Expression intVarExpression;
    storm::expressions::Expression rationalVarExpression;
    
    ASSERT_NO_THROW(trueExpression = manager->boolean(true));
    ASSERT_NO_THROW(falseExpression = manager->boolean(false));
    ASSERT_NO_THROW(threeExpression = manager->integer(3));
    ASSERT_NO_THROW(piExpression = manager->rational(3.14));
    ASSERT_NO_THROW(boolVarExpression = manager->declareBooleanVariable("x"));
    ASSERT_NO_THROW(intVarExpression = manager->declareIntegerVariable("y"));
    ASSERT_NO_THROW(rationalVarExpression = manager->declareRationalVariable("z"));
    
    storm::expressions::Expression tempExpression;
    ASSERT_NO_THROW(tempExpression = (intVarExpression < threeExpression || boolVarExpression) && boolVarExpression);

    std::map<storm::expressions::Variable, storm::expressions::Expression> substution = { std::make_pair(manager->getVariable("y"), rationalVarExpression), std::make_pair(manager->getVariable("x"), manager->boolean(true)) };
    storm::expressions::Expression substitutedExpression;
    ASSERT_NO_THROW(substitutedExpression = tempExpression.substitute(substution));
    EXPECT_TRUE(substitutedExpression.simplify().isTrue());
}

TEST(Expression, SimplificationTest) {
    std::shared_ptr<storm::expressions::ExpressionManager> manager(new storm::expressions::ExpressionManager());
    
    storm::expressions::Expression trueExpression;
    storm::expressions::Expression falseExpression;
    storm::expressions::Expression threeExpression;
    storm::expressions::Expression intVarExpression;
    
    ASSERT_NO_THROW(trueExpression = manager->boolean(true));
    ASSERT_NO_THROW(falseExpression = manager->boolean(false));
    ASSERT_NO_THROW(threeExpression = manager->integer(3));
    ASSERT_NO_THROW(intVarExpression = manager->declareIntegerVariable("y"));
    
    storm::expressions::Expression tempExpression;
    storm::expressions::Expression simplifiedExpression;

    ASSERT_NO_THROW(tempExpression = trueExpression || intVarExpression > threeExpression);
    ASSERT_NO_THROW(simplifiedExpression = tempExpression.simplify());
    EXPECT_TRUE(simplifiedExpression.isTrue());

    ASSERT_NO_THROW(tempExpression = falseExpression && intVarExpression > threeExpression);
    ASSERT_NO_THROW(simplifiedExpression = tempExpression.simplify());
    EXPECT_TRUE(simplifiedExpression.isFalse());
}

TEST(Expression, SimpleEvaluationTest) {
    std::shared_ptr<storm::expressions::ExpressionManager> manager(new storm::expressions::ExpressionManager());
    
    storm::expressions::Expression trueExpression;
    storm::expressions::Expression falseExpression;
    storm::expressions::Expression threeExpression;
    storm::expressions::Expression piExpression;
    storm::expressions::Expression boolVarExpression;
    storm::expressions::Expression intVarExpression;
    storm::expressions::Expression rationalVarExpression;
    
    ASSERT_NO_THROW(trueExpression = manager->boolean(true));
    ASSERT_NO_THROW(falseExpression = manager->boolean(false));
    ASSERT_NO_THROW(threeExpression = manager->integer(3));
    ASSERT_NO_THROW(piExpression = manager->rational(3.14));
    ASSERT_NO_THROW(boolVarExpression = manager->declareBooleanVariable("x"));
    ASSERT_NO_THROW(intVarExpression = manager->declareIntegerVariable("y"));
    ASSERT_NO_THROW(rationalVarExpression = manager->declareRationalVariable("z"));
    
    storm::expressions::Expression tempExpression;
    
    ASSERT_NO_THROW(tempExpression = (intVarExpression < threeExpression || boolVarExpression) && boolVarExpression);
    
    ASSERT_NO_THROW(storm::expressions::SimpleValuation valuation(manager));
    storm::expressions::SimpleValuation valuation(manager);
    ASSERT_NO_THROW(valuation.setBooleanValue(manager->getVariable("x"), false));
    ASSERT_NO_THROW(valuation.setIntegerValue(manager->getVariable("y"), 0));
    ASSERT_NO_THROW(valuation.setRationalValue(manager->getVariable("z"), 0));
    
    ASSERT_THROW(tempExpression.evaluateAsDouble(&valuation), storm::exceptions::InvalidTypeException);
    ASSERT_THROW(tempExpression.evaluateAsInt(&valuation), storm::exceptions::InvalidTypeException);
    EXPECT_FALSE(tempExpression.evaluateAsBool(&valuation));
    ASSERT_NO_THROW(valuation.setIntegerValue(manager->getVariable("y"), 3));
    EXPECT_FALSE(tempExpression.evaluateAsBool(&valuation));
    
    ASSERT_NO_THROW(tempExpression = storm::expressions::ite(intVarExpression < threeExpression, trueExpression, falseExpression));
    ASSERT_THROW(tempExpression.evaluateAsDouble(&valuation), storm::exceptions::InvalidTypeException);
    ASSERT_THROW(tempExpression.evaluateAsInt(&valuation), storm::exceptions::InvalidTypeException);
    EXPECT_FALSE(tempExpression.evaluateAsBool(&valuation));
}

TEST(Expression, VisitorTest) {
    std::shared_ptr<storm::expressions::ExpressionManager> manager(new storm::expressions::ExpressionManager());
    
    storm::expressions::Expression threeExpression;
    storm::expressions::Expression piExpression;
    storm::expressions::Expression intVarExpression;
    storm::expressions::Expression rationalVarExpression;
    
    ASSERT_NO_THROW(threeExpression = manager->integer(3));
    ASSERT_NO_THROW(piExpression = manager->rational(3.14));
    ASSERT_NO_THROW(intVarExpression = manager->declareIntegerVariable("y"));
    ASSERT_NO_THROW(rationalVarExpression = manager->declareRationalVariable("z"));
        
    storm::expressions::Expression tempExpression = intVarExpression + rationalVarExpression * threeExpression;
    storm::expressions::LinearityCheckVisitor visitor;
    EXPECT_TRUE(visitor.check(tempExpression));
}