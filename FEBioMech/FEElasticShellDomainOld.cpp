#include "stdafx.h"
#include "FEElasticShellDomainOld.h"
#include "FEElasticMaterial.h"
#include "FEBodyForce.h"
#include <FECore/log.h>
#include <FECore/FEModel.h>
#include <FECore/FEAnalysis.h>
#include <math.h>

//-----------------------------------------------------------------------------
FEElasticShellDomainOld::FEElasticShellDomainOld(FEModel* pfem) : FEShellDomain(&pfem->GetMesh()), FEElasticDomain(pfem)
{
	m_pMat = 0;
	m_dofU = pfem->GetDOFIndex("u");
	m_dofV = pfem->GetDOFIndex("v");
	m_dofW = pfem->GetDOFIndex("w");
}

//-----------------------------------------------------------------------------
void FEElasticShellDomainOld::SetMaterial(FEMaterial* pmat)
{
	m_pMat = dynamic_cast<FESolidMaterial*>(pmat);
}

//-----------------------------------------------------------------------------
bool FEElasticShellDomainOld::Initialize(FEModel& mdl)
{
	// initialize base class
	FEShellDomain::Initialize();

	// error flag (set true on error)
	bool bmerr = false;

	// set the local coordinate system for each integration point
	FECoordSysMap* pmap = m_pMat->GetElasticMaterial()->GetCoordinateSystemMap();
	for (size_t i=0; i<m_Elem.size(); ++i)
	{
		// unpack element data
		FEShellElement& el = m_Elem[i];

		// set the local element coordinates
		if (pmap)
		{
			for (int n=0; n<el.GaussPoints(); ++n)
			{
				FEElasticMaterialPoint& pt = *el.GetMaterialPoint(n)->ExtractData<FEElasticMaterialPoint>();
				pt.m_Q = pmap->LocalElementCoord(el, n);
			}
		}
	}

	// check for initially inverted shells
	for (int i=0; i<Elements(); ++i)
	{
		FEShellElement& el = Element(i);
		int nint = el.GaussPoints();
		for (int n=0; n<nint; ++n)
		{
			double J0 = detJ0(el, n);
			if (J0 <= 0)
			{
				felog.printf("**************************** E R R O R ****************************\n");
				felog.printf("Negative jacobian detected at integration point %d of element %d\n", n+1, el.GetID());
				felog.printf("Jacobian = %lg\n", J0);
				felog.printf("Did you use the right node numbering?\n");
				felog.printf("Nodes:");
				for (int l=0; l<el.Nodes(); ++l)
				{
					felog.printf("%d", el.m_node[l]+1);
					if (l+1 != el.Nodes()) felog.printf(","); else felog.printf("\n");
				}
				felog.printf("*******************************************************************\n\n");
				bmerr = true;
			}
		}
	}

	return (bmerr == false);
}

//-----------------------------------------------------------------------------
void FEElasticShellDomainOld::Activate()
{
	for (int i=0; i<Nodes(); ++i)
	{
		FENode& node = Node(i);
		if (node.HasFlags(FENode::EXCLUDE) == false)
		{
			if (node.m_rid < 0)
			{
				node.m_ID[m_dofX] = DOF_ACTIVE;
				node.m_ID[m_dofY] = DOF_ACTIVE;
				node.m_ID[m_dofZ] = DOF_ACTIVE;
			}

			if (node.HasFlags(FENode::SHELL))
			{
				node.m_ID[m_dofU] = DOF_ACTIVE;
				node.m_ID[m_dofV] = DOF_ACTIVE;
				node.m_ID[m_dofW] = DOF_ACTIVE;
			}
		}
	}
}

//-----------------------------------------------------------------------------
//! calculates covariant basis vectors at an integration point
void FEElasticShellDomainOld::CoBaseVectors0(FEShellElement& el, int n, vec3d g[3])
{
	int i;

	int neln = el.Nodes();

	// current nodal coordinates and directors
	vec3d r[FEElement::MAX_NODES], D[FEElement::MAX_NODES];
	for (i = 0; i<neln; ++i)
	{
		FENode& ni = m_pMesh->Node(el.m_node[i]);
		r[i] = ni.m_r0;
		D[i] = el.m_D0[i];
	}

	double eta = el.gt(n);

	double* Mr = el.Hr(n);
	double* Ms = el.Hs(n);
	double* M = el.H(n);

	// initialize covariant basis vectors
	g[0] = g[1] = g[2] = vec3d(0, 0, 0);

	for (i = 0; i<neln; ++i)
	{
		g[0] += (r[i] + D[i] * eta / 2)*Mr[i];
		g[1] += (r[i] + D[i] * eta / 2)*Ms[i];
		g[2] += D[i] * (M[i] / 2);
	}
}

