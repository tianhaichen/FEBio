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
#include "FEImageDataMap.h"
#include <FECore/FEDomainMap.h>

BEGIN_FECORE_CLASS(FEImageDataMap, FEDomainDataGenerator)
	ADD_PARAMETER(m_r0, "range_min");
	ADD_PARAMETER(m_r1, "range_max");

	ADD_PROPERTY(m_imgSrc, "image");
END_FECORE_CLASS();

FEImageDataMap::FEImageDataMap(FEModel* fem) : FEDomainDataGenerator(fem), m_map(m_im)
{
	m_imgSrc = nullptr;
}

bool FEImageDataMap::Init()
{
	if (m_imgSrc == nullptr) return false;

	if (m_imgSrc->GetImage3D(m_im) == false)
	{
		return false;
	}

	m_map.SetRange(m_r0, m_r1);

	return FEDomainDataGenerator::Init();
}

void FEImageDataMap::value(const vec3d& x, double& data)
{
	data = m_map.value(m_map.map(x));
}

FEDomainMap* FEImageDataMap::Generate()
{
	FEElementSet* elset = GetElementSet();
	if (elset == nullptr) return nullptr;

	// TODO: Can I use FMT_NODE?
	FEDomainMap* map = new FEDomainMap(FEDataType::FE_DOUBLE, Storage_Fmt::FMT_MULT);
	map->Create(elset);
	if (FEDomainDataGenerator::Generate(*map) == false)
	{
		delete map; map = nullptr;
	}
	return map;
}
