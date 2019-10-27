#include <iostream>
#include "CObject.h"
#include <assert.h>
#include "Parameters.h"

typedef unsigned char BYTE;

void* CObject::operator new(size_t sz){
	void *ret = NULL;
	ret = (void *) ::operator new(sz);
	return ret;
};

CObject* CObArray::operator[](int nIndex)
{
	if (nIndex < m_nSize) return m_pData[nIndex];
	else throw "Trying to access CObArray element beyond its size";
}

int CObArray::GetSize() {
	return m_nSize;
}
void CObArray::Add(CObject* newElement)
{
	//SetSize(m_nSize + 1);
	assert(this != NULL);
	InsertAt(m_nSize, newElement, 1);
}
CObArray::CObArray()
{
	m_pData = NULL;
	m_nSize = m_nMaxSize = m_nGrowBy = 0;
}
CObArray::~CObArray()
{
	//ASSERT_VALID(this);
	delete[](BYTE*)m_pData;
}
void CObArray::SetSize(int nNewSize, int nGrowBy)
{
	//ASSERT_VALID(this);

	assert(nNewSize >= 0);

	if (nGrowBy != -1)
		m_nGrowBy = nGrowBy;  // set new size

	if (nNewSize == 0)
	{
		// shrink to nothing
		delete[](BYTE*)m_pData;
		m_pData = NULL;
		m_nSize = m_nMaxSize = 0;
	}
	else if (m_pData == NULL)
	{
		// create one with exact size
#ifdef PRint_MAX
		ASSERT(nNewSize <= PRint_MAX / sizeof(CObject*));    // no overflow
#endif
		m_pData = (CObject**) new BYTE[nNewSize * sizeof(CObject*)];

		memset(m_pData, 0, nNewSize * sizeof(CObject*));  // zero fill

		m_nSize = m_nMaxSize = nNewSize;
	}
	else if (nNewSize <= m_nMaxSize)
	{
		// it fits
		if (nNewSize > m_nSize)
		{
			// initialize the new elements

			memset(&m_pData[m_nSize], 0, (nNewSize - m_nSize) * sizeof(CObject*));

		}

		m_nSize = nNewSize;
	}
	else
	{
		// otherwise, grow array
		int nGrowBy = m_nGrowBy;
		if (nGrowBy == 0)
		{
			// heuristically determine growth when nGrowBy == 0
			//  (this avoids heap fragmentation in many situations)
			nGrowBy = std::min(1024, std::max(4, m_nSize / 8));
		}
		int nNewMax;
		if (nNewSize < m_nMaxSize + nGrowBy)
			nNewMax = m_nMaxSize + nGrowBy;  // granularity
		else
			nNewMax = nNewSize;  // no slush

		assert(nNewMax >= m_nMaxSize);  // no wrap around
#ifdef PRint_MAX
		assert(nNewMax <= PRint_MAX / sizeof(CObject*)); // no overflow
#endif
		CObject** pNewData = (CObject**) new BYTE[nNewMax * sizeof(CObject*)];

		// copy new data from old
		//memcpy(pNewData, m_pData, m_nSize * sizeof(CObject*));
		memmove(pNewData, m_pData, m_nSize * sizeof(CObject*));

		// construct remaining elements
		assert(nNewSize > m_nSize);

		memset(&pNewData[m_nSize], 0, (nNewSize - m_nSize) * sizeof(CObject*));


		// get rid of old stuff (note: no destructors called)
		delete[](BYTE*)m_pData;
		m_pData = pNewData;
		m_nSize = nNewSize;
		m_nMaxSize = nNewMax;
	}
}
int CObArray::Append(const CObArray& src)
{
	//ASSERT_VALID(this);
	assert(this != &src);   // cannot append to itself

	int nOldSize = m_nSize;
	SetSize(m_nSize + src.m_nSize);

	//memcpy(m_pData + nOldSize, src.m_pData, src.m_nSize * sizeof(CObject*));
	memmove(m_pData + nOldSize, src.m_pData, src.m_nSize * sizeof(CObject*));

	return nOldSize;
}
void CObArray::Copy(const CObArray& src)
{
	//ASSERT_VALID(this);
	assert(this != &src);   // cannot append to itself

	SetSize(src.m_nSize);

	//memcpy(m_pData, src.m_pData, src.m_nSize * sizeof(CObject*));
	memmove(m_pData, src.m_pData, src.m_nSize * sizeof(CObject*));

}
void CObArray::FreeExtra()
{
	//ASSERT_VALID(this);

	if (m_nSize != m_nMaxSize)
	{
		// shrink to desired size
#ifdef PRint_MAX
		ASSERT(m_nSize <= PRint_MAX / sizeof(CObject*)); // no overflow
#endif
		CObject** pNewData = NULL;
		if (m_nSize != 0)
		{
			pNewData = (CObject**) new BYTE[m_nSize * sizeof(CObject*)];
			// copy new data from old
			//memcpy(pNewData, m_pData, m_nSize * sizeof(CObject*));
			memmove(pNewData, m_pData, m_nSize * sizeof(CObject*));
		}

		// get rid of old stuff (note: no destructors called)
		delete[](BYTE*)m_pData;
		m_pData = pNewData;
		m_nMaxSize = m_nSize;
	}
}
void CObArray::SetAtGrow(int nIndex, CObject* newElement)
{
	//ASSERT_VALID(this);
	assert(nIndex >= 0);

	if (nIndex >= m_nSize) SetSize(nIndex + 1);

	m_pData[nIndex] = newElement;
}
void CObArray::SetAt(int nIndex, CObject* newElement)
{
	this->SetAtGrow(nIndex, newElement);
}
void CObArray::InsertAt(int nIndex, CObject* newElement, int nCount)
{

	//ASSERT_VALID(this);
	assert(this != NULL);
	assert(nIndex >= 0);    // will expand to meet need
	assert(nCount > 0);     // zero or negative size not allowed

	if (nIndex >= m_nSize)
	{
		// adding after the end of the array
		SetSize(nIndex + nCount);  // grow so nIndex is valid
	}
	else
	{
		// inserting in the middle of the array
		int nOldSize = m_nSize;
		SetSize(m_nSize + nCount);  // grow it to new size
									// shift old data up to fill gap
		memmove(&m_pData[nIndex + nCount], &m_pData[nIndex],
			(nOldSize - nIndex) * sizeof(CObject*));

		// re-init slots we copied from

		memset(&m_pData[nIndex], 0, nCount * sizeof(CObject*));

	}

	// insert new value in the gap
	assert(nIndex + nCount <= m_nSize);



	// copy elements into the empty space
	while (nCount--)
		m_pData[nIndex++] = newElement;

}