//-----------------------------------------------------------------------------
//! calculates contravariant basis vectors at an integration point
void FEElasticShellDomainOld::ContraBaseVectors0(FEShellElement& el, int n, vec3d gcnt[3])
{
	vec3d gcov[3];
	CoBaseVectors0(el, n, gcov);

	mat3d J = mat3d(gcov[0].x, gcov[1].x, gcov[2].x,
		gcov[0].y, gcov[1].y, gcov[2].y,
		gcov[0].z, gcov[1].z, gcov[2].z);
	mat3d Ji = J.inverse();

	gcnt[0] = vec3d(Ji(0, 0), Ji(0, 1), Ji(0, 2));
	gcnt[1] = vec3d(Ji(1, 0), Ji(1, 1), Ji(1, 2));
	gcnt[2] = vec3d(Ji(2, 0), Ji(2, 1), Ji(2, 2));

}

//-----------------------------------------------------------------------------
double FEElasticShellDomainOld::invjac0(FEShellElement& el, double Ji[3][3], int n)
{
	vec3d gcov[3];
	CoBaseVectors0(el, n, gcov);

	mat3d J = mat3d(gcov[0].x, gcov[1].x, gcov[2].x,
		gcov[0].y, gcov[1].y, gcov[2].y,
		gcov[0].z, gcov[1].z, gcov[2].z);

	double det = J.det();

	// make sure the determinant is positive
	if (det <= 0) throw NegativeJacobian(el.GetID(), n + 1, det);

	// calculate the inverse of the jacobian
	double deti = 1.0 / det;

	Ji[0][0] = deti*(J[1][1] * J[2][2] - J[1][2] * J[2][1]);
	Ji[1][0] = deti*(J[1][2] * J[2][0] - J[1][0] * J[2][2]);
	Ji[2][0] = deti*(J[1][0] * J[2][1] - J[1][1] * J[2][0]);

	Ji[0][1] = deti*(J[0][2] * J[2][1] - J[0][1] * J[2][2]);
	Ji[1][1] = deti*(J[0][0] * J[2][2] - J[0][2] * J[2][0]);
	Ji[2][1] = deti*(J[0][1] * J[2][0] - J[0][0] * J[2][1]);

	Ji[0][2] = deti*(J[0][1] * J[1][2] - J[1][1] * J[0][2]);
	Ji[1][2] = deti*(J[0][2] * J[1][0] - J[0][0] * J[1][2]);
	Ji[2][2] = deti*(J[0][0] * J[1][1] - J[0][1] * J[1][0]);

	return det;
}

//-----------------------------------------------------------------------------
//! Calculate jacobian with respect to reference frame
double FEElasticShellDomainOld::detJ0(FEShellElement &el, int n)
{
	vec3d gcov[3];
	CoBaseVectors0(el, n, gcov);

	mat3d J = mat3d(gcov[0].x, gcov[1].x, gcov[2].x,
		gcov[0].y, gcov[1].y, gcov[2].y,
		gcov[0].z, gcov[1].z, gcov[2].z);
	return J.det();
}

//-----------------------------------------------------------------------------
//! calculates covariant basis vectors at an integration point
void FEElasticShellDomainOld::CoBaseVectors(FEShellElement& el, int n, vec3d g[3])
{
    int i;
    
    int neln = el.Nodes();

    // current nodal coordinates and directors
    vec3d r[FEElement::MAX_NODES], D[FEElement::MAX_NODES];
    for (i=0; i<neln; ++i)
    {
        FENode& ni = m_pMesh->Node(el.m_node[i]);
        r[i] = ni.m_rt;
        D[i] = el.m_D0[i] + ni.get_vec3d(m_dofU, m_dofV, m_dofW);
    }
    
    double eta = el.gt(n);
    
    double* Mr = el.Hr(n);
    double* Ms = el.Hs(n);
    double* M  = el.H(n);
    
    // initialize covariant basis vectors
    g[0] = g[1] = g[2] = vec3d(0,0,0);
    
    for (i=0; i<neln; ++i)
    {
        g[0] += (r[i] + D[i]*eta/2)*Mr[i];
        g[1] += (r[i] + D[i]*eta/2)*Ms[i];
        g[2] += D[i]*(M[i]/2);
    }
}

