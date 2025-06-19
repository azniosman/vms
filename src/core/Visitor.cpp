#include "Visitor.h"

Visitor::Visitor()
    : type(VisitorType::Guest)
    , consent(false)
    , retentionPeriod(30)
{
}

Visitor::~Visitor()
{
} 