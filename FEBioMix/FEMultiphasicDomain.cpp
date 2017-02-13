#include "FEMultiphasicDomain.h"
#include "FECore/FEModel.h"

//-----------------------------------------------------------------------------
FEMultiphasicDomain::FEMultiphasicDomain(FEModel* pfem) : FEElasticDomain(pfem)
{
    m_pMat = 0;
    m_dofP = pfem->GetDOFIndex("p");
    m_dofQ = pfem->GetDOFIndex("q");
    
    m_dofC = pfem->GetDOFIndex("concentration", 0);
    m_dofD = pfem->GetDOFIndex("shell concentration", 0);
}
