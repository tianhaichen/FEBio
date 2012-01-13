#include "stdafx.h"
#include "FELinearSolidDomain.h"
#include "FELinearSolidSolver.h"
#include "FEAnalysisStep.h"
#include "FEBioLib/FETransverselyIsotropic.h"

//-----------------------------------------------------------------------------
void FELinearSolidDomain::Reset()
{
	for (int i=0; i<(int) m_Elem.size(); ++i) m_Elem[i].Init(true);
}


//-----------------------------------------------------------------------------
//! \todo The material point initialization needs to move to the base class.
bool FELinearSolidDomain::Initialize(FEModel &mdl)
{
	// initialize base class
	FESolidDomain::Initialize(mdl);

	// initialize material point data
	FEM& fem = dynamic_cast<FEM&>(mdl);
	FEAnalysisStep* pstep = dynamic_cast<FEAnalysisStep*>(fem.m_pStep);

	bool bmerr = false;

	for (size_t i=0; i<m_Elem.size(); ++i)
	{
		FESolidElement& el = m_Elem[i];
		if (dynamic_cast<FELinearSolidSolver*>(pstep->m_psolver))
		{
			// get the elements material
			FEElasticMaterial* pme = fem.GetElasticMaterial(m_pMat);

			// set the local element coordinates
			if (pme)
			{
				if (pme->m_pmap)
				{
					for (int n=0; n<el.GaussPoints(); ++n)
					{
						FEElasticMaterialPoint& pt = *el.m_State[n]->ExtractData<FEElasticMaterialPoint>();
						pt.Q = pme->m_pmap->LocalElementCoord(el, n);
					}
				}
				else
				{
					if (fem.GetDebugFlag())
					{
						// If we get here, then the element has a user-defined fiber axis
						// we should check to see if it has indeed been specified.
						// TODO: This assumes that pt.Q will not get intialized to
						//		 a valid value. I should find another way for checking since I
						//		 would like pt.Q always to be initialized to a decent value.
						if (dynamic_cast<FETransverselyIsotropic*>(pme))
						{
							FEElasticMaterialPoint& pt = *el.m_State[0]->ExtractData<FEElasticMaterialPoint>();
							mat3d& m = pt.Q;
							if (fabs(m.det() - 1) > 1e-7)
							{
								// this element did not get specified a user-defined fiber direction
//								clog.printbox("ERROR", "Solid element %d was not assigned a fiber direction.", i+1);
								bmerr = true;
							}
						}
					}
				}
			}
		}
	}

	return (bmerr == false);
}

//-----------------------------------------------------------------------------
void FELinearSolidDomain::InitElements()
{
	vec3d x0[8], xt[8], r0, rt;
	FEMesh& m = *GetMesh();
	for (size_t i=0; i<m_Elem.size(); ++i)
	{
		FESolidElement& el = m_Elem[i];
		int neln = el.Nodes();
		for (int i=0; i<neln; ++i)
		{
			x0[i] = m.Node(el.m_node[i]).m_r0;
			xt[i] = m.Node(el.m_node[i]).m_rt;
		}

		int n = el.GaussPoints();
		for (int j=0; j<n; ++j) 
		{
			r0 = el.Evaluate(x0, j);
			rt = el.Evaluate(xt, j);

			FEMaterialPoint& mp = *el.m_State[j];
			FEElasticMaterialPoint& pt = *mp.ExtractData<FEElasticMaterialPoint>();
			pt.r0 = r0;
			pt.rt = rt;

			pt.J = defgrad(el, pt.F, j);

			el.m_State[j]->Init(false);
		}
	}
}

//-----------------------------------------------------------------------------
//! Unpack the element. That is, copy element data in traits structure
//!
void FELinearSolidDomain::UnpackLM(FEElement& el, vector<int>& lm)
{
	int N = el.Nodes();
	lm.resize(3*N);

	for (int i=0; i<N; ++i)
	{
		int n = el.m_node[i];
		FENode& node = m_pMesh->Node(n);

		int* id = node.m_ID;

		// get displacement DOFs
		lm[3*i  ] = id[DOF_X];
		lm[3*i+1] = id[DOF_Y];
		lm[3*i+2] = id[DOF_Z];
	}
}

//-----------------------------------------------------------------------------
void FELinearSolidDomain::StiffnessMatrix(FELinearSolidSolver* psolver)
{
	FEM& fem = dynamic_cast<FEM&>(psolver->GetFEModel());
	vector<int> elm;
	for (int i=0; i<(int) m_Elem.size(); ++i)
	{
		// get the next element
		FESolidElement& el = m_Elem[i];

		// number of element nodes
		int ne = el.Nodes();

		// build the element stiffness matrix
		matrix ke(3*ne, 3*ne);
		ElementStiffness(fem, el, ke);

		// set up the LM matrix
		UnpackLM(el, elm);

		// assemble into global matrix
		psolver->AssembleStiffness(ke, elm);
	}
}