//-----------------------------------------------------------------------------
//! calculates contravariant basis vectors at an integration point
void FEElasticShellDomainOld::ContraBaseVectors(FEShellElement& el, int n, vec3d gcnt[3])
{
    vec3d gcov[3];
    CoBaseVectors(el, n, gcov);
    
    mat3d J = mat3d(gcov[0].x, gcov[1].x, gcov[2].x,
                    gcov[0].y, gcov[1].y, gcov[2].y,
                    gcov[0].z, gcov[1].z, gcov[2].z);
    mat3d Ji = J.inverse();
    
    gcnt[0] = vec3d(Ji(0,0),Ji(0,1),Ji(0,2));
    gcnt[1] = vec3d(Ji(1,0),Ji(1,1),Ji(1,2));
    gcnt[2] = vec3d(Ji(2,0),Ji(2,1),Ji(2,2));

}

//-----------------------------------------------------------------------------
// jacobian with respect to current frame
double FEElasticShellDomainOld::detJ(FEShellElement& el, int n)
{
    vec3d gcov[3];
    CoBaseVectors(el, n, gcov);
    
    mat3d J = mat3d(gcov[0].x, gcov[1].x, gcov[2].x,
                    gcov[0].y, gcov[1].y, gcov[2].y,
                    gcov[0].z, gcov[1].z, gcov[2].z);
    return J.det();
}

//-----------------------------------------------------------------------------
double FEElasticShellDomainOld::defgrad(FEShellElement& el, mat3d& F, int n)
{
    vec3d gcov[3], Gcnt[3];
    CoBaseVectors(el, n, gcov);
    ContraBaseVectors0(el, n, Gcnt);
    
    F = (gcov[0] & Gcnt[0]) + (gcov[1] & Gcnt[1]) + (gcov[2] & Gcnt[2]);
	double J = F.det();
	if (J <= 0) throw NegativeJacobian(el.GetID(), n, J, &el);

	return J;
}

//-----------------------------------------------------------------------------
//! Calculate the inverse jacobian with respect to the current frame at 
//! integration point n. The inverse jacobian is return in Ji. The return value
//! is the determinant of the jacobian (not the inverse!)
double FEElasticShellDomainOld::invjact(FEShellElement& el, double Ji[3][3], int n)
{
    vec3d gcov[3];
    CoBaseVectors(el, n, gcov);
    
    mat3d J = mat3d(gcov[0].x, gcov[1].x, gcov[2].x,
                    gcov[0].y, gcov[1].y, gcov[2].y,
                    gcov[0].z, gcov[1].z, gcov[2].z);
    
    double det = J.det();
    
    // make sure the determinant is positive
    if (det <= 0) throw NegativeJacobian(el.GetID(), n+1, det);
    
    // calculate the inverse of the jacobian
    double deti = 1.0 / det;
    
    Ji[0][0] =  deti*(J[1][1]*J[2][2] - J[1][2]*J[2][1]);
    Ji[1][0] =  deti*(J[1][2]*J[2][0] - J[1][0]*J[2][2]);
    Ji[2][0] =  deti*(J[1][0]*J[2][1] - J[1][1]*J[2][0]);
    
    Ji[0][1] =  deti*(J[0][2]*J[2][1] - J[0][1]*J[2][2]);
    Ji[1][1] =  deti*(J[0][0]*J[2][2] - J[0][2]*J[2][0]);
    Ji[2][1] =  deti*(J[0][1]*J[2][0] - J[0][0]*J[2][1]);
    
    Ji[0][2] =  deti*(J[0][1]*J[1][2] - J[1][1]*J[0][2]);
    Ji[1][2] =  deti*(J[0][2]*J[1][0] - J[0][0]*J[1][2]);
    Ji[2][2] =  deti*(J[0][0]*J[1][1] - J[0][1]*J[1][0]);
    
    return det;
}

