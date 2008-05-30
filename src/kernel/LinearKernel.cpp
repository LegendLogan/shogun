/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * Written (W) 1999-2008 Soeren Sonnenburg
 * Copyright (C) 1999-2008 Fraunhofer Institute FIRST and Max-Planck-Society
 */

#include "lib/common.h"
#include "lib/io.h"
#include "features/Features.h"
#include "features/RealFeatures.h"
#include "kernel/LinearKernel.h"

CLinearKernel::CLinearKernel(INT size, DREAL s)
: CSimpleKernel<DREAL>(size), scale(s), initialized(false),
	normal(NULL), normal_length(0)
{
	properties |= KP_LINADD;
}

CLinearKernel::CLinearKernel(CRealFeatures* l, CRealFeatures* r, DREAL s, INT size)
: CSimpleKernel<DREAL>(size), scale(s), initialized(false),
	normal(NULL), normal_length(0)
{
	properties |= KP_LINADD;
	init(l,r);
}

CLinearKernel::~CLinearKernel()
{
	cleanup();
}

bool CLinearKernel::init(CFeatures* l, CFeatures* r)
{
	CSimpleKernel<DREAL>::init(l, r);

	if (!initialized)
		init_rescale();

	SG_INFO( "rescaling kernel by %g (num:%d)\n",scale, CMath::min(l->get_num_vectors(), r->get_num_vectors()));

	return true;
}

void CLinearKernel::init_rescale()
{
	if (scale!=0.0)
		return;
	double sum=0;
	scale=1.0;
	for (INT i=0; (i<lhs->get_num_vectors() && i<rhs->get_num_vectors()); i++)
			sum+=compute(i, i);

	scale=sum/CMath::min(lhs->get_num_vectors(), rhs->get_num_vectors());
	initialized=true;
}

void CLinearKernel::cleanup()
{
	delete_optimization();

	CKernel::cleanup();
}

bool CLinearKernel::load_init(FILE* src)
{
	return false;
}

bool CLinearKernel::save_init(FILE* dest)
{
	return false;
}

void CLinearKernel::clear_normal()
{
	int num = ((CRealFeatures*) lhs)->get_num_features();
	if (normal==NULL)
	{
		normal = new DREAL[num];
		normal_length=num;
	}

	memset(normal, 0, sizeof(DREAL)*normal_length);

	set_is_initialized(true);
}

void CLinearKernel::add_to_normal(INT idx, DREAL weight) 
{
	INT vlen;
	bool vfree;
	double* vec=((CRealFeatures*) lhs)->get_feature_vector(idx, vlen, vfree);

	for (int i=0; i<vlen; i++)
		normal[i]+= weight*vec[i];

	((CRealFeatures*) lhs)->free_feature_vector(vec, idx, vfree);

	set_is_initialized(true);
}
  
DREAL CLinearKernel::compute(INT idx_a, INT idx_b)
{
  INT alen, blen;
  bool afree, bfree;

  double* avec=((CRealFeatures*) lhs)->get_feature_vector(idx_a, alen, afree);
  double* bvec=((CRealFeatures*) rhs)->get_feature_vector(idx_b, blen, bfree);
  
  ASSERT(alen==blen);

  DREAL result=CMath::dot(avec, bvec, alen) / scale;

  ((CRealFeatures*) lhs)->free_feature_vector(avec, idx_a, afree);
  ((CRealFeatures*) rhs)->free_feature_vector(bvec, idx_b, bfree);

  return result;
}

bool CLinearKernel::init_optimization(INT num_suppvec, INT* sv_idx, DREAL* alphas) 
{
	clear_normal();

	for (int i=0; i<num_suppvec; i++)
		add_to_normal(sv_idx[i], alphas[i]);

	set_is_initialized(true);
	return true;
}

bool CLinearKernel::delete_optimization()
{
	delete[] normal;
	normal_length=0;
	normal=NULL;
	set_is_initialized(false);

	return true;
}

DREAL CLinearKernel::compute_optimized(INT idx) 
{
	ASSERT(get_is_initialized());

	INT vlen;
	bool vfree;
	double* vec=((CRealFeatures*) rhs)->get_feature_vector(idx, vlen, vfree);
	ASSERT(vlen==normal_length);
	DREAL result=CMath::dot(normal,vec, vlen)/scale;
	((CRealFeatures*) rhs)->free_feature_vector(vec, idx, vfree);

	return result;
}
