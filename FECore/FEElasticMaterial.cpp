#include "stdafx.h"
#include "FEElasticMaterial.h"
#include "FEModel.h"

//-----------------------------------------------------------------------------
// Material parameters for FEElasticMaterial
BEGIN_PARAMETER_LIST(FEElasticMaterial, FEMaterial)
	ADD_PARAMETER(m_density, FE_PARAM_DOUBLE, "density");
END_PARAMETER_LIST();

//-----------------------------------------------------------------------------
//! Calculates the right Cauchy-Green tensor at the current material point

mat3ds FEElasticMaterialPoint::RightCauchyGreen()
{
	// get the right Cauchy-Green tensor
	// C = Ft*F
	mat3ds C;
	C.xx() = F[0][0]*F[0][0]+F[1][0]*F[1][0]+F[2][0]*F[2][0]; // = C[0][0]
	C.yy() = F[0][1]*F[0][1]+F[1][1]*F[1][1]+F[2][1]*F[2][1]; // = C[1][1]
	C.zz() = F[0][2]*F[0][2]+F[1][2]*F[1][2]+F[2][2]*F[2][2]; // = C[2][2]
	C.xy() = F[0][0]*F[0][1]+F[1][0]*F[1][1]+F[2][0]*F[2][1]; // = C[0][1]
	C.yz() = F[0][1]*F[0][2]+F[1][1]*F[1][2]+F[2][1]*F[2][2]; // = C[1][2]
	C.xz() = F[0][0]*F[0][2]+F[1][0]*F[1][2]+F[2][0]*F[2][2]; // = C[0][2]

	return C;
}

//-----------------------------------------------------------------------------
//! Calculates the left Cauchy-Green tensor at the current material point

mat3ds FEElasticMaterialPoint::LeftCauchyGreen()
{
	// get the left Cauchy-Green tensor
	// b = F*Ft
	mat3ds b;
	b.xx() = F[0][0]*F[0][0]+F[0][1]*F[0][1]+F[0][2]*F[0][2]; // = b[0][0]
	b.yy() = F[1][0]*F[1][0]+F[1][1]*F[1][1]+F[1][2]*F[1][2]; // = b[1][1]
	b.zz() = F[2][0]*F[2][0]+F[2][1]*F[2][1]+F[2][2]*F[2][2]; // = b[2][2]
	b.xy() = F[0][0]*F[1][0]+F[0][1]*F[1][1]+F[0][2]*F[1][2]; // = b[0][1]
	b.yz() = F[1][0]*F[2][0]+F[1][1]*F[2][1]+F[1][2]*F[2][2]; // = b[1][2]
	b.xz() = F[0][0]*F[2][0]+F[0][1]*F[2][1]+F[0][2]*F[2][2]; // = b[0][2]

	return b;
}

//-----------------------------------------------------------------------------
//! Calculates the right Cauchy-Green tensor at the current material point

mat3ds FEElasticMaterialPoint::DevRightCauchyGreen()
{
	double Jm23 = pow(J, -2.0/3.0);

	// get the deviatoric right Cauchy-Green tensor
	// C = Ft*F
	mat3ds C;
	C.xx() = Jm23*(F[0][0]*F[0][0]+F[1][0]*F[1][0]+F[2][0]*F[2][0]); // = C[0][0]
	C.yy() = Jm23*(F[0][1]*F[0][1]+F[1][1]*F[1][1]+F[2][1]*F[2][1]); // = C[1][1]
	C.zz() = Jm23*(F[0][2]*F[0][2]+F[1][2]*F[1][2]+F[2][2]*F[2][2]); // = C[2][2]
	C.xy() = Jm23*(F[0][0]*F[0][1]+F[1][0]*F[1][1]+F[2][0]*F[2][1]); // = C[0][1]
	C.yz() = Jm23*(F[0][1]*F[0][2]+F[1][1]*F[1][2]+F[2][1]*F[2][2]); // = C[1][2]
	C.xz() = Jm23*(F[0][0]*F[0][2]+F[1][0]*F[1][2]+F[2][0]*F[2][2]); // = C[0][2]

	return C;
}

//-----------------------------------------------------------------------------
//! Calculates the left Cauchy-Green tensor at the current material point

mat3ds FEElasticMaterialPoint::DevLeftCauchyGreen()
{
	double Jm23 = pow(J, -2.0/3.0);

	// get the left Cauchy-Green tensor
	// b = F*Ft
	mat3ds b;
	b.xx() = Jm23*(F[0][0]*F[0][0]+F[0][1]*F[0][1]+F[0][2]*F[0][2]); // = b[0][0]
	b.yy() = Jm23*(F[1][0]*F[1][0]+F[1][1]*F[1][1]+F[1][2]*F[1][2]); // = b[1][1]
	b.zz() = Jm23*(F[2][0]*F[2][0]+F[2][1]*F[2][1]+F[2][2]*F[2][2]); // = b[2][2]
	b.xy() = Jm23*(F[0][0]*F[1][0]+F[0][1]*F[1][1]+F[0][2]*F[1][2]); // = b[0][1]
	b.yz() = Jm23*(F[1][0]*F[2][0]+F[1][1]*F[2][1]+F[1][2]*F[2][2]); // = b[1][2]
	b.xz() = Jm23*(F[0][0]*F[2][0]+F[0][1]*F[2][1]+F[0][2]*F[2][2]); // = b[0][2]

	return b;
}

