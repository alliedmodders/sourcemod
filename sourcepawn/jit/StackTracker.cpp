#include <malloc.h>
#include <string.h>
#include <assert.h>
#include "StackTracker.h"

using namespace SourcePawn;
using namespace SourceHook;


void StackTracker::reset(JsiBufWriter *writer, JIns *frm)
{
	m_pFrm = frm;
	m_pBuf = writer;
	m_StackPtr = 0;
	m_Regions.clear();
	memset(&m_LocalParams[0], 0, sizeof(m_LocalParams));
}

cell_t StackTracker::top()
{
	return m_StackPtr;
}

void StackTracker::add(cell_t amt)
{
	stack_region_t region;

	m_StackPtr -= amt;
	region.position = m_StackPtr;
	region.value = NULL;

	m_Regions.push_back(region);
}

JIns *StackTracker::get(cell_t offs)
{
	stack_region_t *region;

	if ((region = find_region(offs)) == NULL)
	{
		return NULL;
	}

	/* If we're a parameter, we can propagate a default load */
	if (region->value == NULL && offs >= 0)
	{
		region->value = m_pBuf->ins_loadi(m_pFrm, offs);
	}

	return region->value;
}

bool StackTracker::set(cell_t offs, JIns *value)
{
	stack_region_t *region;

	if ((region = find_region(offs)) == NULL)
	{
		return false;
	}

	region->value = value;
	m_pBuf->ins_storei(m_pFrm, region->position, value);

	return true;
}

stack_region_t *StackTracker::find_region(cell_t offs)
{
	if (offs < 0)
	{
		List<stack_region_t>::iterator iter;

		for (iter = m_Regions.begin(); iter != m_Regions.end(); iter++)
		{
			if ((*iter).position == offs)
			{
				return &(*iter);
			}
		}
	}
	else
	{
		offs >>= 2;

		if (offs >= STACK_FRAME_REACH)
		{
			return NULL;
		}

		return &m_LocalParams[offs];
	}

	return NULL;
}

bool StackTracker::drop(cell_t amt)
{
	if (m_StackPtr + amt > 0)
	{
		return false;
	}

	m_pBuf->ins_stkdrop(amt);

	/* :TODO: Make this backwards and short-cut out... */
	List<stack_region_t>::iterator iter = m_Regions.begin();
	while (iter != m_Regions.end())
	{
		if ((*iter).position < m_StackPtr)
		{
			iter = m_Regions.erase(iter);
		}
		else
		{
			iter++;
		}
	}

	return true;
}
