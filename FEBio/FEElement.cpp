// FEElement.cpp: implementation of the FEElement class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "FEElement.h"
#include <math.h>

FESolidElement::~FESolidElement()
{
	// clean up material data
	int nint = GaussPoints();
	assert(m_State.size() == nint);
	for (int i=0; i<nint; ++i) if (m_State[i]) delete m_State[i];
	zero(m_State);
}

FEShellElement::~FEShellElement()
{
	// clean up material data
	int nint = GaussPoints();
	assert(m_State.size() == nint);
	for (int i=0; i<nint; ++i) if (m_State[i]) delete m_State[i];
	zero(m_State);
}

FETrussElement::~FETrussElement()
{
	// clean up material data
	int nint = GaussPoints();
	assert(m_State.size() == nint);
	for (int i=0; i<nint; ++i) if (m_State[i]) delete m_State[i];
	zero(m_State);
}