//-----------------------------------------------------------------------------
// Calculates the forces due to the stress
void FEElasticShellDomainOld::InternalForces(FEGlobalVector& R)
{
	// element force vector
	vector<double> fe;

	vector<int> lm;

	int NS = m_Elem.size();
	for (int i=0; i<NS; ++i)
	{
		// get the element
		FEShellElement& el = m_Elem[i];

		// create the element force vector and initialize to zero
		int ndof = 6*el.Nodes();
		fe.assign(ndof, 0);

		// calculate element's internal force
		ElementInternalForce(el, fe);

		// get the element's LM vector
		UnpackLM(el, lm);

		// assemble the residual
		R.Assemble(el.m_node, lm, fe);
	}
}

//-----------------------------------------------------------------------------
//! calculates the internal equivalent nodal forces for shell elements
//! Note that we use a one-point gauss integration rule for the thickness
//! integration. This will integrate linear functions exactly.

void FEElasticShellDomainOld::ElementInternalForce(FEShellElement& el, vector<double>& fe)
{
	int i, n;

	// jacobian matrix determinant
	double detJt;

	const double* Mr, *Ms, *M;

	int nint = el.GaussPoints();
	int neln = el.Nodes();

	double*	gw = el.GaussWeights();
	double eta;
    
    vec3d gcnt[3];

	// repeat for all integration points
	for (n=0; n<nint; ++n)
	{
		FEElasticMaterialPoint& pt = *(el.GetMaterialPoint(n)->ExtractData<FEElasticMaterialPoint>());

		// calculate the jacobian
        detJt = detJ(el, n);

		detJt *= gw[n];

		// get the stress vector for this integration point
		mat3ds& s = pt.m_s;

		eta = el.gt(n);

		Mr = el.Hr(n);
		Ms = el.Hs(n);
		M  = el.H(n);
        
        ContraBaseVectors(el, n, gcnt);

		for (i=0; i<neln; ++i)
		{
            vec3d gradM = gcnt[0]*Mr[i] + gcnt[1]*Ms[i];
            vec3d gradP = (gradM*eta + gcnt[2]*M[i])/2;
            vec3d fu = s*gradM;
            vec3d fd = s*gradP;

			// calculate internal force
			// the '-' sign is so that the internal forces get subtracted
			// from the global residual vector
			fe[6*i  ] -= fu.x*detJt;
			fe[6*i+1] -= fu.y*detJt;
			fe[6*i+2] -= fu.z*detJt;

			fe[6*i+3] -= fd.x*detJt;
			fe[6*i+4] -= fd.y*detJt;
			fe[6*i+5] -= fd.z*detJt;
		}
	}
}

//-----------------------------------------------------------------------------
void FEElasticShellDomainOld::BodyForce(FEGlobalVector& R, FEBodyForce& BF)
{
	// element force vector
	vector<double> fe;

	vector<int> lm;

	int NS = m_Elem.size();
	for (int i=0; i<NS; ++i)
	{
		// get the element
		FEShellElement& el = m_Elem[i];

		// create the element force vector and initialize to zero
		int ndof = 6*el.Nodes();
		fe.assign(ndof, 0);

		// apply body forces to shells
		ElementBodyForce(BF, el, fe);

		// get the element's LM vector
		UnpackLM(el, lm);

		// assemble the residual
		R.Assemble(el.m_node, lm, fe);
	}
}

//-----------------------------------------------------------------------------
//! Calculates element body forces for shells