//-----------------------------------------------------------------------------
//! Calculate the element stiffness matrix
void FELinearSolidDomain::ElementStiffness(FEM &fem, FESolidElement &el, matrix &ke)
{
	int i, i3, j, j3, n;

	// Get the current element's data
	const int nint = el.GaussPoints();
	const int neln = el.Nodes();
	const int ndof = 3*neln;

	// global derivatives of shape functions
	// NOTE: hard-coding of hex elements!
	// Gx = dH/dx
	double Gx[8], Gy[8], Gz[8];

	double Gxi, Gyi, Gzi;
	double Gxj, Gyj, Gzj;

	// The 'D' matrix
	double D[6][6] = {0};	// The 'D' matrix

	// The 'D*BL' matrix
	double DBL[6][3];

	double *Grn, *Gsn, *Gtn;
	double Gr, Gs, Gt;

	// jacobian
	double Ji[3][3], detJ0;
	
	// weights at gauss points
	const double *gw = el.GaussWeights();

	// get the material
	FESolidMaterial* pmat = dynamic_cast<FESolidMaterial*>(m_pMat);
	assert(pmat);

	// calculate element stiffness matrix
	ke.zero();
	for (n=0; n<nint; ++n)
	{
		// calculate jacobian
		detJ0 = invjac0(el, Ji, n)*gw[n];

		Grn = el.Gr(n);
		Gsn = el.Gs(n);
		Gtn = el.Gt(n);

		// setup the material point
		// NOTE: deformation gradient and determinant have already been evaluated in the stress routine
		FEMaterialPoint& mp = *el.m_State[n];
		FEElasticMaterialPoint& pt = *(mp.ExtractData<FEElasticMaterialPoint>());

		for (i=0; i<neln; ++i)
		{
			Gr = Grn[i];
			Gs = Gsn[i];
			Gt = Gtn[i];

			// calculate global gradient of shape functions
			// note that we need the transposed of Ji, not Ji itself !
			Gx[i] = Ji[0][0]*Gr+Ji[1][0]*Gs+Ji[2][0]*Gt;
			Gy[i] = Ji[0][1]*Gr+Ji[1][1]*Gs+Ji[2][1]*Gt;
			Gz[i] = Ji[0][2]*Gr+Ji[1][2]*Gs+Ji[2][2]*Gt;
		}

		// get the 'D' matrix
		tens4ds C = pmat->Tangent(mp);
		C.extract(D);

		// we only calculate the upper triangular part
		// since ke is symmetric. The other part is
		// determined below using this symmetry.
		for (i=0, i3=0; i<neln; ++i, i3 += 3)
		{
			Gxi = Gx[i];
			Gyi = Gy[i];
			Gzi = Gz[i];

			for (j=i, j3 = i3; j<neln; ++j, j3 += 3)
			{
				Gxj = Gx[j];
				Gyj = Gy[j];
				Gzj = Gz[j];

				// calculate D*BL matrices
				DBL[0][0] = (D[0][0]*Gxj+D[0][3]*Gyj+D[0][5]*Gzj);
				DBL[0][1] = (D[0][1]*Gyj+D[0][3]*Gxj+D[0][4]*Gzj);
				DBL[0][2] = (D[0][2]*Gzj+D[0][4]*Gyj+D[0][5]*Gxj);

				DBL[1][0] = (D[1][0]*Gxj+D[1][3]*Gyj+D[1][5]*Gzj);
				DBL[1][1] = (D[1][1]*Gyj+D[1][3]*Gxj+D[1][4]*Gzj);
				DBL[1][2] = (D[1][2]*Gzj+D[1][4]*Gyj+D[1][5]*Gxj);

				DBL[2][0] = (D[2][0]*Gxj+D[2][3]*Gyj+D[2][5]*Gzj);
				DBL[2][1] = (D[2][1]*Gyj+D[2][3]*Gxj+D[2][4]*Gzj);
				DBL[2][2] = (D[2][2]*Gzj+D[2][4]*Gyj+D[2][5]*Gxj);

				DBL[3][0] = (D[3][0]*Gxj+D[3][3]*Gyj+D[3][5]*Gzj);
				DBL[3][1] = (D[3][1]*Gyj+D[3][3]*Gxj+D[3][4]*Gzj);
				DBL[3][2] = (D[3][2]*Gzj+D[3][4]*Gyj+D[3][5]*Gxj);

				DBL[4][0] = (D[4][0]*Gxj+D[4][3]*Gyj+D[4][5]*Gzj);
				DBL[4][1] = (D[4][1]*Gyj+D[4][3]*Gxj+D[4][4]*Gzj);
				DBL[4][2] = (D[4][2]*Gzj+D[4][4]*Gyj+D[4][5]*Gxj);

				DBL[5][0] = (D[5][0]*Gxj+D[5][3]*Gyj+D[5][5]*Gzj);
				DBL[5][1] = (D[5][1]*Gyj+D[5][3]*Gxj+D[5][4]*Gzj);
				DBL[5][2] = (D[5][2]*Gzj+D[5][4]*Gyj+D[5][5]*Gxj);

				ke[i3  ][j3  ] += (Gxi*DBL[0][0] + Gyi*DBL[3][0] + Gzi*DBL[5][0] )*detJ0;
				ke[i3  ][j3+1] += (Gxi*DBL[0][1] + Gyi*DBL[3][1] + Gzi*DBL[5][1] )*detJ0;
				ke[i3  ][j3+2] += (Gxi*DBL[0][2] + Gyi*DBL[3][2] + Gzi*DBL[5][2] )*detJ0;

				ke[i3+1][j3  ] += (Gyi*DBL[1][0] + Gxi*DBL[3][0] + Gzi*DBL[4][0] )*detJ0;
				ke[i3+1][j3+1] += (Gyi*DBL[1][1] + Gxi*DBL[3][1] + Gzi*DBL[4][1] )*detJ0;
				ke[i3+1][j3+2] += (Gyi*DBL[1][2] + Gxi*DBL[3][2] + Gzi*DBL[4][2] )*detJ0;

				ke[i3+2][j3  ] += (Gzi*DBL[2][0] + Gyi*DBL[4][0] + Gxi*DBL[5][0] )*detJ0;
				ke[i3+2][j3+1] += (Gzi*DBL[2][1] + Gyi*DBL[4][1] + Gxi*DBL[5][1] )*detJ0;
				ke[i3+2][j3+2] += (Gzi*DBL[2][2] + Gyi*DBL[4][2] + Gxi*DBL[5][2] )*detJ0;
			}
		}
	}

	// assign symmetic parts
	// TODO: Can this be omitted by changing the Assemble routine so that it only
	// grabs elements from the upper diagonal matrix?
	for (i=0; i<ndof; ++i)
		for (j=i+1; j<ndof; ++j)
			ke[j][i] = ke[i][j];
}