//-----------------------------------------------------------------------------
//! Calculates the Euler-Lagrange strain at the current material point

mat3ds FEElasticMaterialPoint::Strain()
{
	// get the right Cauchy-Green tensor
	// C = Ft*F
	mat3ds C;
	C.xx() = F[0][0]*F[0][0]+F[1][0]*F[1][0]+F[2][0]*F[2][0]; // = C[0][0]
	C.yy() = F[0][1]*F[0][1]+F[1][1]*F[1][1]+F[2][1]*F[2][1]; // = C[1][1]
	C.zz() = F[0][2]*F[0][2]+F[1][2]*F[1][2]+F[2][2]*F[2][2]; // = C[2][2]
	C.xy() = F[0][0]*F[0][1]+F[1][0]*F[1][1]+F[2][0]*F[2][1]; // = C[0][1]
	C.yz() = F[0][1]*F[0][2]+F[1][1]*F[1][2]+F[2][1]*F[2][2]; // = C[1][2]
	C.xz() = F[0][0]*F[0][2]+F[1][0]*F[1][2]+F[2][0]*F[2][2]; // = C[0][2]

	// calculate the Euler-Lagrange strain
	// E = 1/2*(C - 1)
	mat3ds E = (C - mat3dd(1))*0.5;

	return E;
}

//-----------------------------------------------------------------------------
//! Calculates the small-strain tensor from the deformation gradient
mat3ds FEElasticMaterialPoint::SmallStrain()
{
	// caculate small strain tensor
	return mat3ds(F[0][0] - 1.0, F[1][1] - 1.0, F[2][2] - 1.0, 0.5*(F[0][1] + F[1][0]), 0.5*(F[0][2] + F[2][0]), 0.5*(F[1][2] + F[2][1]));
}

//-----------------------------------------------------------------------------
//! Calculates the 2nd PK stress from the Cauchy stress stored in the point

mat3ds FEElasticMaterialPoint::pull_back(const mat3ds& A)
{
	mat3d Fi = F.inverse();
	mat3d P = Fi*A;

	return mat3ds(J*(P[0][0]*Fi[0][0]+P[0][1]*Fi[0][1]+P[0][2]*Fi[0][2]),
				  J*(P[1][0]*Fi[1][0]+P[1][1]*Fi[1][1]+P[1][2]*Fi[1][2]),
				  J*(P[2][0]*Fi[2][0]+P[2][1]*Fi[2][1]+P[2][2]*Fi[2][2]),
				  J*(P[0][0]*Fi[1][0]+P[0][1]*Fi[1][1]+P[0][2]*Fi[1][2]),
				  J*(P[1][0]*Fi[2][0]+P[1][1]*Fi[2][1]+P[1][2]*Fi[2][2]),
				  J*(P[0][0]*Fi[2][0]+P[0][1]*Fi[2][1]+P[0][2]*Fi[2][2]));
}

mat3ds FEElasticMaterialPoint::push_forward(const mat3ds& A)
{
	mat3d P = F*A;
	double Ji = 1 / J;

	return mat3ds(Ji*(P[0][0]*F[0][0]+P[0][1]*F[0][1]+P[0][2]*F[0][2]),
				  Ji*(P[1][0]*F[1][0]+P[1][1]*F[1][1]+P[1][2]*F[1][2]),
				  Ji*(P[2][0]*F[2][0]+P[2][1]*F[2][1]+P[2][2]*F[2][2]),
				  Ji*(P[0][0]*F[1][0]+P[0][1]*F[1][1]+P[0][2]*F[1][2]),
				  Ji*(P[1][0]*F[2][0]+P[1][1]*F[2][1]+P[1][2]*F[2][2]),
				  Ji*(P[0][0]*F[2][0]+P[0][1]*F[2][1]+P[0][2]*F[2][2]));
}

//-----------------------------------------------------------------------------
void FEElasticMaterial::Init()
{
	FEMaterial::Init();

	if (m_density <= 0) throw MaterialError("Invalid material density");
}

//-----------------------------------------------------------------------------
void FEElasticMaterial::Serialize(DumpFile& ar)
{
	FEMaterial::Serialize(ar);
	if (ar.IsSaving())
	{
		ar << m_density << m_unstable;
		int ntype = -1;
		if (m_pmap) ntype = m_pmap->m_ntype;
		else ntype = FE_MAP_NONE;
		assert(ntype != -1);
		ar << ntype;
		if (m_pmap) m_pmap->Serialize(ar);
	}
	else
	{
		ar >> m_density >> m_unstable;
		int ntype;
		ar >> ntype;
		if (m_pmap) delete m_pmap;
		m_pmap = 0;
		assert(ntype != -1);
		FEMesh& mesh = ar.GetFEModel()->GetMesh();
		switch (ntype)
		{
		case FE_MAP_NONE    : m_pmap = 0; break;
		case FE_MAP_LOCAL   : m_pmap = new FELocalMap      (mesh); break;
		case FE_MAP_SPHERE  : m_pmap = new FESphericalMap  (mesh); break;
		case FE_MAP_CYLINDER: m_pmap = new FECylindricalMap(mesh); break;
		case FE_MAP_VECTOR  : m_pmap = new FEVectorMap     (    ); break;
		}
		if (m_pmap) m_pmap->Serialize(ar);
	}
}