void FEElasticShellDomainOld::ElementBodyForce(FEBodyForce& BF, FEShellElement& el, vector<double>& fe)
{
	double dens0 = m_pMat->Density();

	// integration weights
	double* gw = el.GaussWeights();
    double eta;
	double *M, detJt;

	// loop over integration points
	int nint = el.GaussPoints();
	int neln = el.Nodes();

	for (int n=0; n<nint; ++n)
	{
		FEMaterialPoint& mp = *el.GetMaterialPoint(n);
		FEElasticMaterialPoint& pt = *mp.ExtractData<FEElasticMaterialPoint>();
        
        // calculate density in current configuration
        double dens = dens0/pt.m_J;
        
        // calculate the jacobian
        detJt = detJ(el, n)*gw[n];
        
		M  = el.H(n);
		eta = el.gt(n);

		// get the force
		vec3d f = BF.force(mp);

		for (int i=0; i<neln; ++i)
		{
            vec3d fu = f*(dens*M[i]);
            vec3d fd = fu*(eta/2);
            
			fe[6*i  ] -= fu.x*detJt;
			fe[6*i+1] -= fu.y*detJt;
			fe[6*i+2] -= fu.z*detJt;

			fe[6*i+3] -= fd.x*detJt;
			fe[6*i+4] -= fd.y*detJt;
			fe[6*i+5] -= fd.z*detJt;
		}
	}
}

//-----------------------------------------------------------------------------

void FEElasticShellDomainOld::StiffnessMatrix(FESolver* psolver)
{
	FEModel& fem = psolver->GetFEModel();

	matrix ke;

	vector<int> lm;

	int NS = m_Elem.size();
	for (int iel=0; iel<NS; ++iel)
	{
		FEShellElement& el = m_Elem[iel];

		// get the elements material
		FEMaterial* pmat = m_pMat;

		// create the element's stiffness matrix
		int ndof = 6*el.Nodes();
		ke.resize(ndof, ndof);

		// calculate the element stiffness matrix
		ElementStiffness(iel, ke);

		// get the element's LM vector
		UnpackLM(el, lm);

		// assemble element matrix in global stiffness matrix
		psolver->AssembleStiffness(el.m_node, lm, ke);

		if (fem.GetCurrentStep()->GetPrintLevel() == FE_PRINT_MINOR_ITRS_EXP)
		{
			fprintf(stderr, "Calculating stiffness matrix: %.1lf %% \r", 100.0*(iel)/ NS);
		}
	}
}

//-----------------------------------------------------------------------------
//! Calculates the shell element stiffness matrix

