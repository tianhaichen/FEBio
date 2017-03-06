//
//  BFGSSolver2.h
//  FECore
//
//  Created by Gerard Ateshian on 10/12/15.
//  Copyright © 2015 febio.org. All rights reserved.
//

#pragma once

#include "matrix.h"
#include "vector.h"
#include "FENewtonStrategy.h"
#include "LinearSolver.h"

//-----------------------------------------------------------------------------
//! The BFGSSolver solves a nonlinear system of equations using the BFGS method.
//! It depends on the NonLinearSystem to evaluate the function and its jacobian.

class BFGSSolver2 : public FENewtonStrategy
{
public:
    //! constructor
    BFGSSolver2();
    
    //! New initialization method
    void Init(int neq, LinearSolver* pls);
    
    //! perform a BFGS udpate
    bool Update(double s, vector<double>& ui, vector<double>& R0, vector<double>& R1);
    
    //! solve the equations
    void SolveEquations(vector<double>& x, vector<double>& b);
    
public:
    // keep a pointer to the linear solver
    LinearSolver*	m_plinsolve;	//!< pointer to linear solver
    int				m_neq;		//!< number of equations
    
    // BFGS update vectors
    vector<double>	m_dx;       //!< temp vectors for calculating BFGS update vectors
    vector<double>	m_f;        //!< temp vectors for calculating BFGS update vectors
};
