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
#include "FECore/FEModelLoad.h"
#include "febiomech_api.h"
//-----------------------------------------------------------------------------
//! a follower force on a rigid body
class FEBIOMECH_API FERigidFollowerForce : public FEModelLoad
{
public:
    //! constructor
    FERigidFollowerForce(FEModel* pfem);

    //! initialization
    bool Init() override;

    //! Serialization
    void Serialize(DumpStream& ar) override;

    //! Residual
    void LoadVector(FEGlobalVector& R, const FETimeInfo& tp) override;

    //! Stiffness matrix
    void StiffnessMatrix(FELinearSystem& LS, const FETimeInfo& tp) override;

public:
    int     m_rid;      //!< rigid body ID
    vec3d   m_X;        //!< coordinates of attachements in reference state
    vec3d   m_f;        //!< force
    bool    m_brelative;        //!< if active, the ra0 and rb0 are relative w.r.t. the COM

    DECLARE_FECORE_CLASS();
};
