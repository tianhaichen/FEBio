/*This file is part of the FEBio source code and is licensed under the MIT license
listed below.

See Copyright-FEBio.txt for details.

Copyright (c) 2021 University of Utah, The Trustees of Columbia University in
the City of New York, and others.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.*/
#pragma once
#include "FECoreBase.h"
#include "DataRecord.h"

//-----------------------------------------------------------------------------
//! This is the base class for a model data
class FECORE_API FEModelLogData : public FECoreBase
{
	FECORE_SUPER_CLASS

public:
	FEModelLogData(FEModel* fem);
	virtual ~FEModelLogData();
	virtual double value() = 0;
};

//-----------------------------------------------------------------------------
//! This class records model data
class FECORE_API FEModelDataRecord : public DataRecord
{
public:
	FEModelDataRecord(FEModel* pfem, const char* szfile);
	double Evaluate(int item, int ndata);
	void SetData(const char* sz);
	void ClearData();
	void SelectAllItems();
	int Size() const;

private:
	std::vector<FEModelLogData*>	m_data;
};
