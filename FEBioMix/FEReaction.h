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
#include "FECore/FEMaterial.h"
#include "FEBioMix/FESolutesMaterialPoint.h"
#include <map>

//-----------------------------------------------------------------------------
class FEMultiphasic;
class FEFluidSolutes;
class FESolutesMaterial;
class FEMultiphasicFSI;

//-----------------------------------------------------------------------------
//! Base class for reactions.

typedef std::map<int,int> intmap;
typedef std::map<int,int>::iterator itrmap;

class FEBIOMIX_API FEReaction : public FEMaterial
{
public:
    //! constructor
    FEReaction(FEModel* pfem);
    
    //! initialization
    bool Init() override;
    
public:
    //! set stoichiometric coefficient
    void SetStoichiometricCoefficient(intmap& RP, int id, int v) { RP.insert(std::pair<int, int>(id, v)); }
    
public:
    FEMultiphasic*    m_pMP;        //!< pointer to multiphasic material where reaction occurs
    FEFluidSolutes*    m_pFS;        //!< pointer to fluid solutes material where reaction occurs
    FESolutesMaterial* m_pSM;       //!< pointer to solute (split) material where reaction occurs
    FEMultiphasicFSI* m_pMF;       //!< pointer to multiphasic fsi material where reaction occurs
};
