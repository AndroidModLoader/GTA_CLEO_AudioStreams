#pragma once

#include "matrix.h"
class CPlaceable;

class CMatrixLink : public CMatrix
{
public:
    CPlaceable *m_pOwner;
    CMatrixLink *m_pPrev;
    CMatrixLink *m_pNext;
};