void FEElasticShellDomainOld::ElementStiffness(int iel, matrix& ke)
{
	FEShellElement& el = Element(iel);

	int i, i6, j, j6, n;

	// Get the current element's data
	const int nint = el.GaussPoints();
	const int neln = el.Nodes();

    const double* Mr, *Ms, *M;
    vec3d gradM[FEElement::MAX_NODES], gradP[FEElement::MAX_NODES];

    // jacobian matrix determinant
    double detJt;
	
	// weights at gauss points
	const double *gw = el.GaussWeights();
    double eta;
    
    vec3d gcnt[3];

	// calculate element stiffness matrix
	ke.zero();
	for (n=0; n<nint; ++n)
	{
		FEMaterialPoint& mp = *(el.GetMaterialPoint(n));
		FEElasticMaterialPoint& pt = *(mp.ExtractData<FEElasticMaterialPoint>());

        // calculate the jacobian
        detJt = detJ(el, n);
        
        detJt *= gw[n];
        
        // get the stress and elasticity for this integration point
        mat3ds s = pt.m_s;
        tens4ds C = m_pMat->Tangent(mp);
        
        eta = el.gt(n);
        
        Mr = el.Hr(n);
        Ms = el.Hs(n);
        M  = el.H(n);
        
        ContraBaseVectors(el, n, gcnt);
        
		// ------------ constitutive component --------------

		// setup the material point

		for (i=0; i<neln; ++i)
		{
            gradM[i] = gcnt[0]*Mr[i] + gcnt[1]*Ms[i];
            gradP[i] = (gradM[i]*eta + gcnt[2]*M[i])/2;
		}

		for (i=0, i6=0; i<neln; ++i, i6 += 6)
		{
			for (j=0, j6 = 0; j<neln; ++j, j6 += 6)
			{
                mat3d Kuu = vdotTdotv(gradM[i], C, gradM[j]);
                mat3d Kud = vdotTdotv(gradM[i], C, gradP[j]);
                mat3d Kdu = vdotTdotv(gradP[i], C, gradM[j]);
                mat3d Kdd = vdotTdotv(gradP[i], C, gradP[j]);
                
				ke[i6  ][j6  ] += Kuu(0,0)*detJt; ke[i6  ][j6+1] += Kuu(0,1)*detJt; ke[i6  ][j6+2] += Kuu(0,2)*detJt;
                ke[i6+1][j6  ] += Kuu(1,0)*detJt; ke[i6+1][j6+1] += Kuu(1,1)*detJt; ke[i6+1][j6+2] += Kuu(1,2)*detJt;
                ke[i6+2][j6  ] += Kuu(2,0)*detJt; ke[i6+2][j6+1] += Kuu(2,1)*detJt; ke[i6+2][j6+2] += Kuu(2,2)*detJt;

                ke[i6  ][j6+3] += Kud(0,0)*detJt; ke[i6  ][j6+4] += Kud(0,1)*detJt; ke[i6  ][j6+5] += Kud(0,2)*detJt;
				ke[i6+1][j6+3] += Kud(1,0)*detJt; ke[i6+1][j6+4] += Kud(1,1)*detJt; ke[i6+1][j6+5] += Kud(1,2)*detJt;
				ke[i6+2][j6+3] += Kud(2,0)*detJt; ke[i6+2][j6+4] += Kud(2,1)*detJt; ke[i6+2][j6+5] += Kud(2,2)*detJt;

				ke[i6+3][j6  ] += Kdu(0,0)*detJt; ke[i6+3][j6+1] += Kdu(0,1)*detJt; ke[i6+3][j6+2] += Kdu(0,2)*detJt;
                ke[i6+4][j6  ] += Kdu(1,0)*detJt; ke[i6+4][j6+1] += Kdu(1,1)*detJt; ke[i6+4][j6+2] += Kdu(1,2)*detJt;
                ke[i6+5][j6  ] += Kdu(2,0)*detJt; ke[i6+5][j6+1] += Kdu(2,1)*detJt; ke[i6+5][j6+2] += Kdu(2,2)*detJt;
                
				ke[i6+3][j6+3] += Kdd(0,0)*detJt; ke[i6+3][j6+4] += Kdd(0,1)*detJt; ke[i6+3][j6+5] += Kdd(0,2)*detJt;
				ke[i6+4][j6+3] += Kdd(1,0)*detJt; ke[i6+4][j6+4] += Kdd(1,1)*detJt; ke[i6+4][j6+5] += Kdd(1,2)*detJt;
				ke[i6+5][j6+3] += Kdd(2,0)*detJt; ke[i6+5][j6+4] += Kdd(2,1)*detJt; ke[i6+5][j6+5] += Kdd(2,2)*detJt;
			}
		}

		// ------------ initial stress component --------------
	
		for (i=0; i<neln; ++i)
			for (j=0; j<neln; ++j)
			{
                double Kuu = gradM[i]*(s*gradM[j]);
                double Kud = gradM[i]*(s*gradP[j]);
                double Kdu = gradP[i]*(s*gradM[j]);
                double Kdd = gradP[i]*(s*gradP[j]);
                
				// the u-u component
				ke[6*i  ][6*j  ] += Kuu*detJt;
				ke[6*i+1][6*j+1] += Kuu*detJt;
				ke[6*i+2][6*j+2] += Kuu*detJt;

				// the u-d component
				ke[6*i  ][6*j+3] += Kud*detJt;
				ke[6*i+1][6*j+4] += Kud*detJt;
				ke[6*i+2][6*j+5] += Kud*detJt;

				// the d-u component
				ke[6*i+3][6*j  ] += Kdu*detJt;
				ke[6*i+4][6*j+1] += Kdu*detJt;
				ke[6*i+5][6*j+2] += Kdu*detJt;

				// the d-d component
				ke[6*i+3][6*j+3] += Kdd*detJt;
				ke[6*i+4][6*j+4] += Kdd*detJt;
				ke[6*i+5][6*j+5] += Kdd*detJt;
			}

	} // end loop over gauss-points

}


//-----------------------------------------------------------------------------
//! Calculates body forces for shells

