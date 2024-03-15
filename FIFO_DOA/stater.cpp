#include "stater.h"

bool Stater::UpdateMoms(double* data, unsigned int var)
{
	if(var < N_vars)
	{
		double tmp = data[var];
		for(int m = 0; m < N_moms; ++m)
		{
			moms[var][m] = stubb*moms[var][m] + learn*tmp;
			tmp = tmp*data[var];
		}
		mean[var] = moms[var][0];
		variance[var] = moms[var][1] - mean[var]*mean[var];
		return true;
	}
	else
		return false;
}

bool Stater::UpdateMomsAll(double* data)
{
	for(int v = 0; v < N_vars; ++v)
		if(not UpdateMoms(data, v))
			return false;
	return true;
}