//-----------------------------------------------------------------------------
void FELinearSolidDomain::RHS(FELinearSolidSolver *psolver, vector<double>& R)
{
	FEM& fem = dynamic_cast<FEM&>(psolver->GetFEModel());

	// element force vector
	vector<double> fe;

	vector<int> lm;

	int NE = m_Elem.size();
	for (int i=0; i<NE; ++i)
	{
		// get the element
		FESolidElement& el = m_Elem[i];

		// get the element force vector and initialize it to zero
		int ndof = 3*el.Nodes();
		fe.assign(ndof, 0);

		// calculate initial stress vector
		InitialStress(el, fe);

		// calculate internal force vector (for material non-lineararity)
		InternalForce(el, fe);

		// apply body forces
//		if (fem.HasBodyForces()) BodyForces(fem, el, fe);

		// get the element's LM vector
		UnpackLM(el, lm);

		// assemble element 'fe'-vector into global R vector
		psolver->AssembleResidual(el.m_node, lm, fe, R);
	}
}

//-----------------------------------------------------------------------------
//! calculates the equivalent nodal forces for intial stress for solid elements
//!
void FELinearSolidDomain::InitialStress(FESolidElement& el, vector<double>& fe)
{
	int i, n;

	// jacobian matrix, inverse jacobian matrix and determinants
	double Ji[3][3], detJ0;

	double Gx, Gy, Gz;
	mat3ds s;

	const double* Gr, *Gs, *Gt;

	int nint = el.GaussPoints();
	int neln = el.Nodes();

	double*	gw = el.GaussWeights();

	// repeat for all integration points
	for (n=0; n<nint; ++n)
	{
		FEMaterialPoint& mp = *el.m_State[n];
		FEElasticMaterialPoint& pt = *(mp.ExtractData<FEElasticMaterialPoint>());

		// calculate the jacobian
		detJ0 = invjac0(el, Ji, n);

		detJ0 *= gw[n];

		// get the stress vector for this integration point
		s = pt.s0;

		Gr = el.Gr(n);
		Gs = el.Gs(n);
		Gt = el.Gt(n);

		for (i=0; i<neln; ++i)
		{
			// calculate global gradient of shape functions
			// note that we need the transposed of Ji, not Ji itself !
			Gx = Ji[0][0]*Gr[i]+Ji[1][0]*Gs[i]+Ji[2][0]*Gt[i];
			Gy = Ji[0][1]*Gr[i]+Ji[1][1]*Gs[i]+Ji[2][1]*Gt[i];
			Gz = Ji[0][2]*Gr[i]+Ji[1][2]*Gs[i]+Ji[2][2]*Gt[i];

			// calculate internal force
			// the '-' sign is so that the internal forces get subtracted
			// from the global residual vector
			fe[3*i  ] -= ( Gx*s.xx() +
				           Gy*s.xy() +
					       Gz*s.xz() )*detJ0;

			fe[3*i+1] -= ( Gy*s.yy() +
				           Gx*s.xy() +
					       Gz*s.yz() )*detJ0;

			fe[3*i+2] -= ( Gz*s.zz() +
				           Gy*s.yz() +
					       Gx*s.xz() )*detJ0;
		}
	}
}

