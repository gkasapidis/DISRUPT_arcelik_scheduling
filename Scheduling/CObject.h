//#include <string>
#include <iostream>
#include <stdio.h>
#include <string.h>

#ifndef COBJECT_H
#define COBJECT_H

class CObject
{
public:
	void *operator new(size_t sz);
	//virtual ~CObject() {};
};


class CObArray
{
public:

	// Construction
	CObArray();

	// Attributes
	int GetSize();
	int GetUpperBound() const;
	void SetSize(int nNewSize, int nGrowBy = -1);

	// Operations

	// Clean up
	void FreeExtra();
	void RemoveAll();

	// Accessing elements
	CObject *GetAt(unsigned nIndex);
	void SetAt(int nIndex, CObject* newElement);

	CObject*& ElementAt(int nIndex);

	// Potentially growing the array
	void SetAtGrow(int nIndex, CObject* newElement);
	void Add(CObject* newElement);
	int Append(const CObArray& src);
	void Copy(const CObArray& src);

	// overloaded operator helpers
	CObject* operator[](int nIndex);

	// Operations that move elements around
	void InsertAt(int nIndex, CObject* newElement, int nCount = 1);
	void RemoveAt(int nIndex, int nCount = 1);
	void InsertAt(int nStartIndex, CObArray* newElement);

protected:
	CObject** m_pData;   // the actual array of data
	int m_nSize;     // # of elements (upperBound - 1)
	int m_nMaxSize;  // max allocated
	int m_nGrowBy;   // grow amount

public:
	~CObArray();

};


#endif