void FEElasticShellDomainOld::ElementBodyForce(FEModel& fem, FEShellElement& el, vector<double>& fe)
{
	int NF = fem.BodyLoads();
	for (int nf = 0; nf < NF; ++nf)
	{
		FEBodyForce* pbf = dynamic_cast<FEBodyForce*>(fem.GetBodyLoad(nf));
		if (pbf)
		{
            double dens0 = m_pMat->Density();
            
            // integration weights
            double* gw = el.GaussWeights();
            double eta;
            double *M, detJt;
            
            // loop over integration points
            int nint = el.GaussPoints();
            int neln = el.Nodes();
            
            for (int n=0; n<nint; ++n)
            {
                FEMaterialPoint& mp = *el.GetMaterialPoint(n);
                FEElasticMaterialPoint& pt = *mp.ExtractData<FEElasticMaterialPoint>();
                
                // calculate density in current configuration
                double dens = dens0/pt.m_J;
                
                // calculate the jacobian
                detJt = detJ(el, n)*gw[n];
                
                M  = el.H(n);
                eta = el.gt(n);
                
                // get the force
                vec3d f = pbf->force(mp);
                
                for (int i=0; i<neln; ++i)
                {
                    vec3d fu = f*(dens*M[i]);
                    vec3d fd = fu*(eta/2);
                    
                    fe[6*i  ] -= fu.x*detJt;
                    fe[6*i+1] -= fu.y*detJt;
                    fe[6*i+2] -= fu.z*detJt;
                    
                    fe[6*i+3] -= fd.x*detJt;
                    fe[6*i+4] -= fd.y*detJt;
                    fe[6*i+5] -= fd.z*detJt;
                }
            }
		}
	}
}

//-----------------------------------------------------------------------------
void FEElasticShellDomainOld::Update(const FETimeInfo& tp)
{
	FEMesh& mesh = *GetMesh();
	vec3d r0[FEElement::MAX_NODES], rt[FEElement::MAX_NODES];

	int n;
	for (int i=0; i<(int) m_Elem.size(); ++i)
	{
		// get the solid element
		FEShellElement& el = m_Elem[i];

		// get the number of integration points
		int nint = el.GaussPoints();

		// number of nodes
		int neln = el.Nodes();

		// nodal coordinates
		for (int j=0; j<neln; ++j)
		{
			r0[j] = mesh.Node(el.m_node[j]).m_r0;
			rt[j] = mesh.Node(el.m_node[j]).m_rt;
		}

		// loop over the integration points and calculate
		// the stress at the integration point
		for (n=0; n<nint; ++n)
		{
			FEMaterialPoint& mp = *(el.GetMaterialPoint(n));
			FEElasticMaterialPoint& pt = *(mp.ExtractData<FEElasticMaterialPoint>());

			// material point coordinates
			// TODO: I'm not entirly happy with this solution
			//		 since the material point coordinates are used by most materials.
			pt.m_r0 = el.Evaluate(r0, n);
			pt.m_rt = el.Evaluate(rt, n);

			// get the deformation gradient and determinant
			pt.m_J = defgrad(el, pt.m_F, n);

			// calculate the stress at this material point
			pt.m_s = m_pMat->Stress(mp);
		}
	}
}


//-----------------------------------------------------------------------------
//! Unpack the element. That is, copy element data in traits structure
//! Note that for the shell elements the lm order is different compared
//! to the solid element ordering. This is because for shell elements the
//! nodes have six degrees of freedom each, where for solids they only
//! have 3 dofs.
void FEElasticShellDomainOld::UnpackLM(FEElement& el, vector<int>& lm)
{
	int N = el.Nodes();
	lm.resize(N*9);
	for (int i=0; i<N; ++i)
	{
		FENode& node = m_pMesh->Node(el.m_node[i]);
		vector<int>& id = node.m_ID;

		// first the displacement dofs
		lm[6*i  ] = id[m_dofX];
		lm[6*i+1] = id[m_dofY];
		lm[6*i+2] = id[m_dofZ];

		// next the rotational dofs
		lm[6*i+3] = id[m_dofU];
		lm[6*i+4] = id[m_dofV];
		lm[6*i+5] = id[m_dofW];

		// rigid rotational dofs
		lm[6*N + 3*i  ] = id[m_dofRU];
		lm[6*N + 3*i+1] = id[m_dofRV];
		lm[6*N + 3*i+2] = id[m_dofRW];
	}
}