//-----------------------------------------------------------------------------
//! calculates the equivalent nodal forces for intial stress for solid elements
//!
void FELinearSolidDomain::InternalForce(FESolidElement& el, vector<double>& fe)
{
	int i, n;

	// jacobian matrix, inverse jacobian matrix and determinants
	double Ji[3][3], detJ0;

	double Gx, Gy, Gz;
	mat3ds s;

	const double* Gr, *Gs, *Gt;

	int nint = el.GaussPoints();
	int neln = el.Nodes();

	double*	gw = el.GaussWeights();

	// repeat for all integration points
	for (n=0; n<nint; ++n)
	{
		FEMaterialPoint& mp = *el.m_State[n];
		FEElasticMaterialPoint& pt = *(mp.ExtractData<FEElasticMaterialPoint>());

		// calculate the jacobian
		detJ0 = invjac0(el, Ji, n);

		detJ0 *= gw[n];

		// get the stress vector for this integration point
		s = pt.s;

		Gr = el.Gr(n);
		Gs = el.Gs(n);
		Gt = el.Gt(n);

		for (i=0; i<neln; ++i)
		{
			// calculate global gradient of shape functions
			// note that we need the transposed of Ji, not Ji itself !
			Gx = Ji[0][0]*Gr[i]+Ji[1][0]*Gs[i]+Ji[2][0]*Gt[i];
			Gy = Ji[0][1]*Gr[i]+Ji[1][1]*Gs[i]+Ji[2][1]*Gt[i];
			Gz = Ji[0][2]*Gr[i]+Ji[1][2]*Gs[i]+Ji[2][2]*Gt[i];

			// calculate internal force
			// the '-' sign is so that the internal forces get subtracted
			// from the global residual vector
			fe[3*i  ] -= ( Gx*s.xx() +
				           Gy*s.xy() +
					       Gz*s.xz() )*detJ0;

			fe[3*i+1] -= ( Gy*s.yy() +
				           Gx*s.xy() +
					       Gz*s.yz() )*detJ0;

			fe[3*i+2] -= ( Gz*s.zz() +
				           Gy*s.yz() +
					       Gx*s.xz() )*detJ0;
		}
	}
}

//-----------------------------------------------------------------------------
void FELinearSolidDomain::UpdateStresses(FEModel &fem)
{
	int i, n;
	int nint, neln;
	double* gw;

	for (i=0; i<(int) m_Elem.size(); ++i)
	{
		// get the solid element
		FESolidElement& el = m_Elem[i];

		assert(!el.IsRigid());

		assert(el.Type() != FE_UDGHEX);

		// get the number of integration points
		nint = el.GaussPoints();

		// number of nodes
		neln = el.Nodes();

		// nodall coordinates
		vec3d r0[8], rt[8];
		for (int j=0; j<neln; ++j)
		{
			r0[j] = m_pMesh->Node(el.m_node[j]).m_r0;
			rt[j] = m_pMesh->Node(el.m_node[j]).m_rt;
		}

		// get the integration weights
		gw = el.GaussWeights();

		// get the material
		FESolidMaterial* pm = dynamic_cast<FESolidMaterial*>(m_pMat);
		assert(pm);

		// extract the elastic component
		FEElasticMaterial* pme = fem.GetElasticMaterial(pm);
		assert(pme);

		// loop over the integration points and calculate
		// the stress at the integration point
		for (n=0; n<nint; ++n)
		{
			FEMaterialPoint& mp = *el.m_State[n];
			FEElasticMaterialPoint& pt = *(mp.ExtractData<FEElasticMaterialPoint>());

			// material point coordinates
			// TODO: I'm not entirly happy with this solution
			//		 since the material point coordinates are not used by most materials.
			pt.r0 = el.Evaluate(r0, n);
			pt.rt = el.Evaluate(rt, n);

			// get the deformation gradient and determinant
			// TODO: I should not evaulate this, since this can throw negative jacobians
			//       for large deformations. I known I shouldn't use this for large
			//       deformations, but in principle there should never be a negative 
			//       jacobian for small deformations!
			pt.J = defgrad(el, pt.F, n);

			// calculate the stress at this material point
			pt.s = pm->Stress(mp) + pt.s0;
		}
	}
}