void CObArray::RemoveAt(int nIndex, int nCount)
{
	// ASSERT_VALID(this);
	assert(nIndex >= 0);
	assert(nCount >= 0);
	assert(nIndex + nCount <= m_nSize);

	// just remove a range
	int nMoveCount = m_nSize - (nIndex + nCount);

	if (nMoveCount)
		memmove(&m_pData[nIndex], &m_pData[nIndex + nCount], nMoveCount * sizeof(CObject*));
	//memcpy(&m_pData[nIndex], &m_pData[nIndex + nCount],	
	m_nSize -= nCount;
}

void CObArray::InsertAt(int nStartIndex, CObArray* pNewArray)
{
	//ASSERT_VALID(this);
	assert(pNewArray != NULL);
	assert(nStartIndex >= 0);

	const unsigned size = pNewArray->GetSize();
	//ebala to int i toy for parakato san unsigned int i
	unsigned int i;
	if (size > 0) {
		InsertAt(nStartIndex, pNewArray->GetAt(0), size);
		for (i = 1; i < size; i++)	SetAt(nStartIndex + i, pNewArray->GetAt(i));
	}
}
CObject *CObArray::GetAt(unsigned nIndex) {

	//ebala unsigned sto m_nSize
	if (nIndex >= unsigned(m_nSize)) throw "Array bound violation. ";
	else return m_pData[nIndex];

}
void CObArray::RemoveAll(void) {

	if (m_nSize >  0) {

		assert(m_pData != NULL);

		delete[](BYTE*)m_pData;
		m_pData = NULL;
		m_nSize = m_nMaxSize = 0;

	}
}
