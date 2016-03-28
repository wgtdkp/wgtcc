#include "error.h"
#include "visitor.h"

/*************** Constant Evaluator ******************/
void ConstantEvaluator::VisitBinaryOp(BinaryOp* binaryOp)
{
	if (!binaryOp->Ty()->IsInteger())
		Error("integer expected");

	ConstantEvaluator lEvaluator;
	ConstantEvaluator rEvaluator;
	lEvaluator.visit

}
